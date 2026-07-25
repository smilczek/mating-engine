#!/bin/bash

cd $(dirname "$0")
mkdir -p build
gcc main.c -o build/chess
gcc -O0 -g test.c -o build/test
