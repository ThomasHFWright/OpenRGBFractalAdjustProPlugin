#!/bin/sh
set -eu
cd "$(dirname "$0")"
mkdir -p build
c++ -std=c++17 -Wall -Wextra -Werror -pthread -Isrc \
    $(pkg-config --cflags hidapi-hidraw) \
    tests/test_protocol.cpp src/FractalAdjustProController.cpp \
    -o build/test_protocol
./build/test_protocol
