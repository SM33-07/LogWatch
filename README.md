# LogWatch — DSA-Driven Log Analysis and Monitoring Tool

**Course:** CSC2101 — Data Structures and Algorithms (July–November 2026)  
**Standard:** ISO C11 (`-std=c11 -Wall -Wextra -pedantic`)  
**Data Status:** `[SYNTHETIC DATA]`

LogWatch is an offline, menu-driven CLI tool that parses web application access logs into structured memory records to provide diagnostic insights (response times, error frequencies, route traffic) using foundational Data Structures and Algorithms implemented from scratch.

---

## Project Structure

```text
LogWatch/
├── AGENTS.md                  # Workspace operational rules & constraints
├── README.md                  # Project overview and build instructions
├── Makefile                   # GNU Make build script
├── build.bat                  # Windows CMD build runner
├── build.ps1                  # PowerShell build runner
├── .gitignore                 # Build artifact exclusions
├── bin/                       # Compiled binaries and test executables
├── docs/
│   ├── PRD_SRS.md             # Full product and software requirements
│   ├── IMPLEMENTATION_PLAN.md # Phased architecture and roadmap
│   ├── DECISIONS.md           # Architectural decision log
│   └── TEST_PLAN.md           # Comprehensive test cases and matrices
├── include/                   # Header files (C11)
├── src/                       # Module implementations
├── tests/
│   ├── test_utils.h           # Custom C11 test harness macros
│   ├── test_dummy.c           # Infrastructure smoke test runner
│   └── fixtures/              # Synthetic test fixtures
│       ├── synthetic_valid.log
│       ├── synthetic_malformed.log
│       ├── synthetic_empty.log
│       ├── synthetic_single.log
│       ├── synthetic_duplicates.log
│       └── synthetic_blanks.log
└── samples/
    └── synthetic_access.log   # 50-row demonstration log fixture
```

---

## Build and Run

### Prerequisites
- GCC supporting C11 (`-std=c11`)
- Optional: GNU Make (`make` / `mingw32-make`)

### Building on Windows (CMD / Batch)
```cmd
build.bat all     :: Check build status / build application
build.bat test    :: Compile and run test suite
build.bat clean   :: Clean build artifacts in bin/
```

### Building on Windows (PowerShell)
```powershell
.\build.ps1 -Target all
.\build.ps1 -Target test
.\build.ps1 -Target clean
```

### Building with Make (Cross-Platform)
```bash
make all          # Build target
make test         # Build and run automated tests
make clean        # Remove bin/ directory
```

---

## Current Status

- **Milestone 1**: Project skeleton, test harness, synthetic fixtures, and build scripts initialized.
- Application logic (Parser, Dynamic Array, Sorting, Searching, and CLI) is scheduled for subsequent milestones according to `docs/IMPLEMENTATION_PLAN.md`.
