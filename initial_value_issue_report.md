# 初期値が反映されない問題の調査レポート

## 問題の概要

ビルド＆書き込み後に表示された初期値が、`config_private.h`で設定した値と一致していません。

### 表示された初期値
- `a: (0x67720107)` - App ID（期待値: `0xA3F7C2E9`）
- `c: (16)` - チャネル（期待値: `24`）
- `x: (0x03)` - 送信出力と再送回数（期待値: `0x93`）
- `o: (0x00000000)` - オプションビット（期待値: `0x420000`）

### `config_private.h`の設定値
- `CONFIG_DEFAULT_APPID`: `0xA3F7C2E9UL`
- `CONFIG_DEFAULT_CHANNEL`: `24`
- `CONFIG_DEFAULT_POWER_N_RETRY`: `0x93`
- `CONFIG_DEFAULT_OPTBITS`: `0x420000UL`

## 原因

### 1. `Interactive.c`の`au8CustomDefault_Base`配列にハードコードされた値

**ファイル**: `Common/Interactive.c:61-67`

```c
static const uint8 au8CustomDefault_Base[] = {
	19,   // 総バイト数
	E_TWESTG_DEFSETS_APPID, (TWESTG_DATATYPE_UINT32 << 4) | 4, 
	(APP_ID>>24)&0xFF,(APP_ID>>16)&0xFF,(APP_ID>>8)&0xFF,APP_ID&0xFF, // 6bytes
	E_TWESTG_DEFSETS_CHANNELS_3 , (TWESTG_DATATYPE_UINT16 << 4) | 2, 0x00, 0x20, // 4bytes
	E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8<<4)| 1, 0,	// 3bytes
	E_TWESTG_DEFSETS_OPTBITS, (TWESTG_DATATYPE_UINT32<<4) | 4, 0, 0, 0, 0,	// 6bytes
};
```

**問題点**:
1. **App ID**: `APP_ID`マクロ（`0x67720107`）が直接使用されている
   - `CONFIG_DEFAULT_APPID`（`0xA3F7C2E9UL`）が使用されていない

2. **チャネル**: `0x00, 0x20`がハードコードされている
   - これは`((1UL << 16) >> 11)` = チャネル16を意味する
   - `CONFIG_DEFAULT_CHANNELS_3`が使用されていない

3. **オプションビット**: `0, 0, 0, 0`（`0x00000000`）がハードコードされている
   - `CONFIG_DEFAULT_OPTBITS`（`0x420000UL`）が使用されていない

### 2. `POWER_N_RETRY`が`0x03`になっている理由

`au8CustomDefault_Base`には`POWER_N_RETRY`が含まれていないため、`twesettings_std_defsets.c`のデフォルト値が使用されるはずです。しかし、`0x03`が表示されているということは：

1. `twesettings_std_defsets.c`で`CONFIG_DEFAULT_POWER_N_RETRY`が正しく定義されていない可能性
2. または、別の場所で上書きされている可能性
3. または、フラッシュメモリに古い値が残っている可能性

## 解決策

### 修正が必要なファイル: `Common/Interactive.c`

`au8CustomDefault_Base`配列を修正して、`config_private.h`の設定値を使用するように変更する必要があります。

#### 修正内容

1. **`config_private.h`をインクルード**（既に`config.h`経由でインクルードされているが、直接インクルードを確認）

2. **`au8CustomDefault_Base`配列の修正**:
   - App ID: `APP_ID` → `CONFIG_DEFAULT_APPID`
   - チャネル: `0x00, 0x20` → `CONFIG_DEFAULT_CHANNELS_3`から計算
   - オプションビット: `0, 0, 0, 0` → `CONFIG_DEFAULT_OPTBITS`から計算

#### 注意点

- `au8CustomDefault_Base`はバイト配列なので、値のエンディアンに注意が必要
- チャネル値は`CONFIG_DEFAULT_CHANNELS_3`（`uint16`）をバイト配列に変換する必要がある
- オプションビットは`CONFIG_DEFAULT_OPTBITS`（`uint32`）をバイト配列に変換する必要がある

### 修正例

```c
#include "config_private.h"  // 直接インクルードを追加（必要に応じて）

static const uint8 au8CustomDefault_Base[] = {
	19,   // 総バイト数
	E_TWESTG_DEFSETS_APPID, (TWESTG_DATATYPE_UINT32 << 4) | 4, 
	((CONFIG_DEFAULT_APPID>>24)&0xFF), ((CONFIG_DEFAULT_APPID>>16)&0xFF), 
	((CONFIG_DEFAULT_APPID>>8)&0xFF), (CONFIG_DEFAULT_APPID&0xFF), // 6bytes
	E_TWESTG_DEFSETS_CHANNELS_3 , (TWESTG_DATATYPE_UINT16 << 4) | 2, 
	((CONFIG_DEFAULT_CHANNELS_3>>8)&0xFF), (CONFIG_DEFAULT_CHANNELS_3&0xFF), // 4bytes
	E_TWESTG_DEFSETS_LOGICALID, (TWESTG_DATATYPE_UINT8<<4)| 1, CONFIG_DEFAULT_LOGICALID,	// 3bytes
	E_TWESTG_DEFSETS_OPTBITS, (TWESTG_DATATYPE_UINT32<<4) | 4, 
	((CONFIG_DEFAULT_OPTBITS>>24)&0xFF), ((CONFIG_DEFAULT_OPTBITS>>16)&0xFF),
	((CONFIG_DEFAULT_OPTBITS>>8)&0xFF), (CONFIG_DEFAULT_OPTBITS&0xFF),	// 6bytes
};
```

### `POWER_N_RETRY`について

`POWER_N_RETRY`が`0x03`になっている原因を特定するため、以下を確認：

1. `twesettings_std_defsets.c`で`CONFIG_DEFAULT_POWER_N_RETRY`が正しく使用されているか
2. フラッシュメモリに古い設定値が残っていないか
3. インタラクティブモードで設定をリセットする

## まとめ

- **主な原因**: `Interactive.c`の`au8CustomDefault_Base`配列にハードコードされた値が使用されている
- **解決方法**: `au8CustomDefault_Base`配列を修正して、`config_private.h`の設定値を使用するように変更
- **追加確認**: `POWER_N_RETRY`が`0x03`になっている原因を特定する必要がある

