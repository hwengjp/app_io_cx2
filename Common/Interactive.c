#include <string.h>

// TWE common lib
#include "twecommon.h"
#include "tweutils.h"
#include "tweserial.h"
#include "tweprintf.h"

#include "twesercmd_gen.h"
#include "twesercmd_plus3.h"
#include "tweinputstring.h"
#include "twestring.h"

#include "twesettings.h"
#include "twesettings_std.h"
#include "twesettings_std_defsets.h"
#include "twesettings_cmd.h"
#include "twesettings_validator.h"

#include "tweinteractive.h"
#include "tweinteractive_defmenus.h"
#include "tweinteractive_settings.h"
#include "tweinteractive_nvmutils.h"

#include "twenvm.h"
#include "twesysutils.h"

// TWELITE hardware like
#include <jendefs.h>
#include <AppHardwareApi.h>

#include "twenet_defs.h"

// app specific
#include "APP_IO_CX2.h"
#include "config.h"
#include "common.h"

/**********************************************************************************
 * DEFINES
 **********************************************************************************/
#define STG_SAVE_BUFF_SIZE 64		//! セーブバッファサイズ

#define STGS_SET_VER 0x01			//! 設定バージョン
#define STGS_SET_VER_COMPAT 0x01	//! 互換性のある設定バージョン

#define STGS_MAX_SETTINGS_COUNT 16	//! 設定数の最大(確定設定リスト tsFinal の配列数を決める)

/**********************************************************************************
 * PROTOTYPE
 **********************************************************************************/
static TWE_APIRET TWESTGS_VLD_u32FPS(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer* pBuf);

/**********************************************************************************
 * CONSTANTS
 **********************************************************************************/
/*!
 * カスタムデフォルト(BASE)
 *   APPIDのデフォルト値を書き換えている
 */
static const uint8 au8CustomDefault_Base[] = {
	22,   // 総バイト数（TWESTG_DEFSETS_BASEの順序に合わせて並び替え、POWER_N_RETRYを追加）
	E_TWESTG_DEFSETS_APPID, (TWESTG_DATATYPE_UINT32 << 4) | 4, 
	((CONFIG_DEFAULT_APPID>>24)&0xFF), ((CONFIG_DEFAULT_APPID>>16)&0xFF), 
	((CONFIG_DEFAULT_APPID>>8)&0xFF), (CONFIG_DEFAULT_APPID&0xFF), // 6bytes (config_private.hから取得、ビッグエンディアン)
	E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8<<4)| 1, CONFIG_DEFAULT_LOGICALID,	// 3bytes (config_private.hから取得)
	E_TWESTG_DEFSETS_CHANNELS_3 , (TWESTG_DATATYPE_UINT16 << 4) | 2, 
	((CONFIG_DEFAULT_CHANNELS_3>>8)&0xFF), (CONFIG_DEFAULT_CHANNELS_3&0xFF), // 4bytes (config_private.hから取得、ビッグエンディアン)
	E_TWESTG_DEFSETS_POWER_N_RETRY, (TWESTG_DATATYPE_UINT8 << 4) | 1, 
	CONFIG_DEFAULT_POWER_N_RETRY, // 3bytes (config_private.hから取得)
	E_TWESTG_DEFSETS_OPTBITS, (TWESTG_DATATYPE_UINT32<<4) | 4, 
	((CONFIG_DEFAULT_OPTBITS>>24)&0xFF), ((CONFIG_DEFAULT_OPTBITS>>16)&0xFF), 
	((CONFIG_DEFAULT_OPTBITS>>8)&0xFF), (CONFIG_DEFAULT_OPTBITS&0xFF), // 6bytes (config_private.hから取得、ビッグエンディアン)
};

/**
 * 追加の設定
 **/
static const TWESTG_tsElement SetSettings[] = {
	{ E_TWESTG_DEFSETS_SLEEP4,
		{TWESTG_DATATYPE_UINT16, sizeof(uint16), 0, 0, { .u16 = 1000 } },
		{	"DR4",
			"Periodic 1sec Mode(4): sleep period[ms]",
			"Enter the sleep period in Mode(4) in milliseconds.\\n"
			"[100-10000: Period in ms / 0: No timer wakeup]"
		},
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 5, 't' },
		{ {.u16 = 0}, {.u16 = 0xFFFF}, TWESTGS_VLD_u32MinMax, NULL }
	},
	{ E_TWESTG_DEFSETS_SLEEP7,
		{TWESTG_DATATYPE_UINT16, sizeof(uint16), 0, 0, { .u16 = 10 } },
		{	"DR7",
			"Periodic 10sec Mode(7): sleep period[s]",
			"Enter the sleep period in Mode(7) in seconds.\\n"
			"[2-10000: Period in sec / 0: No timer wakeup]"
		},
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 5, 'y' },
		{ {.u16 = 0}, {.u16 = 0xFFFF}, TWESTGS_VLD_u32MinMax, NULL }
	},
	{ E_TWESTG_DEFSETS_FPS,
		{ TWESTG_DATATYPE_UINT8, sizeof(uint8), 0 , 0, { .u8 = CONFIG_DEFAULT_FPS } },
		{	"FPS",
			"0.03 sec continuous Mode(3) [4,8,16,32 Hz]",
			"Enter the transmission frequency per second for Mode3. [4,8,16,32: Freq]"
		},
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 8, 'f' },
		{ {.u8 = 0}, {.u8 = 0}, TWESTGS_VLD_u32FPS, NULL }
	},
	{ E_TWESTG_DEFSETS_ENCENABLE,
		{ TWESTG_DATATYPE_UINT8, sizeof(uint8), 0 , 0, { .u8 = CONFIG_DEFAULT_ENCENABLE } },
		{ "ENB",
			"Encryption [0,1]",
			"Switch 128-bit AES encryption. [0: Disabled / 1: Enabled]"
		},
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 1, 'e' },
		{ {.u8 = 0}, {.u8 = 1}, TWESTGS_VLD_u32MinMax, NULL }
	},
	{ E_TWESTG_DEFSETS_ENCKEY,
		{ TWESTG_DATATYPE_STRING, 15, 0 , 0, { .pu8 = (uint8*)CONFIG_DEFAULT_ENCKEY } },
		{ "ECK",
			"Encryption key [15chrs]",
			"Enter the encryption key as 15 characters of text (not binary)."
		},
		{ E_TWEINPUTSTRING_DATATYPE_STRING, 15, 'k' },
		{ {.u8 = 0}, {.u8 = 0}, TWESTGS_VLD_u32String, NULL }
	},
	{E_TWESTG_DEFSETS_VOID}
};

static const TWESTG_tsMsgReplace MSG_US_0[] = {
	{ E_TWESTG_DEFSETS_LOGICALID,
		"Device ID [1-100,etc]",
		"Set if more than one child unit needs to be identified. \\n"
		"[1-100: Child ID / 120: Child no ID / 121: Parent*\\** / 122: Repeater*\\**]\\n"
		"*\\** Set 0 if using Mx pins to enable Parent or Repeater"
	},
	{ E_TWESTG_DEFSETS_OPTBITS,
		"Option bits [HEX:32bit]",
"00000001 Low latency mode            00000002 Low latency mode(Slp INT)\\n"
"00000010 Tx w/ Ack                   00000020 Disable periodic Tx\\n"
"00000100 Enable long press mode      00000200 Disable ch change by C1/C2\\n"
"00000400 Invert Ix input             00000800 Disable pull-ups on Ix\\n"
"00001000 C:8in4out/P:8out4in         00002000 C:6in6out/P:6out6in\\n"
"00003000 C:12out/P:12in              00010000 Force Rx on C\\n"
"00020000 Disable UART out by changes 00040000 Enable Watchdog out on C2\\n"
"00400000 Inverts the output of Ox.\\n"
"C: Child Device, P:Parent Device"
	},
	{E_TWESTG_DEFSETS_VOID}
};

static const TWESTG_tsMsgReplace MSG_JPN_0[] = {
	{ E_TWESTG_DEFSETS_LOGICALID,
		"論理ﾃﾞﾊﾞｲｽID [1-100,etc]",
		"複数の子機を識別する必要がある場合に設定します.\\n"
		"[1-100: 子機ID / 120: 子機IDなし / 121: 親機*\\** / 122: 中継機*\\**]\\n"
		"*\\**Mxﾋﾟﾝで親機や中継機を指定する場合は0"
	},
	{ E_TWESTG_DEFSETS_SLEEP4,
		"子機間欠1秒ﾓｰﾄﾞ(4) ｽﾘｰﾌﾟ期間[ms]",
		"子機間欠1秒ﾓｰﾄﾞ(4)のｽﾘｰﾌﾟ期間をﾐﾘ秒単位で設定します.\\n"
		"[100-10000: 起床間隔 / 0: ﾀｲﾏ起床しない]"
	},
	{ E_TWESTG_DEFSETS_SLEEP7,
		"子機間欠10秒ﾓｰﾄﾞ(7) ｽﾘｰﾌﾟ期間[s]",
		"子機間欠10秒ﾓｰﾄﾞ(7)のｽﾘｰﾌﾟ期間を秒単位で設定します.\\n"
		"[2-10000: 起床間隔 / 0: ﾀｲﾏ起床しない]"
	},
	{ E_TWESTG_DEFSETS_FPS,
		"子機連続0.03秒ﾓｰﾄﾞ(3)のｻｲｸﾙ [4,8,16,32 Hz]",
		"子機連続0.03秒ﾓｰﾄﾞ(3)の1秒間の送信頻度を設定します. [4,8,16,32: 送信頻度]"
	},
	{ E_TWESTG_DEFSETS_ENCENABLE,
		"暗号化 [0,1]",
		"AES128bitの暗号化設定を切り替えます. [0: 無効 / 1: 有効]"
	},
	{ E_TWESTG_DEFSETS_ENCKEY,
		"暗号鍵 [15文字]",
		"暗号鍵として15文字のﾃｷｽﾄを設定します. (ﾊﾞｲﾅﾘでない)"
	},
	{ E_TWESTG_DEFSETS_OPTBITS,
		"ｵﾌﾟｼｮﾝﾋﾞｯﾄ [HEX:32bit]",
//123456789-123456789-123456789-12345678 4 23456789-123456789-123456789-12345
"00000001 低ﾚｲﾃﾝｼﾓｰﾄﾞ                 00000002 低ﾚｲﾃﾝｼﾓｰﾄﾞ(ｽﾘｰﾌﾟ割り込み)\\n"
"00000010 ACK送信の有効化             00000020 定期送信の無効化\\n"
"00000100 ﾘﾓｺﾝ長押しﾓｰﾄﾞの有効化      00000200 C1/C2ﾁｬﾈﾙ切り替えの無効化\\n"
"00000400 Ixの入力を反転              00000800 Ixの内部ﾌﾟﾙｱｯﾌﾟを停止\\n"
"00001000 子機:8入4出/親機:8出/4入    00002000 子機:6入6出/親機:6出/6入\\n"
"00003000 子機:12出/親機:12入         00010000 子機の受信を強制\\n"
"00020000 入出力変化時にUART出力停止    00040000 C2のｳｫｯﾁﾄﾞｯｸﾞ出力を有効化\\n"
"00400000 Oxの出力を反転"
	},
	{ E_TWESTG_DEFSETS_VOID } // terminator
};

/*!
 * 設定定義(tsSettings)
 *   スロット0..7までの定義を記述
 */
static const TWESTG_tsSettingsListItem SetList[1][5] = {
	{
		{ STGS_KIND_MAIN, TWESTG_SLOT_DEFAULT,
			{ TWESTG_DEFSETS_BASE, SetSettings, NULL,
			au8CustomDefault_Base, TWESTG_DEFCUST_REMOVE_CHAN1, NULL } },
		{ TWESTG_KIND_VOID, TWESTD_SLOT_VOID, { NULL }}, // TERMINATE
		{ TWESTG_KIND_VOID, TWESTD_SLOT_VOID, { NULL }}, // TERMINATE
		{ TWESTG_KIND_VOID, TWESTD_SLOT_VOID, { NULL }}, // TERMINATE
		{ TWESTG_KIND_VOID, TWESTD_SLOT_VOID, { NULL }}, // TERMINATE
	},
};

/**
 * @brief 設定リストのメニュー名
 */
static const uint8 SetList_names[] = "";

/**
 * @brief メニュー定義
 */
static const TWEINTRCT_tsFuncs asFuncs[] = {
	{ 0, (uint8*)"ROOT MENU", TWEINTCT_vSerUpdateScreen_defmenus, TWEINTCT_vProcessInputByte_defmenus, TWEINTCT_vProcessInputString_defmenus, TWEINTCT_u32ProcessMenuEvent_defmenus }, // standard settings
	{ 1, (uint8*)"CONFIG MENU", TWEINTCT_vSerUpdateScreen_settings, TWEINTCT_vProcessInputByte_settings, TWEINTCT_vProcessInputString_settings, TWEINTCT_u32ProcessMenuEvent_settings }, // standard settings
	{ 2, (uint8*)"EEPROM UTIL", TWEINTCT_vSerUpdateScreen_nvmutils, TWEINTCT_vProcessInputByte_nvmutils, TWEINTCT_vProcessInputString_nvmutils, TWEINTCT_u32ProcessMenuEvent_nvmutils }, // standard settings
	{ 0xFF, NULL, NULL, NULL }
};

#define MENU_CONFIG 1 // 設定モード
#define MENU_OTA 2    // OTAは２番目

/**********************************************************************************
 * VARIABLES
 **********************************************************************************/

/*!
 * tsFinal 構造体のデータ領域を宣言する
 */
#define MYSTG_COUNT STGS_MAX_SETTINGS_COUNT
#define MYSTG_STRBUF 32
#define MYSTG_CUSTDEF 8
/*!
 * tsFinal 構造体のデータ領域を宣言する
 */
TWESTG_DECLARE_FINAL(MYDAT, MYSTG_COUNT, MYSTG_STRBUF, MYSTG_CUSTDEF); // 確定設定リストの配列等の宣言

/**********************************************************************************
 * STATIC FUNCTIONS
 **********************************************************************************/
/**
 * 設定構造体の初期化最終
 */
TWEINTRCT_tsContext* MWPFX(psInit_Intrct)(TWESTG_tsFinal *psFinal, TWE_tsFILE* psSer, void *vpHandleSerialInput) {
	TWEINTRCT_tsContext* p = TWEINTRCT_pscInit(psFinal, NULL, psSer, vpHandleSerialInput, asFuncs);

	if (p) {
		// set the custom messages for interactive mode
		if (FW_CONF_LANG() == 2) {
			// Japanese message
			p->msgReplace_1 = MSG_JPN_0;
			p->msgReplace_2 = TWESTG_DEFSETS_BASE_MSG_JP;
		} else {
			// English message
			p->msgReplace_1 = MSG_US_0;
		}
	}

	return p;
}

/*!
 * 確定設定リスト(tsFinal)から各設定を読み出す。
 * ※ コード可読性の観点からイテレータ(TWESTG_ITER_tsFinal_*)を用いた読み出しを行う。
 */
void MWPFX(vQueryAppData)() {
	// 設定のクエリ
	TWESTG_ITER_tsFinal sp;

	TWESTG_ITER_tsFinal_BEGIN(sp, &gc_sFinal); // init iterator
	if (!TWESTG_ITER_tsFinal_IS_VALID(sp)) return; //ERROR DATA

	while (!TWESTG_ITER_tsFinal_IS_END(sp)) { // end condition of iter
		uint16 id = TWESTG_ITER_tsFinal_G_ID(sp);
		switch (id) { // get data as UINT32
		case E_TWESTG_DEFSETS_LOGICALID:
			sAppStg.u8id = TWESTG_ITER_tsFinal_G_U8(sp); break;
		case E_TWESTG_DEFSETS_APPID:
			sAppStg.u32appid = TWESTG_ITER_tsFinal_G_U32(sp); break;
		case E_TWESTG_DEFSETS_CHANNELS_3:
			sAppStg.u32chmask = TWESTG_ITER_tsFinal_G_U16(sp)<<11; break;
		case E_TWESTG_DEFSETS_OPTBITS:
			sAppStg.u32Opt = TWESTG_ITER_tsFinal_G_U32(sp); break;
		case E_TWESTG_DEFSETS_POWER_N_RETRY:
			sAppStg.u8pow = TWESTG_ITER_tsFinal_G_U8(sp); break;
		case E_TWESTG_DEFSETS_UARTBAUD:
			{
				uint16 data = TWESTG_ITER_tsFinal_G_U16(sp);
				sAppStg.u32baud_safe = (data&0x0FFF)*100;
				sAppStg.u8parity = data>>12;
			}
			break;
		case E_TWESTG_DEFSETS_SLEEP4:
			sAppStg.u16SleepDur_ms = TWESTG_ITER_tsFinal_G_U16(sp); break;
		case E_TWESTG_DEFSETS_SLEEP7:
			sAppStg.u16SleepDur_s = TWESTG_ITER_tsFinal_G_U16(sp); break;
		case E_TWESTG_DEFSETS_FPS:
			sAppStg.u8Fps = TWESTG_ITER_tsFinal_G_U8(sp); break;
		case E_TWESTG_DEFSETS_HOLDPORT:
			sAppStg.u32HoldMask = TWESTG_ITER_tsFinal_G_U32(sp); break;
		case E_TWESTG_DEFSETS_HOLDTIME:
			sAppStg.u16HoldDur_ms = TWESTG_ITER_tsFinal_G_U16(sp); break;
		case E_TWESTG_DEFSETS_ENCENABLE:
			sAppStg.u8Crypt = TWESTG_ITER_tsFinal_G_U8(sp); break;
		case E_TWESTG_DEFSETS_ENCKEY:
		{
			uint8* pu8key = TWESTG_ITER_tsFinal_G_PU8(sp);
			memcpy( sAppStg.au8AesKey, pu8key, TWESTG_ITER_tsFinal_DLEN(sp) );
			break;
		}

		}
		TWESTG_ITER_tsFinal_INCR(sp); // incrment
	}
}

/*!
 * 確定設定データを再構築する。
 *
 * \param u8kind  種別
 * \param u8slot  スロット
 * \param bNoLoad TRUEならslotに対応するセーブデータを読み込まない。
 */
void MWPFX(vAppLoadData)(uint8 u8kind, uint8 u8slot, bool_t bNoLoad) {
	// 値のチェック
	bool_t bOk = FALSE;
	if (u8kind == STGS_KIND_MAIN && u8slot < STGS_KIND_SLOT_MAX) bOk = TRUE;
	if (!bOk) {
		return;
	}

	/// tsFinal 構造体の初期化とロード
	// tsFinal 構造体の初期化
	TWESTG_INIT_FINAL(MYDAT, &gc_sFinal);

	// tsFinal 構造体に基本情報を適用する
	TWESTG_u32SetBaseInfoToFinal(&gc_sFinal, APP_ID, APPVER, STGS_SET_VER, STGS_SET_VER_COMPAT);
	// tsFinal 構造体に kind, slot より、デフォルト設定リストを構築する
	TWESTG_u32SetSettingsToFinal(&gc_sFinal, u8kind, u8slot, SetList[u8kind]);
	// セーブデータがあればロードする
	TWESTG_u32LoadDataFrAppstrg(&gc_sFinal, u8kind, u8slot, APP_ID, STGS_SET_VER_COMPAT, bNoLoad ? TWESTG_LOAD_OPT_NOLOAD : 0);
}

/*!
 * セーブ用に最初のセクタを計算する。
 *
 * \param u8kind 種別
 * \param u8slot スロット
 * \param u32Opt オプション
 * \return 0xFF:error, その他:セクタ番号
 */
static uint8 s_u8GetFirstSect(uint8 u8kind, uint8 u8slot) {
	return MW_NVMEM_FIRST_SECTOR + 1;
}

/*!
 * データセーブを行う。
 * twesettings ライブラリから呼び出されるコールバック関数。
 *
 * \param pBuf   データ領域 pBuf->pu8buff[-16..-1] を利用することができる。
 * \param u8kind 種別
 * \param u8slot スロット
 * \param u32Opt オプション
 * \param ...
 * \return TWE_APIRET
 */
TWE_APIRET TWESTG_cbu32SaveSetting(TWE_tsBuffer *pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal *psFinal) {
	uint8 u8sect = s_u8GetFirstSect(u8kind, u8slot);
	if (u8sect != 0xFF) {
		bool_t bRes = TWENVM_bWrite(pBuf, u8sect); //先頭セクターはコントロールブロックとして残し、2セクター単位で保存
		return bRes ? TWE_APIRET_SUCCESS : TWE_APIRET_FAIL;
	} else return TWE_APIRET_FAIL;
}

/**
 * データロードを行う。
 * twesettings ライブラリから呼び出されるコールバック関数。
 *
 * @param pBuf 		データ領域 pBuf->pu8buff[-16..-1] を利用することができる。
 * @param u8kind 	種別
 * @param u8slot 	スロット
 * @param u32Opt 	オプション
 * @param ...
 * @return TWE_APIRET
 */
TWE_APIRET TWESTG_cbu32LoadSetting(TWE_tsBuffer *pBuf, uint8 u8kind, uint8 u8slot, uint32 u32Opt, TWESTG_tsFinal *psFinal) {
	uint8 u8sect = s_u8GetFirstSect(u8kind, u8slot);
	if (u8sect != 0xFF) {
		bool_t bRes = TWENVM_bRead(pBuf, u8sect); //先頭セクターはコントロールブロックとして残し、2セクター単位で保存
		return bRes ? TWE_APIRET_SUCCESS : TWE_APIRET_FAIL;
	} else return TWE_APIRET_FAIL;
}

/*!
 * 諸処理を行うコールバック。
 * 主としてインタラクティブモードから呼び出されるが、一部は他より呼び出される。
 *
 * \param pContext インタラクティブモードのコンテキスト(NULLの場合はインタラクティブモード以外からの呼び出し)
 * \param u32Op    コマンド番号
 * \param u32Arg1  引数１（役割はコマンド依存）
 * \param u32Arg2  引数２（役割はコマンド依存）
 * \param vpArg    引数３（役割はコマンド依存、主としてデータを戻す目的で利用する）
 * \return コマンド依存の定義。TWE_APIRET_FAILの時は何らかの失敗。
 */
TWE_APIRET TWEINTRCT_cbu32GenericHandler(TWEINTRCT_tsContext *pContext, uint32 u32Op, uint32 u32Arg1, uint32 u32Arg2, void *vpArg) {
	uint32 u32ApiRet = TWE_APIRET_SUCCESS;

	switch (u32Op) {
	case E_TWEINTCT_OP_GET_APPINFO:
		if (vpArg != NULL) {
			tsTWEINTRCT_APPINFO *pInfo = (tsTWEINTRCT_APPINFO *)vpArg;

			// generate U32 code from MWPFX_CSTR.
			pInfo->u32appid =
					  ((uint32)((uint8)(MWPFX_CSTR[0])) << 24)
					| ((uint32)((uint8)(MWPFX_CSTR[1])) << 16)
					| ((uint32)((uint8)(MWPFX_CSTR[2])) << 8)
					| ((uint32)((uint8)(MWPFX_CSTR[3])) << 0)
					;
			pInfo->u32version = (((uint32)VERSION_MAIN << 16) | ((uint32)VERSION_SUB << 8) | ((uint32)VERSION_VAR));
			pInfo->u8confid = FW_CONF_ID();
			pInfo->u8langid = FW_CONF_LANG();
			pInfo->u8InteractiveCapability = TRUE;
		} else {
			u32ApiRet = TWE_APIRET_FAIL;
		}
		break;

	case E_TWEINTCT_MENU_EV_LOAD:
		gc_u8MenuMode = (uint8)u32Arg1;
		// メニューロード時の KIND/SLOT の決定。
		if (gc_u8MenuMode == MENU_CONFIG) {
			// 通常メニュー
			gc_u8AppKind = STGS_KIND_MAIN;
			gc_u8AppSlot = TWESTG_SLOT_DEFAULT;
			MWPFX(vAppLoadData)(gc_u8AppKind, gc_u8AppSlot, FALSE); // 設定を行う
			u32ApiRet = TWE_APIRET_SUCCESS_W_VALUE((uint32)gc_u8AppKind << 8 | gc_u8AppSlot);
		}
		break;

	case E_TWEINRCT_OP_UNHANDLED_CHAR: // 未処理文字列があった場合、呼び出される。
		break;

	case E_TWEINRCT_OP_RESET: // モジュールリセットを行う
		TWE_fprintf(&gc_sSer, "\r\n!INF RESET SYSTEM...");
		TWE_fflush(&gc_sSer);
		vAHI_SwReset();
		break;

	case E_TWEINRCT_OP_REVERT: // 設定をもとに戻す。ただしセーブはしない。
		MWPFX(vAppLoadData)(gc_u8AppKind, gc_u8AppSlot, u32Arg1);
		break;

	case E_TWEINRCT_OP_CHANGE_KIND_SLOT:
		// KIND/SLOT の切り替えを行う。切り替え後 pContext->psFinal は、再ロードされること。
		// u32Arg1,2 0xFF: no set, 0xFF00: -1, 0x0100: +1, 0x00?? Direct Set

		// 本コードでは MENU_CONFIG 以外のページは無いので、この頁のときのみ設定する。
		if (gc_u8MenuMode == MENU_CONFIG) {
			// 0, 0 決め打ち
			gc_u8AppKind = STGS_KIND_MAIN;
			gc_u8AppSlot = TWESTG_SLOT_DEFAULT;

			// データの再ロード（同じ設定でも再ロードするのが非効率だが・・・）
			MWPFX(vAppLoadData)(gc_u8AppKind, gc_u8AppSlot, FALSE); // 設定を行う

			// 値を戻す。
			// ここでは設定の失敗は実装せず、SUCCESS としている。
			// VALUE は現在の KIND と SLOT。
			u32ApiRet = TWE_APIRET_SUCCESS_W_VALUE((uint16)gc_u8AppKind << 8 | gc_u8AppSlot);
		} else {
			u32ApiRet = TWE_APIRET_FAIL_W_VALUE((uint16)gc_u8AppKind << 8 | gc_u8AppSlot);
		}
		break;

	case E_TWEINRCT_OP_WAIT: // 一定時間待つ（ポーリング処理）
		TWESYSUTL_vWaitPoll(u32Arg1);
		break;

	case E_TWEINRCT_OP_GET_APPNAME: // CONFIG行, アプリ名
		if (vpArg != NULL) {
			// &(char *)vpArg: には、バッファ16bytesのアドレスが格納されているため strcpy() でバッファをコピーしてもよいし、
			// 別の固定文字列へのポインタに書き換えてもかまわない。
			*((uint8**)vpArg) = (uint8*)"APP_IO_CX2";
		}
		break;

	case E_TWEINRCT_OP_GET_KINDNAME: // CONFIG行, KIND種別名
		if (vpArg != NULL) {
			// &(char *)vpArg: には、バッファ16bytesのアドレスが格納されているため strcpy() でバッファをコピーしてもよいし、
			// 別の固定文字列へのポインタに書き換えてもかまわない。

			// このスコープ内に const uint8 SetList_names[][8] = { .. }; としても、うまくいかない。理由不明。
			*((uint8**)vpArg) = (uint8*)SetList_names;
		}
		break;

	case E_TWEINTCT_OP_GET_OPTMSG:
		if (vpArg != NULL) {
			// &(char *)vpArg: には、バッファ32bytesのアドレスが格納されているため strcpy() でバッファをコピーしてもよいし、
			// 別の固定文字列へのポインタに書き換えてもかまわない。

			// このコードは -Os 最適化では不具合を起こす場合がある（case 節内にあるのが原因？）
			TWE_snprintf(*((char**)vpArg), 32, "v%d-%02d-%d/SID=%08X", VERSION_MAIN, VERSION_SUB, VERSION_VAR, ToCoNet_u32GetSerial() );
		}
		break;

	case E_TWEINTCT_OP_GET_SID: // シリアル番号
		if (vpArg != NULL) {
			// シリアル値を書き込む
			*((uint32*)vpArg) = ToCoNet_u32GetSerial();
		}
		break;

	default:
		break;
	}

	return u32ApiRet;
}


static TWE_APIRET TWESTGS_VLD_u32FPS(struct _TWESTG_sElement* pMe, TWESTG_tsDatum* psDatum, uint16 u16OpId, TWE_tsBuffer* pBuf){
	TWE_APIRET ret = TWE_APIRET_FAIL;
	TWESTG_tuDatum* pDatum = &psDatum->uDatum;

	if (u16OpId == TWESTGS_VLD_OP_VALIDATE && pBuf->u8bufflen == 0) {
		// 未入力の場合はデフォルトに戻す
		TWESTG_vSetUDatumFrUDatum(pMe->sDatum.u8Type, pDatum, &pMe->sDatum.uDatum);
		ret = TWE_APIRET_SUCCESS;
	} else 	if (u16OpId == TWESTGS_VLD_OP_VALIDATE) {
		uint32 u32num = 0;
		u32num = TWESTR_i32DecstrToNum(pBuf->pu8buff, pBuf->u8bufflen);
		ret = TWE_APIRET_SUCCESS;

		// 値の変換に成功したので値の範囲のチェックを行う
		if (TWE_APIRET_IS_SUCCESS(ret)) {
			ret = TWE_APIRET_FAIL; // 一旦FAILにして、チェックが成功したら TRUE に戻す
			if (pMe->sDatum.u8Type & 0x1) { // 符号なし
				switch(u32num){
					case 4: case 8: case 16: case 32:
						TWESTG_vSetUDatumFrU32(pMe->sDatum.u8Type, pDatum, u32num);
						ret = TWE_APIRET_SUCCESS;
						break;
					default:
						ret = TWE_APIRET_FAIL;
						break;
				}
			}
			else { // 符号あり
				ret = TWE_APIRET_FAIL;
			}
		}
	}
	return ret;
}
