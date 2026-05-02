/* Copyright (C) 2017 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

#ifndef  CONFIG_H_INCLUDED
#define  CONFIG_H_INCLUDED

#if defined __cplusplus
extern "C" {
#endif

/****************************************************************************/
/***        Include Files                                                 ***/
/****************************************************************************/
#include <AppHardwareApi.h>
#include "app_prefix.h"


/****************************************************************************/
/***        FOR ALL IN ONE APP                                            ***/
/****************************************************************************/
/*
 * ファームのビルド定義
 */
# define IS_OPT_INTRCT_ON_BOOT() (0)
# define IS_CONF_NORMAL() (1)
# define FW_CONF_ID() (1)

# define USE_LANG_INTERACTIVE_DEFAULT 0 // 0,1:English 2:Japanese
# undef USE_LANG_INTERACTIVE  // 既存の定義を削除
# define USE_LANG_INTERACTIVE 0  // 英語に強制設定
# define FW_CONF_LANG() USE_LANG_INTERACTIVE

#define MW_NVMEM_FIRST_SECTOR 0
#define MW_NVMEM_ALLOCATED (1+2)
/****************************************************************************/
/***        Macro Definitions                                             ***/
/****************************************************************************/

/* アプリケーション名 */
#define APP_NAME "TWE APP_IO_CX2"

/* Serial Configuration */
#define UART_BAUD			115200UL //!< UART のボーレート（デフォルト）
#define UART_BAUD_SAFE		38400UL //!< UART のボーレート（他の設定）
#define UART_PARITY_ENABLE	E_AHI_UART_PARITY_DISABLE //!< パリティは none
#define UART_PARITY_TYPE 	E_AHI_UART_EVEN_PARITY //!< E_AHI_UART_PARITY_ENABLE 時の指定
#define UART_STOPBITS 		E_AHI_UART_1_STOP_BIT //!< ストップビットは１

/* Specify which serial port to use when outputting debug information */
#define UART_PORT_MASTER    E_AHI_UART_0 //!< UARTポートの指定

/* Specify the PAN ID and CHANNEL to be used by tags, readers and gateway */
#define APP_ID              0x67720107 //!< アプリケーションID。同じIDでないと通信しない。

// チャネル設定は config_private.h から取得
#include "config_private.h"

// 後方互換性のための定義（config_private.h から取得）
#define CHANNEL CHANNEL_DEFAULT  //!< 使用するチャネル（config_private.h の CHANNEL_DEFAULT を使用）
// CHMASKはconfig_private.hで定義されているため、ここでは定義しない

// Coordinator specific settings
#define PARENT_ADDR     	0x8001

// スリープ間隔
#define MODE4_SLEEP_DUR_ms 1000UL
#define MODE7_SLEEP_DUR_ms 0UL

/* twesettings 関連 */
#define STGS_KIND_MAIN 0x00		//! 親機用設定
#define STGS_KIND_SLOT_MAX 1 	//! 親機は SLOT0 のみ
#define STGS_KIND_MAX 1			//! 種別数の最大

#define APPVER				((VERSION_MAIN << 24) | (VERSION_SUB << 16) | (VERSION_VAR << 8))

/**
 * ボタン押し下げ時連続送信モード
 *
 * - DI=G 検知時に連続送信し、DI=H になった後も１秒間(ON_PRESS_TRANSMIT_KEEP_TX_ms)継続送信する。
 * - 受信側は、500ms(ON_PRESS_TRANSMIT_RESET_ms) 無線電波を受信しなかった場合、DO=H に戻す
 */
#define ON_PRESS_TRANSMIT

/**
 * ボタン押し下げ時連続送信モード時の、無受信時に DO=H に戻すまでの時間[ms]
 */
#define ON_PRESS_TRANSMIT_RESET_ms 500

/**
 * ボタン押し下げ時連続送信モード時の、DI=H に戻った後に継続送信する時間 [ms]
 */
#define ON_PRESS_TRANSMIT_KEEP_TX_ms 1000

/**
 * 低レイテンシモード時で割り込みタイミングから、再度送信する
 */
#define LOW_LATENCY_DELAYED_TRANSMIT_COUNTER 7

/**
 * リモコンモードで長押しがリリースされてからの再送回数
 */
#define REMOTE_MODE_ADDITIONAL_TX_COUNT 3

/**
 * 評価キット 002_L 使用時
 */
#ifdef USE_DEV_KIT_002_L
# warning "USE_DEV_KIT_002_L is defined... Undef this for official release."
#endif

/****************************************************************************/
/***        Type Definitions                                              ***/
/****************************************************************************/
/**
 * 追加設定の列挙体
 **/
typedef enum{
	E_TWESTG_DEFSETS_APPSTART = 0x80,
	E_TWESTG_DEFSETS_SLEEP4,
	E_TWESTG_DEFSETS_SLEEP7,
	E_TWESTG_DEFSETS_FPS,
	E_TWESTG_DEFSETS_HOLDPORT,
	E_TWESTG_DEFSETS_HOLDTIME,
	E_TWESTG_DEFSETS_ENCENABLE,
	E_TWESTG_DEFSETS_ENCKEY
} teTWESTG_APP_DEFSETS;

/****************************************************************************/
/***        Exported Functions                                            ***/
/****************************************************************************/

/****************************************************************************/
/***        Exported Variables                                            ***/
/****************************************************************************/

#if defined __cplusplus
}
#endif

#endif  /* CONFIG_H_INCLUDED */

/****************************************************************************/
/***        END OF FILE                                                   ***/
/****************************************************************************/
