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

if /i "%TARGET%"=="wasm" (
    where emcc >nul 2>nul
    if !ERRORLEVEL! neq 0 (
        if exist "C:\emsdk\emsdk_env.bat" (
            call "C:\emsdk\emsdk_env.bat"
        ) else (
            echo [ERROR] Emscripten ^(emcc^) not found in PATH or C:\emsdk.
            exit /b 1
        )
    )
    if not exist "web\public\wasm" mkdir "web\public\wasm"
    if not exist "web\lib\wasm" mkdir "web\lib\wasm"
    echo [BUILD] Compiling C WebAssembly bridge via emcc...
    emcc -std=c11 -Wall -Wextra -pedantic -Iinclude -O2 src\wasm_bridge.c src\log_array.c src\log_entry.c src\parser.c src\sort.c src\search.c -s MODULARIZE=1 -s EXPORT_NAME="createLogWatchModule" -s ALLOW_MEMORY_GROWTH=1 -s FORCE_FILESYSTEM=1 -s NO_EXIT_RUNTIME=1 -s "EXPORTED_FUNCTIONS=['_bridge_init','_bridge_load_from_vfs','_bridge_load_file','_bridge_clear','_bridge_free','_bridge_get_size','_bridge_get_capacity','_bridge_get_stats_json','_bridge_get_records_json','_bridge_sort','_bridge_compare_all_sorts','_bridge_search_response_time','_bridge_search_threshold','_bridge_search_status','_bridge_search_route','_bridge_free_string','_malloc','_free']" -s "EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','FS','lengthBytesUTF8']" -o web\public\wasm\logwatch_wasm.js
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] WebAssembly compilation failed.
        exit /b 1
    )
    copy /y "web\public\wasm\*" "web\lib\wasm\" >nul
    echo [INFO] WebAssembly bridge built: web\public\wasm\logwatch_wasm.js and .wasm
    exit /b 0
)

if /i "%TARGET%"=="test-wasm" (
    if not exist "web\lib\wasm\logwatch_wasm.js" (
        call "%~f0" wasm
        if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!
    )
    echo [RUN] Executing WebAssembly integration test suite...
    node tests\test_wasm_bridge.js
    if !ERRORLEVEL! neq 0 (
        echo [FAIL] WebAssembly integration tests failed.
        exit /b 1
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

    echo [BUILD] Compiling sorting test runner...
    gcc %CFLAGS% tests\test_sort.c src\sort.c src\log_array.c src\log_entry.c src\parser.c -o bin\test_sort.exe
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] Sorting test compilation failed.
        exit /b 1
    )

    echo [BUILD] Compiling search test runner...
    gcc %CFLAGS% tests\test_search.c src\search.c src\log_array.c src\log_entry.c src\parser.c -o bin\test_search.exe
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] Search test compilation failed.
        exit /b 1
    )

    echo [BUILD] Compiling CLI test runner...
    gcc %CFLAGS% tests\test_cli.c src\cli.c src\sort.c src\search.c src\log_array.c src\log_entry.c src\parser.c -o bin\test_cli.exe
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] CLI test compilation failed.
        exit /b 1
    )

    if not exist "bin\logwatch.exe" (
        echo [BUILD] Compiling logwatch application for executable acceptance tests...
        gcc %CFLAGS% src\main.c src\cli.c src\sort.c src\search.c src\log_array.c src\log_entry.c src\parser.c -o bin\logwatch.exe
        if !ERRORLEVEL! neq 0 (
            echo [ERROR] Application compilation failed.
            exit /b 1
        )
    )

    echo [BUILD] Compiling executable acceptance test runner...
    gcc %CFLAGS% tests\test_cli_executable.c -o bin\test_cli_executable.exe
    if !ERRORLEVEL! neq 0 (
        echo [ERROR] Executable acceptance test compilation failed.
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
    if !ERRORLEVEL! neq 0 (
        echo [FAIL] Dynamic array test suite failed.
        exit /b 1
    )

    echo [RUN] Executing sorting test runner...
    bin\test_sort.exe
    if !ERRORLEVEL! neq 0 (
        echo [FAIL] Sorting test suite failed.
        exit /b 1
    )

    echo [RUN] Executing search test runner...
    bin\test_search.exe
    if !ERRORLEVEL! neq 0 (
        echo [FAIL] Search test suite failed.
        exit /b 1
    )

    echo [RUN] Executing CLI test runner...
    bin\test_cli.exe
    if !ERRORLEVEL! neq 0 (
        echo [FAIL] CLI test suite failed.
        exit /b 1
    )

    echo [RUN] Executing executable acceptance test runner...
    bin\test_cli_executable.exe
    if !ERRORLEVEL! neq 0 exit /b !ERRORLEVEL!

    if exist "web\lib\wasm\logwatch_wasm.js" (
        echo [RUN] Executing WebAssembly integration test suite...
        node tests\test_wasm_bridge.js
        if !ERRORLEVEL! neq 0 (
            echo [FAIL] WebAssembly integration tests failed.
            exit /b 1
        )
    )
    exit /b 0
)

if /i "%TARGET%"=="all" (
    echo [BUILD] Checking Milestone status...
    if exist "src\main.c" (
        echo [BUILD] Compiling logwatch application...
        gcc %CFLAGS% src\main.c src\cli.c src\sort.c src\search.c src\log_array.c src\log_entry.c src\parser.c -o bin\logwatch.exe
        if !ERRORLEVEL! neq 0 (
            echo [ERROR] Application compilation failed.
            exit /b 1
        )
        echo [INFO] Application built successfully: bin\logwatch.exe
    ) else (
        echo [INFO] Milestone 3 active. Core CLI application deferred.
    )
    exit /b 0
)

echo [ERROR] Unknown build target: %TARGET%
echo Usage: build.bat [all^|test^|clean^|wasm^|test-wasm]
exit /b 1
