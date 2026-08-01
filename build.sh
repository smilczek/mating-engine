#!/bin/bash

cd $(dirname "$0")
mkdir -p build
gcc -O2 -g -fno-omit-frame-pointer main.c -o build/chess
gcc -O0 -g test.c -o build/test
gcc -O2 -g test_engine.c -o build/test_engine
