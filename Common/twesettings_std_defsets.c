/* Copyright (C) 2019-2020 Mono Wireless Inc. All Rights Reserved.
 *
 * The twesettings library is dual-licensed under MW-SLA and MW-OSSLA terms.
 * - MW-SLA-1J,1E or later (MONO WIRELESS SOFTWARE LICENSE AGREEMENT).
 * - MW-OSSLA-1J,1E or later (MONO WIRELESS OPEN SOURCE SOFTWARE LICENSE AGREEMENT). 
 * 
 * このファイルは標準ライブラリからコピーされ、ローカルでカスタマイズ可能にしたものです。
 * デフォルト値の変更は、このファイル内の該当箇所を編集してください。
 */

#include "twecommon.h"
#include "twesettings0.h"
#include "twesettings_std.h"
#include "twesettings_std_defsets.h"
#include "twesettings_validator.h"
#include "tweinputstring.h"
#include "tweutils.h"
#include "config_private.h"

/*!
 * 共通定義
 * 基本設定を追加する。
 */
const TWESTG_tsElement TWESTG_DEFSETS_BASE[] = {
	{ E_TWESTG_DEFSETS_APPID,  // アプリケーションID
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = CONFIG_DEFAULT_APPID }}, // 32bit (config_private.h から取得)
		{ "AID",
		  "Application ID [HEX:32bit]",
		  "To separate the network logically, All devices must have the same value."
		  " (leading or trailing 0000/FFFF are invalid)" },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, 'a' },
		{ {.u32 = 0}, {.u32 = 0}, TWESTGS_VLD_u32AppId, NULL },
	},
	{ E_TWESTG_DEFSETS_LOGICALID,
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = CONFIG_DEFAULT_LOGICALID }},
		{ "LID",
		  "Device ID [1-100,etc]",
		  "Set if more than one child unit needs to be identified." },
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 3, 'i' },
		{ {.u8 = 0}, {.u8 = 255}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_CHANNEL,
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = CONFIG_DEFAULT_CHANNEL }},
		{ "CHN",
		  "Channel [11-26]",
		  "To separate the network physically, All devices must have the same value."
		},
		{ E_TWEINPUTSTRING_DATATYPE_DEC, 2, 'c' },
		{ {.u8 = 11}, {.u8 = 26}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_CHANNELS_3,
		{ TWESTG_DATATYPE_UINT16, sizeof(uint16), 0, 0, {.u16 = CONFIG_DEFAULT_CHANNELS_3 }},
		{ "CHL", "Channel(s)",
		  "To separate the network physically, All devices must have the same value.\\n"
		  "When up to 3 channels are specified, Channel agility function will be enabled."
		},
		{ E_TWEINPUTSTRING_DATATYPE_CUSTOM_DISP_MASK | E_TWEINPUTSTRING_DATATYPE_STRING, 8, 'c' },
		{ {.u16 = 0}, {.u16 = 0xFFFF}, TWESTGS_VLD_u32ChList, NULL },
	},
	{ E_TWESTG_DEFSETS_POWER_N_RETRY,
		// デフォルト値: CONFIG_DEFAULT_POWER_N_RETRY (再送: 9回=最大, 出力: レベル3=最大)
		// XY形式: X=再送回数(0=デフォルト2回, 1-9=回数, F=無効), Y=出力レベル(3=最大, 2, 1, 0=最小)
		{ TWESTG_DATATYPE_UINT8,  sizeof(uint8),  0, 0, {.u8 = CONFIG_DEFAULT_POWER_N_RETRY }},
		{ "PWR", "RF Power/Retransmissions [HEX:8bit]",
		  "Enter two XY digits."
		  "\\nX: Number of retransmissions [0: default 2 times / 1-9: times, F: disabled]"
		  "\\nY: Transmission power level [3: strongest / 2 / 1 / 0: weakest]"
		  "For example, 42 results in 4 retransmissions and level 2 output power."
		},
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 2, 'x' },
		{ {.u8 = 0}, {.u8 = 0xFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{ E_TWESTG_DEFSETS_UARTBAUD,
		// デフォルト値: CONFIG_DEFAULT_UARTBAUD (115200 bps)
		// 値は100で割った値で格納される (1152 = 115200 / 100)
		{ TWESTG_DATATYPE_UINT16, sizeof(uint16), 0, 0, {.u16 = CONFIG_DEFAULT_UARTBAUD }},
		{ "UOP", "UART Baud Alt. [XXXXX]",
		  "Baudrate when the BPS pin or the option bit was set."
		  "\\n- XXXXX is the baud rate [9600,19200,38400,57600,115200,230400: bps]. "
		  "\\n If you use other baud rates, you need to check the waveform in advance."
		},
		{ E_TWEINPUTSTRING_DATATYPE_CUSTOM_DISP_MASK | E_TWEINPUTSTRING_DATATYPE_STRING, 10, 'b' },
		{ {.u16 = 0}, {.u16 = 0}, TWESTGS_VLD_u32UartBaudOpt, NULL },
	},
	{ E_TWESTG_DEFSETS_OPTBITS,
		{ TWESTG_DATATYPE_UINT32, sizeof(uint32), 0, 0, {.u32 = CONFIG_DEFAULT_OPTBITS }}, // デフォルト: config_private.h から取得
		{ "OPT", "Option Bits [HEX:32bit]", "You can activate the settings associated with each bit." },
		{ E_TWEINPUTSTRING_DATATYPE_HEX, 8, 'o' },
		{ {.u32 = 0}, {.u32 = 0xFFFFFFFF}, TWESTGS_VLD_u32MinMax, NULL },
	},
	{E_TWESTG_DEFSETS_VOID} // FINAL DATA
};

const TWESTG_tsMsgReplace TWESTG_DEFSETS_BASE_MSG_JP[] = {
    {
		E_TWESTG_DEFSETS_APPID,
		"ｱﾌﾟﾘｹｰｼｮﾝID [HEX:32bit]",
		"論理的にﾈｯﾄﾜｰｸを分離します.通信を行う端末に同一の値を設定します. (先頭や末尾が0000/FFFFの値は無効)"
    },
	{
		E_TWESTG_DEFSETS_LOGICALID,
		"論理ﾃﾞﾊﾞｲｽID [1-100,etc]",
		"複数の子機を識別する必要がある場合に設定します."
	},
	{
		E_TWESTG_DEFSETS_CHANNEL,
		"周波数ﾁｬﾈﾙ [11-26]",
		"物理的にﾈｯﾄﾜｰｸを分離します.通信を行う端末に同一の値を設定します."
	},
	{
		E_TWESTG_DEFSETS_CHANNELS_3,
		"周波数ﾁｬﾈﾙ(複数可) [11-26]",
		"物理的にﾈｯﾄﾜｰｸを分離します.通信を行う端末に同一の値を設定します.\\n"
		"最大3ﾁｬﾈﾙを指定することで、ﾁｬﾈﾙｱｼﾞﾘﾃｨを有効化できます."
	},
	{
		E_TWESTG_DEFSETS_POWER_N_RETRY,
		"送信出力と再送回数 [HEX:8bit]",
		"電波の送信出力とﾊﾟｹｯﾄを追加で送信する回数を指定します."
		"XYの２桁で指定します."
		"\\nX: 追加送信の回数 [0: ﾃﾞﾌｫﾙﾄ2回 / 1-9: 回数 / F: 無効]"
		"\\nY: 無線の出力レベル [3: 最大 / 2 / 1 / 0: 最小]"
	},
	{
		E_TWESTG_DEFSETS_UARTBAUD,
		"UART代替ﾎﾞｰﾚｰﾄ [XXXXX]",
		"BPSﾋﾟﾝまたは指定のｵﾌﾟｼｮﾝﾋﾞｯﾄを適用した場合のボーレートを設定します."
		"\\n- XXXXX はﾎﾞｰﾚｰﾄです[9600,19200,38400,57600,115200,230400]."
		"\\n  その他のﾎﾞｰﾚｰﾄは事前に波形を確認してください."
	},
	{
		E_TWESTG_DEFSETS_OPTBITS,
		"ｵﾌﾟｼｮﾝﾋﾞｯﾄ [HEX:32bit]",
		"各ﾋﾞｯﾄに紐付いた設定を利用できます."
	},
	{ E_TWESTG_DEFSETS_VOID }
};

/*!
 * カスタムデフォルト対応スロット数
 */
const uint8 TWESTG_DEFCUST_U8_SLOT_NUMBER = 8; // 0...8 までの配列

/*!
 * カスタムデフォルト、スロット番号ごとに LID を設定する
 *
 * const uint8 XXX[][各スロットで格納される最大バイト] = {
 *    { SLOT定義のデータ長, ID, データ型, データ, ID, データ型, データ, ... },  // SLOT定義0
 *    ...
 *    { SLOT定義のデータ長, ID, データ型, データ, ID, データ型, データ, ... }}; // SLOT定義N
 */
const uint8 TWESTG_DEFCUST_SLOT[][4] = {
	{ 1, E_TWESTG_DEFSETS_VOID },                                           // 0 はダミー
	{ 3, E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 2 }, // SLOT1
	{ 3, E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 3 }, // SLOT2
	{ 3, E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 4 }, // ...
	{ 3, E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 5 },
	{ 3, E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 6 },
	{ 3, E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 7 },
	{ 3, E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8 << 4) | 1, 8 },
};

/*!
 * E_TWESTG_DEFSETS_CHANNELS_3（チャネル設定、最大３チャネル）を削除する
 */
const uint8 TWESTG_DEFCUST_REMOVE_CHAN3[] = {
	2,   // 総バイト数(このバイトは含まない。手計算で間違えないように入力！)
	E_TWESTG_DEFSETS_CHANNELS_3, TWESTG_DATATYPE_UNUSE
};

/*!
 * E_TWESTG_DEFSETS_CHANNEL (チャネル設定：１チャンネルのみ）を削除する
 */
const uint8 TWESTG_DEFCUST_REMOVE_CHAN1[] = {
	2,   // 総バイト数(このバイトは含まない。手計算で間違えないように入力！)
	E_TWESTG_DEFSETS_CHANNEL, TWESTG_DATATYPE_UNUSE
};
