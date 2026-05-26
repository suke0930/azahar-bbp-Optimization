# Azahar BBP Optimization
# おことわり
割とAI SLOP寄りなので注意してください
このforkはまずどのような手を使ってでも機能の再現/workを最優先しており、最適化は二の次で進めてます

## 日本語

これは、バンブラP（Daigasso! Band Brothers P / BBP）の最適化と検証を目的にした、個人用の Azahar fork です。

このリポジトリは、BBP 向けのパッチ、診断コード、実験ビルドを公開・整理するための場所です。汎用の Azahar 配布版を目指すものではなく、upstream への取り込みや一般用途への適合を前提にしません。

## 目的

- BBP の描画やパフォーマンス挙動に絞った Azahar fork を管理する。
- BBP 最適化パッチや実験ビルドを、個人用の公開リポジトリとして残す。
- 診断コード、レンダリング修正、framebuffer 周りの実験などをひとつの履歴にまとめる。
- 将来的に使えそうな変更も、まずは BBP 専用の形で扱えるようにする。

## スコープ

この fork は BBP 専用の作業と個人的な実験を対象にしています。変更には、エミュレータ側の診断コード、描画修正、framebuffer handling の実験、パフォーマンス最適化、一時的な調査コードが含まれる場合があります。

すべての変更を upstream に提出できる形へ整える必要はありません。必要になったものだけ、後から整理して再利用する方針です。

## BBP ローカルマルチプレイ対応

現在の pre-release では、BBP のローカル通信マルチプレイで部屋を検出して参加できるようにするための DLP/ローカル通信まわりの修正を含めています。

既知の挙動として、ゲスト側の部屋一覧で部屋表示が一時的に点滅したり消えたりする場合があります。多くの場合は一定時間後に再表示されますが、完全に安定した表示ではないため、接続できない場合は少し待ってから再度選択してください。

## Radeon Vulkan Fallback

Windows の Radeon 環境で通常の `azahar.exe` が Vulkan 初期化中に落ちる場合、配布物に同梱される `azahar-vulkan-validation.cmd` を使うと起動できることがあります。

この fallback は `VK_LAYER_KHRONOS_validation` を使うため、Vulkan SDK / validation layer が必要です。通常起動では不要ですが、fallback を使う場合は事前に以下のいずれかで導入してください。

```powershell
winget install --id KhronosGroup.VulkanSDK -e
```

https://vulkan.lunarg.com/sdk/home

## 開発環境

この fork の調査、実装、整理には Codex、OpenCode、Claude などのエージェント系ツールを利用しています。生成された変更は、手元で確認しながら BBP 向けのパッチとして整理します。

## ベースプロジェクト

Azahar は Citra をベースにしたオープンソースの Nintendo 3DS エミュレータです。

Base project:

https://github.com/azahar-emu/azahar

## ライセンス

このリポジトリは upstream Azahar のライセンスに従います。詳細は `license.txt` を参照してください。

---

## English

This is a personal Azahar fork dedicated to optimizing and testing Daigasso!
Band Brothers P, known in Japanese as バンブラP and also referred to as BBP.

This repository is a personal publication and experiment space for BBP-focused
patches, diagnostics, and builds. It is not intended to be a general-purpose
Azahar distribution, and changes do not need to satisfy upstream acceptance
criteria by default.

## Purpose

- Maintain a personal Azahar fork focused on BBP rendering and performance.
- Publish BBP optimization patches and experimental builds in one public place.
- Keep diagnostics, rendering fixes, framebuffer experiments, and related work
  in a stable history.
- Allow useful changes to start as BBP-specific work before any broader cleanup.

## Scope

This fork is intended for BBP-specific work and personal experimentation.
Changes may include emulator-side diagnostics, rendering fixes, framebuffer
handling experiments, performance optimizations, and temporary investigation
code useful for Band Brothers P.

Not every change is expected to be suitable for upstream submission. If a patch
becomes broadly useful, it can be cleaned up later.

## BBP Local Multiplayer Support

Current pre-release builds include DLP/local communication fixes that allow BBP
local multiplayer rooms to be discovered and joined.

Known behavior: on the guest side, room entries may temporarily flicker or
disappear from the room list. They usually return after a short time, but the
display is not fully stable yet. If a room cannot be selected immediately, wait
briefly and try selecting it again.

## Radeon Vulkan Fallback

On Windows Radeon systems where the normal `azahar.exe` crashes during Vulkan
initialization, the bundled `azahar-vulkan-validation.cmd` may allow the app to
start successfully.

This fallback uses `VK_LAYER_KHRONOS_validation`, so the Vulkan SDK /
validation layer must be installed first. Normal startup does not require it,
but the fallback launcher does. Install it with either:

```powershell
winget install --id KhronosGroup.VulkanSDK -e
```

https://vulkan.lunarg.com/sdk/home

## Remote Debug API (`feature/remote-debug-api`)

このブランチは、LLM エージェントが HTTP REST API 経由で Azahar エミュレータを
プログラムから制御できるようにするリモートデバッグ機能を追加します。

**ユースケース**: LLM エージェントがエミュレータ（Windows）とは別のマシン（Linux）
から HTTP で操作し、自動テスト・自動探索・デバッグを行うことを想定しています。

**ビルド**: CMake の `-DENABLE_REMOTE_SERVER=ON`（デフォルト OFF）で有効化。
LibRetro ビルドでは自動的に無効化されます。

**詳細な API リファレンス**: [`docs/remote_api.md`](docs/remote_api.md)

### 有効化方法

1. 設定で `enable_remote_server = true`（デフォルト `false`）、`remote_server_port = 49355` を設定
2. エミュレータ起動後、`curl http://127.0.0.1:49355/api/v1/emulator/status` で動作確認

### 実装済みエンドポイント（Phase 1）

| Method | Path | 説明 |
|--------|------|------|
| POST | `/api/v1/emulator/control` | pause / resume / stop / reset |
| POST | `/api/v1/emulator/speed` | エミュレーション速度設定 (0-1000%) |
| GET  | `/api/v1/emulator/status` | 現在の状態・Title ID |
| POST | `/api/v1/state/save` | セーブステート保存 (slot 0-10) |
| POST | `/api/v1/state/load` | セーブステート読込 (slot 0-10) |
| GET  | `/api/v1/state/list` | セーブステート一覧 (stub) |
| GET  | `/api/v1/cheats/list` | チート一覧 (stub) |
| POST | `/api/v1/cheats/enable` | チート有効化 (stub) |
| POST | `/api/v1/cheats/disable` | チート無効化 (stub) |
| GET  | `/api/v1/events` | イベントポーリング (stub) |
| GET  | `/api/v1/video/screenshot` | スクリーンショット (stub) |

## Development Environment

This fork uses agent-based tools such as Codex, OpenCode, and Claude for
investigation, implementation, and cleanup. Generated changes are reviewed
locally and organized as BBP-focused patches.

## Base Project

Azahar is an open-source Nintendo 3DS emulator based on Citra.

Base project:

https://github.com/azahar-emu/azahar

## License

This repository follows the upstream Azahar license. See `license.txt`.
