#!/usr/bin/env bash
set -euo pipefail

# Simple build+run helper for Arch/Linux using vcpkg in $HOME

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TOOLCHAIN="$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake"
TRIPLET="x64-linux"
CONFIG=${CONFIG:-Debug}

echo "[run.sh] Config: $CONFIG, Triplet: $TRIPLET"

# Ensure vcpkg exists
if [[ ! -d "$HOME/vcpkg" ]]; then
  echo "[run.sh] vcpkg not found in ~. Cloning and bootstrapping..."
  git clone https://github.com/microsoft/vcpkg "$HOME/vcpkg"
  "$HOME/vcpkg/bootstrap-vcpkg.sh"
fi

mkdir -p "$BUILD_DIR"

echo "[run.sh] Configuring CMake..."
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE="$CONFIG" \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
  -DVCPKG_TARGET_TRIPLET="$TRIPLET" \
  -DOpenGL_GL_PREFERENCE=GLVND

echo "[run.sh] Building..."
cmake --build "$BUILD_DIR" --config "$CONFIG"

BIN="$BUILD_DIR/ImGuiOpenGLProject"
if [[ ! -x "$BIN" ]]; then
  echo "[run.sh] ERROR: Binary not found at $BIN" >&2
  exit 1
fi

echo "[run.sh] Running $BIN"
exec "$BIN"
