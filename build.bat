@echo off
cd /d "%~dp0"
if not exist build mkdir build

pushd build
cl /nologo /O2 /DNDEBUG /Fe:uci-mating-engine.exe ../uci.c
cl /nologo /O2 /DNDEBUG /Fe:mating-engine.exe ../main.c
cl /nologo /O2 /Zi /Oy- /Fe:chess.exe ../main.c
cl /nologo /Od /Zi /Fe:test.exe ../test.c
cl /nologo /O2 /Zi /Fe:test_engine.exe ../test_engine.c
popd
