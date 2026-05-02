/* Copyright (C) 2017 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE *
 * AGREEMENT).                                                    */

/** @file
 * アプリケーションのメイン処理
 *
 * @defgroup MASTER アプリケーションのメイン処理
 */

#ifndef  MASTER_H_INCLUDED
#define  MASTER_H_INCLUDED

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include <jendefs.h>
#include <AppHardwareApi.h>

#ifdef MWLIB_MULTINONE
#include "../../TWELITE_Apps/MultInOne.h"
#endif

#include "app_prefix.h"

#include "twenet_defs.h"

#include "btnMgr.h"

// twesettings
#include "twecommon.h"
#include "tweserial.h"
#include "tweserial_jen.h"
#include "tweprintf.h"
#include "twesettings.h"
#include "tweutils.h"
#include "twesercmd_gen.h"
#include "tweinteractive.h"
#include "twesysutils.h"
#include "twesettings_cmd.h"

// Application Headers
#include "common.h"
#include "app_save.h"

#include "Interactive.h"
#include "duplicate_checker.h"

#include "Pairing.h"

/** @ingroup MASTER
 * ネットワークのモード列挙体 (ショートアドレス管理か中継ネットワーク層を使用したものか)
 */
typedef enum {
	E_NWKMODE_MAC_DIRECT,//!< ネットワークは構成せず、ショートアドレスでの通信を行う
	E_NWKMODE_LAYERTREE  //!< 中継ネットワークでの通信を行う(これは使用しない)
} teNwkMode;

/** @ingroup MASTER
 * IO の状態
 */
typedef struct {
	uint32 u32BtmBitmap; //!< 現在のビットの状況 (0xFFFFFFFF: 未確定)
	uint32 u32BtmUsed; //!< 利用対象ピンかどうか (0xFFFFFFFF: 未確定)
	uint32 u32BtmChanged; //!< 変更があったポートまたは割り込み対象ピン (0xFFFFFFFF: 未確定)

	uint8 au8Input[MAX_IO_TBL]; //!< 入力ポート (0: Hi, 1: Lo, 0xFF: 未確定)
	uint8 au8Output[MAX_IO_TBL]; //!< 出力ポート (0: Hi, 1:Lo, 0xFF: 未確定)

	uint32 u32TxLastTick; //!< 最後に送った時刻
	uint32 u32RxLastTick; //!< 最後に受信した時刻

	int16 i16TxCbId; //!< 送信時のID
} tsIOData;

/** @ingroup MASTER
 * IO 設定要求
 */
typedef struct {
	uint16 u16IOports;          //!< 出力IOの状態 (1=Lo, 0=Hi)
	uint16 u16IOports_use_mask; //!< 設定を行うポートなら TRU
} tsIOSetReq;

/** @ingroup MASTER
 * アプリケーションの情報
 */
typedef struct {
	// ToCoNet
	uint32 u32ToCoNetVersion; //!< ToCoNet のバージョン番号を保持
	uint16 u16ToCoNetTickDelta_ms; //!< ToCoNet の Tick 周期 [ms]
	uint8 u8AppIdentifier; //!< AppID から自動決定

	// メインアプリケーション処理部
	void *prPrsEv; //!< vProcessEvCoreSlpまたはvProcessEvCorePwrなどの処理部へのポインタ

	// DEBUG
	uint8 u8DebugLevel; //!< デバッグ出力のレベル

	// Wakeup
	bool_t bWakeupByButton; //!< TRUE なら起床時に DI 割り込みにより起床した
	uint32 u32SleepDur; //!< スリープ間隔 [ms]

	// mode3 fps
	uint8 u8FpsBitMask; //!< mode=3 連続送信時の秒間送信タイミングを判定するためのビットマスク (64fps のカウンタと AND を取って判定)

	// Network mode
	teNwkMode eNwkMode; //!< ネットワークモデル(未使用：将来のための拡張用)
	uint8 u8AppLogicalId; //!< ネットワーク時の抽象アドレス 0:親機 1~:子機, 0xFF:通信しない

	// Network context
	tsToCoNet_Nwk_Context *pContextNwk; //!< ネットワークコンテキスト(未使用)
	tsToCoNet_NwkLyTr_Config sNwkLayerTreeConfig; //!< LayerTree の設定情報(未使用)

	// Flash Information
	tsFlashApp sAppStgs;     //!< 設定情報を保存する。
	int8 bFlashLoaded; //!< フラッシュからの読み込みが正しく行われた場合は TRUE

	uint32 u32DIO_startup; //!< 電源投入時のIO状態

	// config mode
	uint8 u8Mode; //!< 動作モード(IO M1,M2,M3 から設定される)
	uint8 u8ChCfg; //!< チャネル設定(EI1,EI2 から設定される)
	uint8 u8IoTbl; //!< IO割り当てテーブル番号

	// button manager
	tsBTM_Config sBTM_Config; //!< ボタン入力（連照により状態確定する）管理構造体
	PR_BTM_HANDLER pr_BTM_handler; //!< ボタン入力用のイベントハンドラ (TickTimer 起点で呼び出す)
	uint32 u32BTM_Tick_LastChange; //!< ボタン入力で最後に変化が有ったタイムスタンプ (操作の無効期間を作る様な場合に使用する)

	uint16 au16HoldBtn[MAX_IO_TBL]; //!< ボタンの入力を一定時間維持する
	uint32 u32BtnMask_Special; //!< ボタンの入力に対する特殊設定に対応するマスク

	// latest state
	tsIOData sIOData_now; //!< 現時点での IO 情報
	tsIOData sIOData_reserve; //!< 保存された状態(0がデフォルトとする)
	uint8 u8IOFixState; //!< IOの読み取り確定ビット

	// Counter
	uint32 u32CtTimer0; //!< 64fps カウンタ。スリープ後も維持
	uint16 u16CtTimer0; //!< 64fps カウンタ。起動時に 0 クリアする
	uint16 u16CtRndCt; //!< 起動時の送信タイミングにランダムのブレを作る

	uint8 u8UartReqNum; //!< UART の要求番号

	uint16 u16TxFrame; //!< 送信フレーム数
	uint8 u8SerMsg_RequestNumber; //!< シリアルメッセージの要求番号

	bool_t bCustomDefaults; //!< カスタムデフォルトがロードされたかどうか

	uint8 u8RxSetting; //!< bit0: 起動時 bit1: 常時

	uint8 u8StandardTxRetry; //!< デフォルトの再送設定
	uint8 u8StandardTxAckRetry; //!< デフォルトの再送設定(ACKモード時)
} tsAppData;

#ifndef MWLIB_MULTINONE
extern tsAppData sAppData; //!< アプリケーションデータ  @ingroup MASTER
#else
#define sAppData (*(tsAppData *)_MULTINONE_PS_APP_DATA) //!< アプリケーションデータ  @ingroup MASTER
#endif

/** @ingroup MASTER
 * アプリケーション制御データ (static 定義していたものを移動)
 */

typedef struct _sAppDataExt {
	uint8 au8SerialTxBuffer[512]; //!< シリアル出力バッファ
	uint8 au8SerialRxBuffer[512]; //!< シリアル入力バッファ

	uint32 u32WakeBtnStatus; //!< 起床時のボタンの状態 @ingroup MASTER
	uint16 u16OnPrsTx_Count; //!< リモコンモードにおける余剰送信回数のカウント @ingroup MASTER
	bool_t bChatteringSleep; //!< TRUEなら、チャタリング抑制を目的としたスリープを実施した @ingroup MASTER

	TWE_tsFILE sSerStream;
	TWESTG_tsFinal sFinal;
	TWEINTRCT_tsContext* psIntr;

	TWESERCMD_tsSerCmd_Context sSerCmdIn; //!< シリアル入力用
	TWESERCMD_tsSerCmd_Context sSerCmdOut; //!< シリアル出力

	//tsModbusCmd sSerCmdIn; //!< シリアル入力系列のパーサー (modbus もどき)  @ingroup MASTER
	//tsInpStr_Context sSerInpStr; //!< 文字列入力  @ingroup MASTER

	tsTimerContext sTimerApp; //!< タイマー管理構造体  @ingroup MASTER

	uint8 au8SerInBuff[128]; //!< シリアルの入力書式のための暫定バッファ  @ingroup MASTER
	uint8 au8SerOutBuff[256]; //!< シリアルの出力書式のための暫定バッファ  @ingroup MASTER

	tsDupChk_Context sDupChk_IoData; //!< 重複チェック(IO関連のデータ転送)  @ingroup MASTER
	tsDupChk_Context sDupChk_SerMsg; //!< 重複チェック(シリアル関連のデータ転送)  @ingroup MASTER

	struct {
		uint32 u32LastQuick;
		uint16 u16CountMax;
		uint8 u8cond;
		bool_t bLowLatencyTxCond; // 判定回数を減らすため、このフラグを利用する (初期値 0xFF)
		bool_t bBurstActive; // H->L割り込み起点の1秒連射が有効中
		uint8 u8BurstRemain; // 連射の残回数
		uint16 u16BurstLatchedBm; // 連射中に固定送信するDI状態
		uint16 u16BurstLatchedUsed; // 連射中に固定送信するDI使用マスク
		uint32 u32IdleLastTxTick; // 非連射モードの最終送信時刻
		uint32 u32BurstIrqPortMask; // 連射開始を許可するDIポートマスク
	} In_App_IO_C;

	struct {
		const tsCbHandlerPair* psCbHandlerPair;
		void *pvProcessEvPair;

		tsPairingConf sPairingConf;		//!< ペアリングの設定パラメタ構造体


		tsPairingInfo sPairingInfo;		//!< ペアリング結果を保存する構造体

		uint8 au8Data[64];				//!< 汎用データ領域

		uint8 u8Port; //= 0xFF;				//!< ペアリングしているときのステータスを見るためのDIOポート番号 0x80,0x81:DO0,1を使用する。 0x00-0x14:DIO0-18(20)を使用する。 0xFF:使用しない
		bool_t bDOFlag;  //= FALSE;			//!< DOを使用するかどうかのフラグ
		uint8 u8RemoteConf; //= 0xFF;		//!< どんなパケットを送信したかを判別するためのフラグ
		uint8 u8Flags; //= 0xFF;			//!< 何を受信したかを判別するためのフラグ
		uint32 u32RcvAddr; //= 0xFFFFFFFF;	//!< 受信した相手のアドレス

		bool_t bLEDBlink;// = TRUE;			//!< 点滅フラグ

		uint8 u8CountSWC;					//!< 送信回数 E_STATE_WAIT_COMMAND
	} In_Paring_C;

	struct {
		uint8 au8PortTbl_DIn[MAX_IO_TBL]; //!< DI のポート番号のテーブル
		uint8 au8PortTbl_DOut[MAX_IO_TBL]; //!< DO のポート番号のテーブル
		uint32 u32_PORT_INPUT_MASK; //!< 入力ポートのマスク @ingroup MASTER
		uint32 u32_PORT_OUTPUT_MASK; //!< 出力ポートのマスク @ingroup MASTER
		uint8 u8_PORT_INPUT_COUNT;  //!< @ingroup MASTER IO入力数
		uint8 u8_PORT_OUTPUT_COUNT; //!< @ingroup MASTER IO出力数
	} In_common_C;

	struct {
		uint8 u8AppKind; //!< 種別 STGS_KIND_MAIN(0x00)
		uint8 u8AppSlot; //!< 現在のスロット
		uint8 u8MenuMode; //!< メニューモード (MSB が１はインタラクティブモード終了)
	} In_Interactive_C;
} tsAppDataExt;

#ifndef MWLIB_MULTINONE
extern tsAppDataExt sAppDataExt; //!< アプリケーションデータ  @ingroup MASTER
#else
#define sAppDataExt (*(tsAppDataExt *)_MULTINONE_PS_APP_DATAEXT) //!< アプリケーションデータ  @ingroup MASTER
#endif

#define DATAEXT(X) (sAppDataExt.X)
#define gc_sSer (sAppDataExt.sSerStream)
#define gc_sFinal DATAEXT(sFinal)
#define gc_u32WakeBtnStatus DATAEXT(u32WakeBtnStatus)
#define gc_u16OnPrsTx_Count DATAEXT(u16OnPrsTx_Count)
#define gc_bChatteringSleep DATAEXT(bChatteringSleep)
#define gc_sSerCmdIn DATAEXT(sSerCmdIn)
#define gc_sSerCmdOut DATAEXT(sSerCmdOut)
#define gc_sTimerApp DATAEXT(sTimerApp)
#define gc_au8SerInBuff DATAEXT(au8SerInBuff)
#define gc_au8SerOutBuff DATAEXT(au8SerOutBuff)
#define gc_sDupChk_IoData DATAEXT(sDupChk_IoData)
#define gc_sDupChk_SerMsg DATAEXT(sDupChk_SerMsg)

#define gc_bLowLatencyTxCond DATAEXT(In_App_IO_C.bLowLatencyTxCond)

#define gc_psCbHandlerPair DATAEXT(In_Paring_C.psCbHandlerPair)
#define gc_pvProcessEvPair DATAEXT(In_Paring_C.pvProcessEvPair)

#define gc_au8PortTbl_DIn DATAEXT(In_common_C.au8PortTbl_DIn)
#define gc_au8PortTbl_DOut DATAEXT(In_common_C.au8PortTbl_DOut)
#define gc_u32_PORT_INPUT_MASK DATAEXT(In_common_C.u32_PORT_INPUT_MASK)
#define gc_u32_PORT_OUTPUT_MASK DATAEXT(In_common_C.u32_PORT_OUTPUT_MASK)
#define gc_u8_PORT_INPUT_COUNT DATAEXT(In_common_C.u8_PORT_INPUT_COUNT)
#define gc_u8_PORT_OUTPUT_COUNT DATAEXT(In_common_C.u8_PORT_OUTPUT_COUNT)

#define gc_u8AppKind DATAEXT(In_Interactive_C.u8AppKind)
#define gc_u8AppSlot DATAEXT(In_Interactive_C.u8AppSlot)
#define gc_u8MenuMode DATAEXT(In_Interactive_C.u8MenuMode)

/**
 * 内部構造体にコピーした設定情報へのアクセスマクロ
 */
#define sAppStg (sAppData.sAppStgs)

/****************************************************************************
 * 内部処理
 ***************************************************************************/
#define E_IO_FIX_STATE_NOT_READY 0x0
#define E_IO_FIX_STATE_READY 0x1

#define RX_STATE_BOOT_ON 0x01
#define RX_STATE_CONTINUOUS 0x02

#define HZ_LOW_LATENCY 1000 //!< 低レイテンシー時の制御周期 [Hz]

#endif  /* MASTER_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
