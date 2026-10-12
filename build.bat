@echo off
setlocal enabledelayedexpansion

REM Ensure bin directory exists
if not exist "bin" mkdir "bin"

REM Detect gcc compiler
where gcc >nul 2>nul
if %ERRORLEVEL% neq 0 (
    if exist "C:\msys64\ucrt64\bin\gcc.exe" (
        set "PATH=C:\msys64\ucrt64\bin;!PATH!"
    ) else if exist "C:\MinGW\bin\gcc.exe" (
        set "PATH=C:\MinGW\bin;!PATH!"
    ) else (
        echo [ERROR] GCC compiler not found in PATH or standard installation directories.
        exit /b 1
    )
)

set CFLAGS=-std=c11 -Wall -Wextra -pedantic -Iinclude -Itests
set TARGET=%1
if "%TARGET%"=="" set TARGET=all

if /i "%TARGET%"=="clean" (
    if exist "bin" (
        rmdir /s /q "bin"
        echo [INFO] Cleaned build directory "bin\".
    )
    exit /b 0
)

if /i "%TARGET%"=="test" (
    echo [BUILD] Compiling smoke test runner...
    gcc %CFLAGS% tests\test_dummy.c -o bin\test_dummy.exe
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] Smoke test compilation failed.
        exit /b 1
    )

    echo [BUILD] Compiling parser test runner...
    gcc %CFLAGS% tests\test_parser.c src\log_entry.c src\parser.c -o bin\test_parser.exe
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] Parser test compilation failed.
        exit /b 1
    )

    echo [BUILD] Compiling dynamic array test runner...
    gcc %CFLAGS% tests\test_array_store.c src\log_entry.c src\log_array.c src\parser.c -o bin\test_array_store.exe
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] Dynamic array test compilation failed.
        exit /b 1
    )

    echo [RUN] Executing smoke test runner...
    bin\test_dummy.exe
    if !ERRORLEVEL! neq 0 (
        echo [FAIL] Smoke test suite failed.
        exit /b 1
    )

    echo [RUN] Executing parser test runner...
    bin\test_parser.exe
    if !ERRORLEVEL! neq 0 (
        echo [FAIL] Parser test suite failed.
        exit /b 1
    )

    echo [RUN] Executing dynamic array test runner...
    bin\test_array_store.exe
    exit /b !ERRORLEVEL!
)

if /i "%TARGET%"=="all" (
    echo [BUILD] Checking Milestone status...
    if exist "src\main.c" (
        echo [BUILD] Compiling logwatch application...
        gcc %CFLAGS% src\*.c -o bin\logwatch.exe
        if !ERRORLEVEL! neq 0 (
            echo [ERROR] Application compilation failed.
            exit /b 1
        )
        echo [INFO] Application built successfully: bin\logwatch.exe
    ) else (
        echo [INFO] Milestone 3 active (Data Model, Parser, and Dynamic Array implemented). Core CLI application deferred to Milestone 6.
    )
    exit /b 0
)

echo [ERROR] Unknown build target: %TARGET%
echo Usage: build.bat [all^|test^|clean]
exit /b 1
