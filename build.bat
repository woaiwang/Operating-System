@echo off
setlocal enabledelayedexpansion

set "CC=gcc"
set "CFLAGS=-std=c99 -Wall -Wextra -Iinclude"
set "SRCDIR=src"
set "BUILDDIR=build"
set "TARGET=%BUILDDIR%\fs.exe"

mkdir "%BUILDDIR%" 2>nul

set "OBJS="
for %%f in (%SRCDIR%\*.c) do (
    set "obj=%BUILDDIR%\%%~nf.o"
    set "OBJS=!OBJS! !obj!"
    echo Compiling %%~nf.c...
    %CC% %CFLAGS% -c "%%f" -o "!obj!"
    if errorlevel 1 (
        echo Error compiling %%~nf.c
        pause
        exit /b 1
    )
)

echo Linking...
%CC% %CFLAGS% %OBJS% -o %TARGET%
if errorlevel 1 (
    echo Error linking
    pause
    exit /b 1
)

echo Build successful!
pause