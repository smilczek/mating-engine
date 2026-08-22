#!/bin/bash

cd $(dirname "$0")
mkdir -p build

gcc -O0 -g test_bitboard.c -o build/test_bitboard
gcc -O3 -g test_engine.c -o build/test_engine
