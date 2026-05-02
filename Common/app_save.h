/* Copyright (C) 2017 Mono Wireless Inc. All Rights Reserved.    *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE   *
 * AGREEMENT).                                                   */

/** @file
 *
 * @defgroup FLASH FLASHメモリの読み書き関数群
 * FLASH への読み書き関数
 */

#ifndef APP_SAVE_H_
#define APP_SAVE_H_

#include <jendefs.h>

/** @ingroup FLASH
 * フラッシュ格納データ構造体
 */
typedef struct _tsFlashApp {
	uint32 u32appkey;		//!< フラッシュデータ判定用キー (デフォルトの APPID を利用)
	uint32 u32ver;			//!< ファームウェアバージョン

	uint32 u32appid;		//!< アプリケーションID
	uint32 u32chmask;		//!< 使用チャネルマスク（３つまで）
	uint8 u8id;				//!< 論理ＩＤ (子機 1～100まで指定)
	uint8 u8pow;			//!< 出力パワー (0-3)
	uint8 u8role;			//!< 未使用(将来のための拡張)
	uint8 u8layer;			//!< 未使用(将来のための拡張)

	uint16 u16SleepDur_ms;	//!< mode4 スリープ期間[ms]
	uint16 u16SleepDur_s; 	//!< mode7 スリープ期間[s]
	uint8 u8Fps;			//!< mode3 毎秒送信回数 (4,8,16,32)

	uint32 u32baud_safe;	//!< ボーレート
	uint8 u8parity;         //!< パリティ 0:none, 1:odd, 2:even

	uint32 u32Opt;			//!< 色々オプション

	uint32 u32HoldMask;		//!< Lo をホールドするIOのリスト
	uint16 u16HoldDur_ms;	//!< Lo をホールドする期間

	uint8 u8Crypt;          //!< 暗号化を有効化するかどうか (1:AES128)
	uint8 au8AesKey[33];    //!< AES の鍵
} tsFlashApp;


#endif /* APP_SAVE_H_ */
