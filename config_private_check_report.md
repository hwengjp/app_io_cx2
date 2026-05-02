# config_private.h 設定値の重複チェックレポート

## 概要
`config_private.h`で定義されている設定値が、他のファイルで直接記述されていないか確認しました。

## チェック結果

### ✅ 問題なし（マクロ参照のみ）

以下の設定値は、他のファイルでマクロ名として参照されており、直接値の記述は見つかりませんでした：

1. **CHANNEL_DEFAULT (24)**
   - `Common/common.c:125`: `CHMASK`マクロ経由で参照
   - `Common/config.h:61`: `CHANNEL`マクロとして参照

2. **CHANNEL_1 (26)**
   - `Common/common.c:126`: `CHMASK_1`マクロ経由で参照

3. **CHANNEL_2 (19)**
   - `Common/common.c:127`: `CHMASK_2`マクロ経由で参照

4. **CHANNEL_3 (22)**
   - `Common/common.c:128`: `CHMASK_3`マクロ経由で参照

5. **CONFIG_DEFAULT_APPID (0xA3F7C2E9UL)**
   - `Common/twesettings_std_defsets.c:26`: マクロとして参照

6. **CONFIG_DEFAULT_LOGICALID (1)**
   - `Common/twesettings_std_defsets.c:35`: マクロとして参照

7. **CONFIG_DEFAULT_CHANNEL (24)**
   - `Common/twesettings_std_defsets.c:43`: マクロとして参照

8. **CONFIG_DEFAULT_CHANNELS_3 ((1UL << 24) >> 11)**
   - `Common/twesettings_std_defsets.c:52`: マクロとして参照

9. **CONFIG_DEFAULT_POWER_N_RETRY (0x93)**
   - `Common/twesettings_std_defsets.c:63`: マクロとして参照
   - **注意**: `Common/common.h:146`で`0x93`が`SERCMD_ID_INFORM_NETWORK_CONFIG`として使用されていますが、これは別の用途（シリアルコマンドID）であり、問題ありません。

10. **CONFIG_DEFAULT_UARTBAUD (1152)**
    - `Common/twesettings_std_defsets.c:76`: マクロとして参照

11. **CONFIG_DEFAULT_OPTBITS (0x420000UL)**
    - `Common/twesettings_std_defsets.c:86`: マクロとして参照

### ⚠️ 注意が必要な箇所

#### 1. APP_ID の直接定義
**ファイル**: `Common/config.h:55`
```c
#define APP_ID              0x67720107 //!< アプリケーションID。同じIDでないと通信しない。
```

**調査結果**:

**`APP_ID`の使用箇所**:
1. **ペアリング時の照合ID** (`APP_IO_CX2/APP_IO_CX2.c:436`)
   ```c
   sConfig.u32PairKey = APP_ID;  // ペアリング時の照合ID
   ```

2. **設定の保存/読み込み時の識別子** (`Common/Interactive.c:373, 377`)
   ```c
   TWESTG_u32SetBaseInfoToFinal(&gc_sFinal, APP_ID, APPVER, STGS_SET_VER, STGS_SET_VER_COMPAT);
   TWESTG_u32LoadDataFrAppstrg(&gc_sFinal, u8kind, u8slot, APP_ID, STGS_SET_VER_COMPAT, ...);
   ```
   - 設定データのバージョン管理と互換性チェックに使用

3. **カスタムデフォルト値** (`Common/Interactive.c:63`)
   ```c
   E_TWESTG_DEFSETS_APPID, (TWESTG_DATATYPE_UINT32 << 4) | 4, 
   (APP_ID>>24)&0xFF,(APP_ID>>16)&0xFF,(APP_ID>>8)&0xFF,APP_ID&0xFF,
   ```
   - インタラクティブモードの初期デフォルト値として使用

4. **ペアリングIDの計算** (`Common/Pairing.h:16`)
   ```c
   #define PAIR_ID  APP_ID+3
   ```

**`CONFIG_DEFAULT_APPID`の使用箇所**:
1. **インタラクティブモードのデフォルト値** (`Common/twesettings_std_defsets.c:26`)
   ```c
   {.u32 = CONFIG_DEFAULT_APPID }  // インタラクティブモードで設定可能なデフォルト値
   ```

**問題点**: 
- `APP_ID`（`0x67720107`）と`CONFIG_DEFAULT_APPID`（`0xA3F7C2E9UL`）が異なる値で定義されています
- `APP_ID`は設定の保存/読み込み時の識別子として使用されており、これは**アプリケーションのバージョン管理用**です
- `CONFIG_DEFAULT_APPID`はインタラクティブモードで**ユーザーが設定可能なアプリケーションIDのデフォルト値**です
- 実際の通信で使用されるアプリケーションIDは、設定から読み込まれた値（`sAppStg.u32appid`）で、これは`CONFIG_DEFAULT_APPID`がデフォルトとして使用されます

**結論**:
- `APP_ID`と`CONFIG_DEFAULT_APPID`は**異なる用途**で使用されています
- `APP_ID`は設定データの識別子（アプリケーション固有の識別子）として使用
- `CONFIG_DEFAULT_APPID`は実際の通信で使用されるアプリケーションIDのデフォルト値
- **現状の実装は正しく、変更の必要はありません**

**推奨対応**:
- 現状のまま維持（変更不要）
- ただし、将来的に`APP_ID`を`CONFIG_DEFAULT_APPID`に統一したい場合は、設定データの互換性に注意が必要です

#### 2. UART_BAUD の直接定義
**ファイル**: `Common/config.h:45`
```c
#define UART_BAUD			115200UL //!< UART のボーレート（デフォルト）
```

**問題点**:
- `config_private.h`の`CONFIG_DEFAULT_UARTBAUD`（`1152`）とは異なる形式（実際のボーレート値）で定義されています
- `UART_BAUD`は実際のボーレート値（115200）、`CONFIG_DEFAULT_UARTBAUD`は100で割った値（1152）なので、用途が異なる可能性があります

**推奨対応**:
- `UART_BAUD`が`CONFIG_DEFAULT_UARTBAUD * 100`として計算すべきか確認
- 統一する場合は、`UART_BAUD`を`(CONFIG_DEFAULT_UARTBAUD * 100UL)`に置き換える

#### 3. ハードコードされたUARTボーレート値
**ファイル**: `APP_IO_CX2/APP_IO_CX2.c:383`
```c
115200UL, // 8N1
```

**問題点**:
- UART初期化時に`115200UL`が直接記述されています
- `UART_BAUD`マクロを使用すべき可能性があります

**推奨対応**:
- `UART_BAUD`マクロを使用するように変更を検討

### 📝 ドキュメント内の記述

以下の値は`README.md`に記載されていますが、これらは**メーカーのデフォルト値**としての例示であり、実際の設定値ではありません：

- `README.md:404-407`: `CHANNEL_DEFAULT=16`, `CHANNEL_1=12`, `CHANNEL_2=21`, `CHANNEL_3=25`（メーカーデフォルト値の例）
- `README.md:426-427`: `CONFIG_DEFAULT_CHANNEL=16`（メーカーデフォルト値の例）
- `README.md:432`: `CONFIG_DEFAULT_POWER_N_RETRY=0x93`（メーカーデフォルト値の例）
- `README.md:442`: `CONFIG_DEFAULT_OPTBITS=0x420000UL`（メーカーデフォルト値の例）

これらは問題ありません（実際の設定値は`config_private.h`で管理されているため）。

## まとめ

- **主要な設定値**: すべてマクロ経由で参照されており、直接値の記述は見つかりませんでした
- **`APP_ID`について**: 調査の結果、`APP_ID`と`CONFIG_DEFAULT_APPID`は異なる用途で使用されています。`APP_ID`は設定データの識別子（アプリケーション固有の識別子）として使用され、`CONFIG_DEFAULT_APPID`は実際の通信で使用されるアプリケーションIDのデフォルト値です。**現状の実装は正しく、変更の必要はありません**
- **注意が必要**: `UART_BAUD`が`config_private.h`の設定値と異なる形式で直接定義されています。用途を確認し、必要に応じて統一を検討してください

