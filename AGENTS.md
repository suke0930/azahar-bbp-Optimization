# Azahar BBP Optimization

Daigasso! Band Brothers P (バンブラP / BBP) 向け Azahar 3DS エミュレータの最適化 fork。

- Upstream: https://github.com/azahar-emu/azahar
- この fork は BBP の描画/パフォーマンス/DLP 通信に特化した実験・パッチを管理
- 必ずしも upstream 互換・汎用配布向けではない

## Build

**Windows 向け MXE クロスビルド（Linux 上）:**

スキル `skill(name="azahar-build")` をロードして `build-mxe.sh` を実行する。

```bash
./build-mxe.sh              # 増分ビルド
CLEAN=1 ./build-mxe.sh      # クリーンビルド
```

成果物: `out-mxe/azahar-bbp.7z`

## Remote Debug API

`ENABLE_REMOTE_SERVER=ON` でビルド済み。設定ファイルに以下を追加してエミュレータを再起動:

```ini
[Debugging]
enable_remote_server=true
remote_server_port=49355
remote_server_bind_address=127.0.0.1
```

Windows: `%APPDATA%\Azahar\config\qt-config.ini`
Linux: `~/.config/azahar-emu/qt-config.ini`

API 詳細は `docs/remote_api.md` 参照。

## Key Conventions

- Commit messages: conventional commits
- Branch: `feature/remote-debug-api` (current)
- Build artifacts are git-ignored (`build-mxe/`, `out-mxe/`, `.ccache-mxe/`)
- `src/core/remote/` — HTTP debug API (LLM agent 向け)
- `src/core/hle/service/dlp/` — BBP ローカルマルチプレイ DLP 修正
