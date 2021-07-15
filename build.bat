@echo off

set DEFINES=/DGJ_DEBUG
set COMPILE_FLAGS=/nologo /FC /Z7 /FA /Od
set INCLUDES=/I..\gj
set LINKER_FLAGS=/incremental:no /opt:icf /opt:ref

set SOURCES=..\win32_json.c
set EXECUTABLE=/Fewin32_json.exe

mkdir build
pushd build
cl %DEFINES% %COMPILE_FLAGS% %INCLUDES% %SOURCES% %EXECUTABLE% /link %LINKER_FLAGS% /subsystem:console
popd
