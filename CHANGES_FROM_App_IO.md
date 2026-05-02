# APP_IO_CX2.c 修正内容ドキュメント

> **注意**: 本ドキュメントは、App_IOからの修正内容を詳細に記述したものです。  
> 仕様とコンパイル手順については、[README.md](README.md)を参照してください。

## 目次

本ドキュメントは、`APP_IO_CX2.c`（旧`App_IO.c`）に対して実施した修正内容をまとめたものです。専用プログラム化のため、以下の変更を行いました：

- [1. アプリケーション名の変更](#1-アプリケーション名の変更) - `App_IO` → `APP_IO_CX2`、すべての`App_IO.h`参照を`APP_IO_CX2.h`に変更
- [2. モード設定の簡素化](#2-モード設定の簡素化) - M1のみ使用、親機・連射子機のみ対応
- [3. 中継処理の削除](#3-中継処理の削除) - ROUTERモード関連、不要なモード処理の削除（ROUTER、CHILD、CHILD_SLP_1SEC、CHILD_SLP_10SEC）、M2, M3ピンによるモード設定の削除（PORT_CONF2, PORT_CONF3の使用を削除）
- [4. MULTINONE統合フレームワーク関連コードの削除](#4-multinone統合フレームワーク関連コードの削除) - `Common/config.h`からMULTINONE統合フレームワーク関連コードの削除
- [5. スリープモード処理の無効化](#5-スリープモード処理の無効化) - `vProcessEvCoreSlp`関数は残存、`__attribute__((unused))`を追加
- [6. 設定値の外部化（`config_private.h`の導入）](#6-設定値の外部化config_privatehの導入)
- [7. インタラクティブモードの言語設定を英語に変更](#7-インタラクティブモードの言語設定を英語に変更)
- [8. デフォルト設定値の一元管理（`twesettings_std_defsets.c`の追加）](#8-デフォルト設定値の一元管理twesettings_std_defsetscの追加)
- [9. 動作仕様](#9-動作仕様)
- [10. コンパイル・動作確認](#10-コンパイル動作確認)
- [11. 注意事項](#11-注意事項)
- [12. 関連ファイル](#12-関連ファイル)
- [13. 変更履歴](#13-変更履歴)
- [14. 参考情報](#14-参考情報)

---

## 1. アプリケーション名の変更

### 1.1 変更内容

アプリケーション名を`App_IO`から`APP_IO_CX2`に変更しました。これには以下の変更が含まれます：

- **ファイル名の変更**:
  - `App_IO.c` → `APP_IO_CX2.c`
  - `App_IO.h` → `APP_IO_CX2.h`
- **フォルダ名の変更**:
  - `App_IO/` → `APP_IO_CX2/`
- **すべての`App_IO.h`参照を`APP_IO_CX2.h`に変更**:
  - `Common/Pairing.c`
  - `Common/common.c`
  - `Common/Interactive.c`
  - `APP_IO_CX2/APP_IO_CX2.c`
- **設定値の変更**:
  - `APP_NAME`マクロの値: `"TWE APP_IO"` → `"TWE APP_IO_CX"`

### 1.2 修正箇所

#### ファイル名の変更

- `App_IO.ORG/App_IO.c` → `APP_IO_CX2/APP_IO_CX2.c`
- `App_IO.ORG/App_IO.h` → `APP_IO_CX2/APP_IO_CX2.h`

#### インクルードパスの変更

```c
// 変更前
#include "App_IO.h"

// 変更後
#include "APP_IO_CX2.h"
```

### 1.3 影響

- アプリケーション名が明確に識別できるようになりました
- オリジナルの`App_IO`との区別が明確になりました

---

## 2. モード設定の簡素化

### 2.1 変更内容

- **変更前**: M1, M2, M3の3ピンで8通りのモード設定
- **変更後**: M1のみで2通りのモード設定

### 2.2 対応モード

| M1（プルアップ想定） | モード | 説明 |
|----|--------|------|
| オープン（Hi） | `E_IO_MODE_CHILD_CONT_TX` | 子機 |
| GND（Lo） | `E_IO_MODE_PARNET` | 親機（App_IO データシートと同じ） |

### 2.3 削除されたモード

以下のモードは削除されました：

- `E_IO_MODE_CHILD` (通常子機)
- `E_IO_MODE_ROUTER` (中継機)
- `E_IO_MODE_CHILD_SLP_1SEC` (1秒スリープ子機)
- `E_IO_MODE_CHILD_SLP_10SEC` (10秒スリープ子機)

### 2.4 修正箇所

#### モード読み取り（APP_IO_CX2.c）

```c
// M1 のみ。bPortRead() は utils.h で Lo=真 として定義されているため、
// 「真→PARNET（親）、偽→CHILD（子）」はデータシートの M1=G / O と一致（メーカー App_IO_CX と同一）。
if (!f_warm_start) {
    vPortAsInput(PORT_CONF1);
    sAppData.u8Mode = bPortRead(PORT_CONF1) ? E_IO_MODE_PARNET : E_IO_MODE_CHILD_CONT_TX;
}
```

#### モード依存の初期値設定（APP_IO_CX2.c）

```c
// 各モード依存の初期値の設定など
switch(sAppData.u8Mode) {
case E_IO_MODE_PARNET:
    sAppData.u8AppLogicalId = LOGICAL_ID_PARENT;
    break;

case E_IO_MODE_CHILD_CONT_TX:
    sAppData.u8AppLogicalId = LOGICAL_ID_CHILDREN;
    break;

default: // 未定義機能なので、SILENT モードにする。
    sAppData.u8AppLogicalId = 255;
    sAppStg.u8role = E_APPCONF_ROLE_SILENT;
    break;
}
```

#### 状態遷移マシンの登録（APP_IO_CX2.c）

`E_IO_MODE_PARNET`と`E_IO_MODE_CHILD_CONT_TX`のみに対応するように簡素化。

---

## 3. 中継処理の削除

### 3.1 変更内容

中継機（ROUTER）モードが削除されたため、パケットの中継処理を削除しました。

### 3.2 修正箇所

#### 受信関数からの削除

以下の関数から中継処理を削除：

- `vReceiveIoData()` (APP_IO_CX2.c)

削除されたコード例：

```c
// 削除前
if (u8TxFlag == 0 && sAppData.u8Mode == E_IO_MODE_ROUTER) {
    // 中継処理
    i16TransmitRepeat(pRx);
}

// 削除後
(void)u8TxFlag; // 未使用変数の警告回避
```

#### 状態遷移からの削除

`vProcessEvCorePwr()`関数から`E_STATE_APP_RUNNING_ROUTER`への遷移を削除。

---

## 4. MULTINONE統合フレームワーク関連コードの削除

### 4.1 変更内容

複数のアプリケーションを1つのファームウェアに統合するためのMULTINONEフレームワーク関連コードを削除しました。本アプリケーションは単一アプリケーションとして動作するため、これらのコードは不要です。

### 4.2 削除されたコード

> **注意**: 実際のコード差分を確認した結果、`APP_IO_CX2.c`にはMULTINONE関連コードが残存しています（7箇所）。`Common/config.h`ではMULTINONE関連コードが削除されています。

#### 4.2.1 Common/config.h

- **設定マクロの条件分岐**（27-43行目）- MULTINONE用の動的設定と通常ビルド用の固定設定を削除
- **NVMEM設定の条件分岐**（45-50行目）- MULTINONE用のセクター設定を削除

#### 4.2.2 APP_IO_CX2/APP_IO_CX2.c

> **注意**: `APP_IO_CX2.c`にはMULTINONE関連コードが残存しています（`#ifdef MWLIB_MULTINONE`など）。

### 4.3 修正箇所

#### ハードウェア初期化（Common/config.h）

```c
// 削除前（Common.ORG/config.h）
#ifdef MWLIB_MULTINONE
extern uint32 _MULTINONE_U32_APP_CONFIG;
# define FW_CONF_ID() ((_MULTINONE_U32_APP_CONFIG>>16)&0xFF)
# define IS_CONF_NORMAL() (FW_CONF_ID()==1)
# define FW_CONF_LANG() ((_MULTINONE_U32_APP_CONFIG>>24)&0xFF)
# define IS_OPT_INTRCT_ON_BOOT() (_MULTINONE_U32_APP_CONFIG & MULTINEONE_APPCONF_MASK_OPT_INTR)
#else
# define IS_OPT_INTRCT_ON_BOOT() (0)
# define IS_CONF_NORMAL() (1)
# define FW_CONF_ID() (1)
# define USE_LANG_INTERACTIVE_DEFAULT 2
# ifndef USE_LANG_INTERACTIVE
#  define USE_LANG_INTERACTIVE USE_LANG_INTERACTIVE_DEFAULT
# endif
# define FW_CONF_LANG() USE_LANG_INTERACTIVE
#endif

// 削除後（Common/config.h）
# define IS_OPT_INTRCT_ON_BOOT() (0)
# define IS_CONF_NORMAL() (1)
# define FW_CONF_ID() (1)
# define USE_LANG_INTERACTIVE_DEFAULT 0
# undef USE_LANG_INTERACTIVE
# define USE_LANG_INTERACTIVE 0
# define FW_CONF_LANG() USE_LANG_INTERACTIVE
```

### 4.4 影響

- `Common/config.h`が単一アプリケーション専用に簡素化されました
- 設定はコンパイル時に固定値として決定されます
- `APP_IO_CX2.c`にはMULTINONE関連コードが残存していますが、通常ビルド時には使用されません

---

## 5. スリープモード処理の無効化

### 5.1 変更内容

スリープモード（1秒/10秒間欠子機）が削除されたため、スリープモード用の状態遷移マシン関数を無効化しました。

### 5.2 変更された関数

- **`vProcessEvCoreSlp()` 関数** - スリープ稼動モード用の状態遷移マシン
  - 関数自体は削除されていません（コードに残存）
  - `__attribute__((unused))`を追加して未使用関数の警告を抑制
  - 呼び出し箇所は削除されています

### 5.3 影響

- スリープモード関連の処理は呼び出されなくなりました
- 常時通電モード（`vProcessEvCorePwr()`）のみが使用されます

---

## 6. 設定値の外部化（`config_private.h`の導入）

### 6.1 変更内容

チャネル設定、アプリケーションID、論理デバイスID、オプションビットなどの設定値を`config_private.h`に外部化しました。このファイルはGitに含まれず、テンプレートファイル（`config_private.h.template`）を提供しています。

### 6.2 追加されたファイル

- `Common/config_private.h` - 実際の設定値（Gitに含まれない）
- `Common/config_private.h.template` - テンプレートファイル（Gitに含まれる）

### 6.3 定義された設定値

- `CHANNEL_DEFAULT`, `CHANNEL_1`, `CHANNEL_2`, `CHANNEL_3` - チャネル設定
- `CONFIG_DEFAULT_APPID` - アプリケーションID
- `CONFIG_DEFAULT_LOGICALID` - 論理デバイスID
- `CONFIG_DEFAULT_CHANNEL`, `CONFIG_DEFAULT_CHANNELS_3` - チャネル設定
- `CONFIG_DEFAULT_POWER_N_RETRY` - 送信出力と再送回数
- `CONFIG_DEFAULT_UARTBAUD` - UARTボーレート
- `CONFIG_DEFAULT_OPTBITS` - オプションビット
- `CONFIG_DEFAULT_FPS` - FPS設定
- `CONFIG_DEFAULT_ENCENABLE`, `CONFIG_DEFAULT_ENCKEY` - 暗号化設定

---

## 7. インタラクティブモードの言語設定を英語に変更

### 7.1 変更内容

インタラクティブモードのメニュー表示言語を日本語から英語に変更しました。

### 7.2 修正箇所

#### Common/config.h

```c
# define USE_LANG_INTERACTIVE_DEFAULT 0 // 0,1:English 2:Japanese
# undef USE_LANG_INTERACTIVE  // 既存の定義を削除
# define USE_LANG_INTERACTIVE 0  // 英語に強制設定
```

#### APP_IO_CX2/build/Makefile

```makefile
TWE_LANG_PREF ?= EN
```

---

## 8. デフォルト設定値の一元管理（`twesettings_std_defsets.c`の追加）

### 8.1 変更内容

デフォルト設定値を`twesettings_std_defsets.c`で一元管理するようにしました。このファイルは標準ライブラリからコピーしたものです。

### 8.2 追加されたファイル

- `Common/twesettings_std_defsets.c` - デフォルト値定義
- `Common/twesettings_std_defsets.h` - ヘッダファイル

### 8.3 修正箇所

#### Makefileへの追加（APP_IO_CX2/build/Makefile）

```makefile
APPSRC += twesettings_std_defsets.c
```

---

## 9. 動作仕様

### 9.1 モード切り替え

- **M1=オープン（Hi）**: 子機モード（`E_IO_MODE_CHILD_CONT_TX`）
- **M1=GND（Lo）**: 親機モード（`E_IO_MODE_PARNET`、App_IO データシートの M1=G, M2=O, M3=O 例と一致）

### 9.2 チャネル設定

| EI1 | EI2 | チャネル設定値 | 使用チャネル |
|-----|-----|--------------|------------|
| O (0) | O (0) | 0 | デフォルト/フラッシュ値 |
| G (1) | O (0) | 1 | チャネル12 |
| O (0) | G (1) | 2 | チャネル21 |
| G (1) | G (1) | 3 | チャネル25 |

**注意**: ウォッチドッグ出力機能が有効な場合、`PORT_EI2`は使用されず、`PORT_EI1`のみで制御（0または1の2通り）。

### 9.3 削除された機能

- シリアルポート通信
- 中継機能
- スリープモード（1秒/10秒間欠子機）
- 通常子機モード

---

## 10. コンパイル・動作確認

### 10.1 リンターエラー

修正後、リンターエラーはありません。

### 10.2 動作確認項目

以下の動作を確認してください：

1. **モード切り替え**
   - M1=オープン（Hi）で子機として動作
   - M1=GND（Lo）で親機として動作

2. **チャネル設定**
   - EI1, EI2の組み合わせでチャネルが切り替わる
   - ウォッチドッグ出力有効時はEI1のみで制御

3. **無線通信**
   - 親機と子機間で正常に通信できる
   - 中継機能が動作しないことを確認

4. **ウォッチドッグ出力**
   - オプションビット有効時、PORT_EI2からパルスが出力される

---

## 11. 注意事項

### 11.1 互換性

- 本修正により、以前のバージョンとの互換性は失われます
- 中継機として動作していたデバイスは使用できません
- シリアルポート経由の設定・デバッグはできません

### 11.2 今後の拡張

必要に応じて、以下の機能を追加できます：

- 新しいモードの追加
- その他の専用機能

**注意**: シリアルポート機能はインタラクティブモードで使用されているため、既に実装されています。

---

## 12. 関連ファイル

### 修正されたファイル

- `APP_IO_CX2/APP_IO_CX2.c` - メインアプリケーションファイル（旧`App_IO/App_IO.c`、2740行→2618行、122行削減）
- `APP_IO_CX2/APP_IO_CX2.h` - ヘッダファイル（旧`App_IO/App_IO.h`）
- `Common/config.h` - 設定ファイル（MULTINONE統合フレームワーク削除、config_private.h導入、言語設定変更）
- `Common/Interactive.c` - インタラクティブモード処理（APP_IO_CX2.hへの変更、config_private.h使用）
- `Common/Pairing.c` - ペアリング処理（APP_IO_CX2.hへの変更）
- `Common/common.c` - 共通処理（APP_IO_CX2.hへの変更）
- `Common/twesettings_std_defsets.c` - 標準ライブラリからコピーしたデフォルト値定義
- `Common/twesettings_std_defsets.h` - 標準ライブラリからコピーしたヘッダファイル

### 新規追加ファイル

- `Common/config_private.h.template` - 設定値テンプレートファイル（Gitに含まれる）
- `Common/config_private.h` - 実際の設定値ファイル（Gitに含まれない、各自作成）

### 参照ファイル

- `Common/common.h` - モード定義、チャネルマスク定義
- `Common/Interactive.h` - オプションビット定義

---

## 13. 変更履歴

| 日付 | 変更内容 |
|------|---------|
| - | アプリケーション名を`App_IO`から`APP_IO_CX2`に変更（ファイル名、フォルダ名、設定値、すべての`App_IO.h`参照を`APP_IO_CX2.h`に変更） |
| - | モード設定をM1のみに簡素化（M2, M3ピンの使用を削除） |
| - | 不要なモード処理を削除（ROUTER、CHILD、CHILD_SLP_1SEC、CHILD_SLP_10SEC） |
| - | 中継処理を削除（ROUTERモード関連） |
| - | スリープモード処理を無効化（`vProcessEvCoreSlp`関数は残存、`__attribute__((unused))`を追加） |
| - | `Common/config.h`からMULTINONE統合フレームワーク関連コードを削除 |
| 2026-01-04 | 設定値の外部化（`config_private.h`の導入） |
| 2026-01-04 | インタラクティブモードの言語設定を英語に変更 |
| 2026-01-04 | デフォルト設定値の一元管理（`twesettings_std_defsets.c`の追加） |

---

## 14. 参考情報

### 14.1 チャネル設定入力

詳細は、チャネル設定入力機能のドキュメントを参照してください。

### 14.2 ウォッチドッグ出力機能

ウォッチドッグ出力機能の詳細については、関連ドキュメントを参照してください。

---

**作成日**: 2024年
**最終更新**: 2026年1月4日（オリジナルコードとの差分比較に基づく更新）


