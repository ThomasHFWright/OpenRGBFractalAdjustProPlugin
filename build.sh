#!/bin/sh
set -eu
cd "$(dirname "$0")"
source_dir=$(realpath "${1:?Usage: ./build.sh /path/to/official/OpenRGB}")
expected=728846f66861dd1cb7dc04835f651830d6ef13ce
test "$(git -C "$source_dir" rev-parse HEAD)" = "$expected" || {
    echo "Use official OpenRGB revision $expected (plugin API 5)." >&2
    exit 1
}
mkdir -p build
cd build
qmake ../FractalAdjustProPlugin.pro "OPENRGB_SOURCE=$source_dir"
make -j4
