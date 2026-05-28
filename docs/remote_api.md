# Remote Debug HTTP API

## Overview

Azahar にリモートデバッグ用の HTTP REST API を追加します。
主に LLM エージェントがエミュレータをプログラムから制御する目的で設計されています。

### 前提

- **プロトコル**: HTTP REST（JSON リクエスト/レスポンス）
- **セキュリティ**: ローカル専用。認証・暗号化なし（127.0.0.1 バインドがデフォルト）
- **CORS**: 全レスポンスに `Access-Control-Allow-Origin: *` を設定（`remote_http_server.cpp` の pre_routing_handler）
- **依存ライブラリ**: cpp-httplib v0.14.0 / nlohmann/json v3.9.0（両方とも bundled、新規外部依存なし）
- **ビルドフラグ**: `ENABLE_REMOTE_SERVER`（CMake option、デフォルト OFF）
- **LibRetro**: 非対応（`_LIBRETRO_INCOMPATIBLE_OPTIONS` に含む）

## ビルド設定

```cmake
cmake -B build -DENABLE_REMOTE_SERVER=ON
cmake --build build
```

## 設定値

| キー | 型 | デフォルト | 説明 |
|------|-----|-----------|------|
| `enable_remote_server` | bool | `false` | サーバーを有効化 |
| `remote_server_port` | u16 | `49355` | 待受ポート |
| `remote_server_bind_address` | string | `127.0.0.1` | バインドアドレス |

## エンドポイント一覧

### Emulator Control

#### `POST /api/v1/emulator/control`

エミュレータの実行状態を制御します。

**Request Body:**
```json
{"action": "pause"}
```

**action 一覧:**

| action | 動作 | レスポンス state |
|--------|------|-----------------|
| `pause` | 一時停止（フレームアドバンスモード） | `paused` |
| `resume` | 再開 | `running` |
| `stop` | シャットダウン | `stopped` |
| `reset` | リセット（再起動） | `running` |

**成功レスポンス (200):**
```json
{"status": "ok", "state": "paused"}
```

**エラーレスポンス (400):**
```json
{"error": "unknown action: hoge"}
```

**エラーレスポンス (409):**
```json
{"error": "signal already pending", "code": "signal_pending"}
```

---

#### `POST /api/v1/emulator/speed`

エミュレーション速度を設定します。

**Request Body:**
```json
{"speed_percent": 200}
```

- `speed_percent`: 1–1000（デフォルト 100、範囲外は clamp）。

**成功レスポンス (200):**
```json
{"status": "ok", "current_speed": 200}
```

---

#### `GET /api/v1/emulator/status`

エミュレータの現在状態を取得します。

**成功レスポンス (200):**
```json
{
  "state": "running",
  "is_powered_on": true,
  "title_id": "00040000000EDF00"
}
```

- `state`: `running` / `paused` / `stopped`
- `is_powered_on`: 電源状態
- `title_id`: 現在実行中のタイトル ID（16進数、停止中は `0000000000000000`）

---

### Save States

#### `POST /api/v1/state/save`

セーブステートを保存します。HTTP レスポンスは保存処理が完了するまで返りません（最大 10 秒）。

**Request Body:**
```json
{"slot": 3}
```

- `slot`: 0–10（デフォルト 0、範囲外は clamp）

**成功レスポンス (200):**
```json
{"status": "ok", "slot": 3}
```

**エラーレスポンス:**
- `409 signal_pending`: 別のシグナルまたは state 操作が処理中
- `504 state_operation_timeout`: 10 秒以内に保存が完了しなかった。未開始のリクエストはキャンセルされます
- `500 state_operation_failed`: 保存処理が失敗した

---

#### `POST /api/v1/state/load`

セーブステートを読み込みます。HTTP レスポンスは読み込み処理が完了するまで返りません（最大 10 秒）。

**Request Body:**
```json
{"slot": 3}
```

- `slot`: 0–10（デフォルト 0、範囲外は clamp）

**成功レスポンス (200):**
```json
{"status": "ok", "slot": 3}
```

**エラーレスポンス:**
- `409 signal_pending`: 別のシグナルまたは state 操作が処理中
- `504 state_operation_timeout`: 10 秒以内に読み込みが完了しなかった。未開始のリクエストはキャンセルされます
- `500 state_operation_failed`: 読み込み処理が失敗した

---

#### `GET /api/v1/state/list`

セーブステート一覧を返します（現在は stub）。

**成功レスポンス (200):**
```json
{"status": "not_implemented"}
```

---

### Cheats

#### `GET /api/v1/cheats/list`

有効なチート一覧を返します（現在は stub）。

**成功レスポンス (200):**
```json
{"status": "not_implemented"}
```

#### `POST /api/v1/cheats/enable`

チートを有効化します（現在は stub）。

**成功レスポンス (200):**
```json
{"status": "not_implemented"}
```

#### `POST /api/v1/cheats/disable`

チートを無効化します（現在は stub）。

**成功レスポンス (200):**
```json
{"status": "not_implemented"}
```

---

### Events

#### `GET /api/v1/events`

イベントキューをポーリングします（現在は stub）。

**成功レスポンス (200):**
```json
{"status": "not_implemented"}
```

---

### Video

#### `GET /api/v1/video/screenshot`

スクリーンショットを取得します（現在は未実装）。

**成功レスポンス (200):**
```json
{"status": "not_implemented"}
```

---

## LLM エージェント向け注意事項

1. **エラーハンドリング**: HTTP ステータスコードと JSON 内のエラー情報を併用してください。
   400 = リクエスト不正、404 = パス未登録、409 = シグナル競合、504 = state 操作タイムアウト、500 = サーバー内部エラー。
   200 でも body に `"status":"error"` が含まれる場合があります（`action` 不明など）。

2. **速度設定**: `speed_percent` は 1–1000 の範囲です（デフォルト 100）。
   範囲外は clamp されます。

3. **非同期動作**: `stop` / `reset` は `SendSignal` 経由でシグナルをキューに送り、
   レスポンスは「シグナルを受け付けた」ことを保証します。`save` / `load` は
   state 操作が完了、失敗、競合、またはタイムアウトするまでレスポンスを返しません。

4. **レート制限**: 現在の実装にはレート制限がありません。連続リクエストは
   エミュレーションスレッドに負荷をかける可能性があります。

5. **Events エンドポイント**: 現在 stub です。将来はフレームバッファ更新や
   状態変更などのイベントをポーリングできるようになる予定です。

## Phase 計画

| Phase | 内容 | 状態 |
|-------|------|------|
| 1 | 基本制御 + ステート + チート/イベント/ビデオ stub | ✅ 完了 |
| 2 | メモリ読み書きエンドポイント | 📝 未着手 |
| 3 | スクリーンショット実装、入力注入（タッチ/ボタン/アナログ） | 📝 未着手 |
| 4 | マクロ記録/再生 | 📝 未着手 |

## ビルド設定リファレンス

```cmake
option(ENABLE_REMOTE_SERVER "Enable remote debug HTTP server" OFF)
```

`src/core/CMakeLists.txt` で条件付きコンパイル:
```cmake
if (ENABLE_REMOTE_SERVER)
    target_compile_definitions(citra_core PUBLIC -DENABLE_REMOTE_SERVER)
    target_sources(citra_core PRIVATE
        remote/remote_server.cpp
        remote/remote_server.h
        remote/remote_http_server.cpp
        remote/remote_http_server.h
        remote/remote_handler.cpp
        remote/remote_handler.h
        remote/remote_types.h
        remote/handlers/emulator_handler.cpp
        remote/handlers/state_handler.cpp
        remote/handlers/cheat_handler.cpp
        remote/handlers/events_handler.cpp
        remote/handlers/video_handler.cpp
    )
endif()
```

## ソース構成

```
src/core/remote/
├── remote_types.h              # RemoteRequest / RemoteResponse / RemoteEvent
├── remote_http_server.h/.cpp   # httplib::Server ラッパー
├── remote_server.h/.cpp        # 高レベル Server オーケストレータ
├── remote_handler.h/.cpp       # リクエストディスパッチャ（ルーティング）
└── handlers/
    ├── emulator_handler.cpp    # control / speed / status
    ├── state_handler.cpp       # save / load / list
    ├── cheat_handler.cpp       # list / enable / disable (stub)
    ├── events_handler.cpp      # events polling (stub)
    └── video_handler.cpp       # screenshot (stub)
```

コア側の統合ポイント:
- `src/core/core.h` — `#ifdef ENABLE_REMOTE_SERVER` で `Remote::Server` のメンバを追加
- `src/core/core.cpp` — `System::Init()` で起動、`System::Shutdown()` で破棄
- `src/common/settings.h` — `enable_remote_server`, `remote_server_port`, `remote_server_bind_address`
