# Azahar MXE Build Skill

## Overview

Build azahar for Windows from Linux using MXE cross-compilation inside Docker.

## Prerequisites

- Docker installed and running
- `opensauce04/azahar-build-environment:latest` image pulled (`docker pull opensauce04/azahar-build-environment:latest`)
- `7z` (p7zip) for `.7z` archive output (falls back to `zip` if unavailable)

## Build

```bash
./build-mxe.sh              # Incremental build (ccache reused)
CLEAN=1 ./build-mxe.sh      # Clean build
```

## Environment Variables

| Variable | Default | Description |
|---|---|---|
| `BUILD_DIR` | `build-mxe` | CMake build directory |
| `CCACHE_DIR` | `.ccache-mxe` | ccache directory |
| `OUT_DIR` | `out-mxe` | Output directory for archives |
| `DOCKER_IMAGE` | `opensauce04/azahar-build-environment:latest` | Docker image |
| `BUILD_ID` | `azahar-bbp` | Build identifier for filenames |
| `CLEAN` | `0` | Set to `1` for clean build |
| `ARCHIVE_FMT` | `zip` | `zip` or `7z` |
| `ARCHIVE_LV` | `2` | Compression level 0-9 (7z only) |

## Output

```
out-mxe/
├── azahar-bbp.7z          # Bundled archive (azahar.exe + all DLLs)
├── azahar-bbp-sha256.txt  # SHA256 checksums
└── azahar-bbp-build.log   # Full build log
```

## Troubleshooting

**`json.hpp: No such file or directory`**
→ `src/core/CMakeLists.txt` must have `target_link_libraries(citra_core PRIVATE json-headers)` in the `ENABLE_REMOTE_SERVER` block. This fix is already committed.

**`dllwalker` submodule not initialized**
→ The script runs `git submodule update --init --recursive` automatically.

**Docker permission denied**
→ Ensure your user is in the `docker` group: `sudo usermod -aG docker $USER`

**Build takes too long**
→ First build is slow (~1h). Subsequent builds with ccache are much faster (~5-15min).

## Agent Usage

When asked to build this project, load this skill and run:

```bash
cd /path/to/azahar-bbp-Optimization
CLEAN=1 ARCHIVE_FMT=7z ./build-mxe.sh
```

The built artifact will be at `out-mxe/azahar-bbp.7z`.
