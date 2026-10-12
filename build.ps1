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

switch ($Target.ToLower()) {
    "clean" {
        if (Test-Path "bin") {
            Remove-Item -Recurse -Force "bin"
            Write-Host "[INFO] Cleaned build directory `"bin/`"."
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
        exit $LASTEXITCODE
    }
    "all" {
        Write-Host "[BUILD] Checking Milestone status..."
        if (Test-Path "src/main.c") {
            Write-Host "[BUILD] Compiling logwatch application..."
            $srcFiles = Get-ChildItem "src/*.c" | Select-Object -ExpandProperty FullName
            & gcc $cflags $srcFiles -o "bin/logwatch.exe"
            if ($LASTEXITCODE -ne 0) {
                Write-Error "[ERROR] Application compilation failed."
                exit $LASTEXITCODE
            }
            Write-Host "[INFO] Application built successfully: bin/logwatch.exe"
        } else {
            Write-Host "[INFO] Milestone 3 active (Data Model, Parser, and Dynamic Array implemented). Core CLI application deferred to Milestone 6."
        }
        exit 0
    }
    default {
        Write-Error "[ERROR] Unknown build target: $Target. Usage: .\build.ps1 [-Target all|test|clean]"
        exit 1
    }
}
