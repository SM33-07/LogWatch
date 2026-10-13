# LogWatch - PowerShell Build Script
param (
    [string]$Target = "all"
)

$ErrorActionPreference = "Stop"

# Ensure bin directory exists
if (-not (Test-Path "bin")) {
    New-Item -ItemType Directory -Path "bin" | Out-Null
}

# Locate GCC compiler
$gccCmd = Get-Command "gcc" -ErrorAction SilentlyContinue
if (-not $gccCmd) {
    if (Test-Path "C:\msys64\ucrt64\bin\gcc.exe") {
        $env:PATH = "C:\msys64\ucrt64\bin;$($env:PATH)"
    } elseif (Test-Path "C:\MinGW\bin\gcc.exe") {
        $env:PATH = "C:\MinGW\bin;$($env:PATH)"
    } else {
        Write-Error "[ERROR] GCC compiler not found in PATH or standard installation directories."
        exit 1
    }
}

$cflags = @("-std=c11", "-Wall", "-Wextra", "-pedantic", "-Iinclude", "-Itests")

function Invoke-EmccSetup {
    if (-not (Get-Command "emcc" -ErrorAction SilentlyContinue)) {
        if (Test-Path "C:\emsdk\emsdk_env.ps1") {
            & "C:\emsdk\emsdk_env.ps1"
        } else {
            Write-Error "[ERROR] Emscripten (emcc) not found and C:\emsdk\emsdk_env.ps1 is missing."
            exit 1
        }
    }
}

$appSrcFiles = @("src/main.c", "src/cli.c", "src/sort.c", "src/search.c", "src/log_array.c", "src/log_entry.c", "src/parser.c")

switch ($Target.ToLower()) {
    "clean" {
        if (Test-Path "bin") {
            Remove-Item -Recurse -Force "bin"
            Write-Host "[INFO] Cleaned build directory `"bin/`"."
        }
        exit 0
    }
    "wasm" {
        Invoke-EmccSetup
        if (-not (Test-Path "web/public/wasm")) {
            New-Item -ItemType Directory -Path "web/public/wasm" -Force | Out-Null
        }
        if (-not (Test-Path "web/lib/wasm")) {
            New-Item -ItemType Directory -Path "web/lib/wasm" -Force | Out-Null
        }
        Write-Host "[BUILD] Compiling C WebAssembly bridge via emcc..."
        $wasmSrc = @("src/wasm_bridge.c", "src/log_array.c", "src/log_entry.c", "src/parser.c", "src/sort.c", "src/search.c")
        $exportedFuncs = "['_bridge_init','_bridge_load_from_vfs','_bridge_load_file','_bridge_clear','_bridge_free','_bridge_get_size','_bridge_get_capacity','_bridge_get_stats_json','_bridge_get_records_json','_bridge_sort','_bridge_compare_all_sorts','_bridge_search_response_time','_bridge_search_threshold','_bridge_search_status','_bridge_search_route','_bridge_free_string','_malloc','_free']"
        $exportedMethods = "['ccall','cwrap','UTF8ToString','stringToUTF8','FS','lengthBytesUTF8']"
        & emcc -std=c11 -Wall -Wextra -pedantic -Iinclude -O2 $wasmSrc `
            -s MODULARIZE=1 -s EXPORT_NAME="createLogWatchModule" `
            -s ALLOW_MEMORY_GROWTH=1 -s FORCE_FILESYSTEM=1 -s NO_EXIT_RUNTIME=1 `
            -s "EXPORTED_FUNCTIONS=$exportedFuncs" `
            -s "EXPORTED_RUNTIME_METHODS=$exportedMethods" `
            -o "web/public/wasm/logwatch_wasm.js"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[ERROR] WebAssembly compilation failed."
            exit $LASTEXITCODE
        }
        Copy-Item "web/public/wasm/*" "web/lib/wasm/" -Force
        Write-Host "[INFO] WebAssembly bridge built: web/public/wasm/logwatch_wasm.js and .wasm"
        exit 0
    }
    "test-wasm" {
        Invoke-EmccSetup
        if (-not (Test-Path "web/lib/wasm/logwatch_wasm.js")) {
            & $PSCommandPath -Target wasm
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
        Write-Host "[RUN] Executing WebAssembly integration test suite..."
        & node "tests/test_wasm_bridge.js"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[FAIL] WebAssembly integration tests failed."
            exit $LASTEXITCODE
        }
        exit 0
    }
    "test" {
        Write-Host "[BUILD] Compiling smoke test runner..."
        & gcc $cflags "tests/test_dummy.c" -o "bin/test_dummy.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[ERROR] Smoke test compilation failed."
            exit $LASTEXITCODE
        }

        Write-Host "[BUILD] Compiling parser test runner..."
        & gcc $cflags "tests/test_parser.c" "src/log_entry.c" "src/parser.c" -o "bin/test_parser.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[ERROR] Parser test compilation failed."
            exit $LASTEXITCODE
        }

        Write-Host "[BUILD] Compiling dynamic array test runner..."
        & gcc $cflags "tests/test_array_store.c" "src/log_entry.c" "src/log_array.c" "src/parser.c" -o "bin/test_array_store.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[ERROR] Dynamic array test compilation failed."
            exit $LASTEXITCODE
        }

        Write-Host "[BUILD] Compiling sorting test runner..."
        & gcc $cflags "tests/test_sort.c" "src/sort.c" "src/log_array.c" "src/log_entry.c" "src/parser.c" -o "bin/test_sort.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[ERROR] Sorting test compilation failed."
            exit $LASTEXITCODE
        }

        Write-Host "[BUILD] Compiling search test runner..."
        & gcc $cflags "tests/test_search.c" "src/search.c" "src/log_array.c" "src/log_entry.c" "src/parser.c" -o "bin/test_search.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[ERROR] Search test compilation failed."
            exit $LASTEXITCODE
        }

        Write-Host "[BUILD] Compiling CLI test runner..."
        & gcc $cflags "tests/test_cli.c" "src/cli.c" "src/sort.c" "src/search.c" "src/log_array.c" "src/log_entry.c" "src/parser.c" -o "bin/test_cli.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[ERROR] CLI test compilation failed."
            exit $LASTEXITCODE
        }

        if (-not (Test-Path "bin/logwatch.exe")) {
            Write-Host "[BUILD] Compiling logwatch application for executable acceptance tests..."
            & gcc $cflags $appSrcFiles -o "bin/logwatch.exe"
            if ($LASTEXITCODE -ne 0) {
                Write-Error "[ERROR] Application compilation failed."
                exit $LASTEXITCODE
            }
        }

        Write-Host "[BUILD] Compiling executable acceptance test runner..."
        & gcc $cflags "tests/test_cli_executable.c" -o "bin/test_cli_executable.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[ERROR] Executable acceptance test compilation failed."
            exit $LASTEXITCODE
        }

        Write-Host "[RUN] Executing smoke test runner..."
        & "bin/test_dummy.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[FAIL] Smoke test suite failed."
            exit $LASTEXITCODE
        }

        Write-Host "[RUN] Executing parser test runner..."
        & "bin/test_parser.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[FAIL] Parser test suite failed."
            exit $LASTEXITCODE
        }

        Write-Host "[RUN] Executing dynamic array test runner..."
        & "bin/test_array_store.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[FAIL] Dynamic array test suite failed."
            exit $LASTEXITCODE
        }

        Write-Host "[RUN] Executing sorting test runner..."
        & "bin/test_sort.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[FAIL] Sorting test suite failed."
            exit $LASTEXITCODE
        }

        Write-Host "[RUN] Executing search test runner..."
        & "bin/test_search.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[FAIL] Search test suite failed."
            exit $LASTEXITCODE
        }

        Write-Host "[RUN] Executing CLI test runner..."
        & "bin/test_cli.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[FAIL] CLI test suite failed."
            exit $LASTEXITCODE
        }

        Write-Host "[RUN] Executing executable acceptance test runner..."
        & "bin/test_cli_executable.exe"
        if ($LASTEXITCODE -ne 0) {
            Write-Error "[FAIL] Executable acceptance test suite failed."
            exit $LASTEXITCODE
        }

        if (Test-Path "web/lib/wasm/logwatch_wasm.js") {
            Write-Host "[RUN] Executing WebAssembly integration test suite..."
            & node "tests/test_wasm_bridge.js"
            if ($LASTEXITCODE -ne 0) {
                Write-Error "[FAIL] WebAssembly integration tests failed."
                exit $LASTEXITCODE
            }
        }
        exit 0
    }
    "all" {
        Write-Host "[BUILD] Checking Milestone status..."
        if (Test-Path "src/main.c") {
            Write-Host "[BUILD] Compiling logwatch application..."
            & gcc $cflags $appSrcFiles -o "bin/logwatch.exe"
            if ($LASTEXITCODE -ne 0) {
                Write-Error "[ERROR] Application compilation failed."
                exit $LASTEXITCODE
            }
            Write-Host "[INFO] Application built successfully: bin/logwatch.exe"
        } else {
            Write-Host "[INFO] Core CLI application deferred."
        }
        exit 0
    }
    default {
        Write-Error "[ERROR] Unknown build target: $Target. Usage: .\build.ps1 [-Target all|test|clean|wasm|test-wasm]"
        exit 1
    }
}
