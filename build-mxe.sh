#!/usr/bin/env bash
# build-mxe.sh — MXE クロスコンパイルで Linux 上から Windows 向け azahar をビルド
#
# 事前条件:
#   - Docker がインストールされていること
#   - opensauce04/azahar-build-environment:latest イメージが pull 済みであること
#
# 使い方:
#   ./build-mxe.sh              # 通常ビルド (ccache 再利用)
#   CLEAN=1 ./build-mxe.sh      # クリーンビルド
#
# 環境変数:
#   BUILD_DIR     ビルドディレクトリ (デフォルト: build-mxe)
#   CCACHE_DIR    ccache ディレクトリ (デフォルト: .ccache-mxe)
#   OUT_DIR       出力先 (デフォルト: out-mxe)
#   DOCKER_IMAGE  Docker イメージ (デフォルト: opensauce04/azahar-build-environment:latest)
#   BUILD_ID      ビルド識別子 (デフォルト: azahar-bbp)
#   CLEAN         1 でビルドディレクトリを削除してからビルド
#   ARCHIVE_FMT   アーカイブ形式 zip|7z (デフォルト: zip)
#   ARCHIVE_LV    圧縮レベル 0-9、7z のみ有効 (デフォルト: 2)

set -euo pipefail

if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
    sed -n '/^# build-mxe/,/^$/p' "$0" | sed 's/^# \?//'
    exit 0
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$SCRIPT_DIR"

# --- 設定 ---
BUILD_DIR="${BUILD_DIR:-$REPO_DIR/build-mxe}"
CCACHE_DIR="${CCACHE_DIR:-$REPO_DIR/.ccache-mxe}"
OUT_DIR="${OUT_DIR:-$REPO_DIR/out-mxe}"
DOCKER_IMAGE="${DOCKER_IMAGE:-opensauce04/azahar-build-environment:latest}"
BUILD_ID="${BUILD_ID:-azahar-bbp}"
CLEAN="${CLEAN:-0}"
ARCHIVE_FMT="${ARCHIVE_FMT:-zip}"
ARCHIVE_LV="${ARCHIVE_LV:-2}"

# --- 安全チェック ---
case "$BUILD_ID" in
    *[!A-Za-z0-9._-]*|"") echo "Unsafe BUILD_ID: $BUILD_ID" >&2; exit 2 ;;
esac
case "$ARCHIVE_FMT" in
    zip|7z) ;;
    *) echo "ARCHIVE_FMT must be zip or 7z: $ARCHIVE_FMT" >&2; exit 2 ;;
esac
case "$ARCHIVE_LV" in
    [0-9]) ;;
    *) echo "ARCHIVE_LV must be 0-9: $ARCHIVE_LV" >&2; exit 2 ;;
esac

echo ""
echo "=== azahar MXE クロスビルド ==="
echo "BUILD_DIR:   $BUILD_DIR"
echo "CCACHE_DIR:  $CCACHE_DIR"
echo "OUT_DIR:     $OUT_DIR"
echo "DOCKER:      $DOCKER_IMAGE"
echo "BUILD_ID:    $BUILD_ID"
echo "CLEAN:       $CLEAN"
echo "ARCHIVE:     $ARCHIVE_FMT (level $ARCHIVE_LV)"
echo ""

# --- 事前チェック ---
if ! command -v docker >/dev/null 2>&1; then
    echo "ERROR: Docker がインストールされていません" >&2
    exit 1
fi
if ! docker image inspect "$DOCKER_IMAGE" >/dev/null 2>&1; then
    echo "Docker イメージ $DOCKER_IMAGE が見つかりません。pull します..."
    docker pull "$DOCKER_IMAGE"
fi

# --- 準備 ---
mkdir -p "$BUILD_DIR" "$CCACHE_DIR" "$OUT_DIR"

# dllwalker サブモジュール初期化 (MXE ビルド時の DLL 収集に必要)
echo "[1/4] サブモジュール初期化..."
git submodule update --init --recursive

if [[ "$CLEAN" == "1" ]]; then
    echo "[*] クリーンビルド: $BUILD_DIR を削除"
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
fi

# --- Docker で MXE クロスコンパイル ---
LOG="$OUT_DIR/$BUILD_ID-build.log"

echo "[2/4] Docker コンテナで MXE ビルド実行..."
docker run --rm -u "$(id -u):$(id -g)" \
  -e CCACHE_DIR=/work-ccache \
  -e CCACHE_COMPILERCHECK=content \
  -e CCACHE_SLOPPINESS=time_macros \
  -v "$REPO_DIR:/work" \
  -v "$BUILD_DIR:/build" \
  -v "$CCACHE_DIR:/work-ccache" \
  -w /build \
  "$DOCKER_IMAGE" \
  bash -lc "set -euo pipefail
    export PATH=\"/mxe/usr/bin:\$PATH\"

    echo \"BUILD_START \$(date -Is)\"

    # CMake 設定
    x86_64-w64-mingw32.shared-cmake /work \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER_LAUNCHER=ccache \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
      -DCITRA_USE_PRECOMPILED_HEADERS=OFF \
      -DENABLE_QT_TRANSLATION=ON \
      -DUSE_DISCORD_PRESENCE=ON \
      -DUSE_SYSTEM_BOOST=ON \
      -DUSE_SYSTEM_CRYPTOPP=ON

    # ビルド + bundle
    x86_64-w64-mingw32.shared-cmake --build . --target bundle -- -j\$(nproc)

    # 成果物確認
    test -f bundle/azahar.exe || { echo \"ERROR: bundle/azahar.exe が見つかりません\" >&2; exit 1; }

    echo \"BUILD_END \$(date -Is)\"
    ccache -s
  " 2>&1 | tee "$LOG"

# --- アーカイブ ---
ARCHIVE_PATH="$OUT_DIR/$BUILD_ID.$ARCHIVE_FMT"
SHA_PATH="$OUT_DIR/$BUILD_ID-sha256.txt"

echo ""
echo "[3/4] アーカイブ作成 ($ARCHIVE_FMT)..."

if [[ "$ARCHIVE_FMT" == "7z" ]]; then
    if ! command -v 7z >/dev/null 2>&1; then
        echo "WARNING: 7z が見つかりません。zip にフォールバックします。" >&2
        ARCHIVE_FMT="zip"
        ARCHIVE_PATH="$OUT_DIR/$BUILD_ID.zip"
    fi
fi

if [[ "$ARCHIVE_FMT" == "7z" ]]; then
    7z a -t7z -mx="$ARCHIVE_LV" -mmt=on "$ARCHIVE_PATH" "$BUILD_DIR/bundle" >/dev/null
else
    (cd "$BUILD_DIR" && zip -qr "$ARCHIVE_PATH" bundle)
fi

# --- チェックサム ---
echo "[4/4] SHA256 チェックサム生成..."
sha256sum "$BUILD_DIR/bundle/azahar.exe" "$ARCHIVE_PATH" | tee "$SHA_PATH"

echo ""
echo "=== ビルド完了 ==="
ls -lh "$BUILD_DIR/bundle/azahar.exe" "$ARCHIVE_PATH"
echo ""
echo "成果物:"
echo "  exe:     $BUILD_DIR/bundle/azahar.exe"
echo "  archive: $ARCHIVE_PATH"
echo "  sha256:  $SHA_PATH"
echo "  log:     $LOG"
