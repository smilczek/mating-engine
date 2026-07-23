#!/bin/bash

cd $(dirname "$0")
mkdir -p build
gcc -O0 -g main.c -o build/chess
