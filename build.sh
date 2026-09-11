#!/bin/sh
set -eu
cd "$(dirname "$0")"
source_dir=$(realpath "${1:?Usage: ./build.sh /path/to/official/OpenRGB}")
expected=5e81e26fcc65d3dacfb76b0a30ec0142ec7bb131
test "$(git -C "$source_dir" rev-parse HEAD)" = "$expected" || {
    echo "Use official OpenRGB revision $expected (release 1.0rc3.1, plugin API 4)." >&2
    exit 1
}
mkdir -p build
cd build
qmake ../FractalAdjustProPlugin.pro "OPENRGB_SOURCE=$source_dir"
make -j4
