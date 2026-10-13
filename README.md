# LogWatch — DSA-Driven Log Analysis and Monitoring Tool

**Course:** CSC2101 — Data Structures and Algorithms (July–November 2026)  
**Standard:** ISO C11 (`-std=c11 -Wall -Wextra -pedantic`)  
**Data Provenance Status:** `[SYNTHETIC DATA]` (Offline Laboratory Tool)

LogWatch is an offline, menu-driven CLI application designed to ingest, parse, inspect, sort, and query web application access logs into structured memory records. The project implements foundational Data Structures and Algorithms from scratch using pure ISO C11 with zero external libraries.

---

## 1. Project Purpose and Limitations

### Purpose
- Demonstrate core Data Structures and Algorithms in a realistic systems-programming context.
- Implement dynamically growing contiguous array storage (`LogArray`).
- Implement and empirically analyze elementary sorting algorithms (Bubble Sort with early exit, Selection Sort minimizing swaps, Insertion Sort using record shifts).
- Implement multi-match Linear Search preserving source ordering.
- Provide rigorous empirical operation counting (`SortMetrics`, `SearchMetrics`) and compare with theoretical Big-O complexity.

### Limitations & Gated Scope
- **Offline Only**: LogWatch operates strictly on local file fixtures. It contains zero networking, telemetry, web server, or database components.
- **Strictly Synthetic Data**: All fixtures, demonstrations, and test outputs are explicitly tagged `[SYNTHETIC]`. This project does not process live production traffic.
- **Syllabus Gated**: Divide-and-conquer algorithms (Binary Search, Merge Sort, Quick Sort) and advanced structures (BST, AVL Trees, Heaps, Hash Tables, Graphs) are scheduled post-core and strictly deferred.

---

## 2. Input Schema and Validation Rules

LogWatch ingests pipe-delimited (`|`) log rows structured as follows:

```text
timestamp|route|status_code|response_time_ms|client_id
```

### Field Specifications & Limits
| Field | Type / Limit | Validation Rules |
|---|---|---|
| `timestamp` | `char[32]` | Strict ISO-8601 UTC format (`YYYY-MM-DDTHH:MM:SSZ`). Verified calendar days (month bounds, leap years). |
| `route` | `char[256]` | Non-empty string starting with `/` (e.g. `/api/auth/login`). |
| `status_code` | `int` | Valid HTTP status code between `100` and `599`. |
| `response_time_ms`| `int` | Request duration in milliseconds: integer $\ge 0$. Guarded against overflow. |
| `client_id` | `char[64]` | Non-empty client identifier string (e.g. `client-A`). |

### Line Processing Invariant
The parser strictly classifies every physical line into one of three disjoint categories:
$$\text{total\_lines} = \text{valid\_records} + \text{malformed\_lines} + \text{blank\_lines}$$
Overlong lines ($> 1024$ characters) are safely consumed without buffer overruns.

---

## 3. Interactive CLI Menu Features

The application provides an interactive, menu-driven terminal interface:

1. **Load Log File**: Ingests a specified pipe-delimited file into a contiguous dynamic array.
2. **Preview Records**: Formatted tabular view of loaded records up to a user-specified bound (default: 10 records).
3. **Sort Records**: In-place sorting of complete records by any supported field in ascending or descending order.
4. **Search Records**: Linear scan query engine supporting 5 query types with multi-match extraction.
5. **Display Algorithm Metrics**: Displays comparison and swap/shift counts alongside theoretical asymptotic complexity references.
6. **Show Dataset Statistics**: Ingestion audit displaying total lines, valid records, malformed lines, blank lines, memory footprint, and the accounting invariant check.
7. **Reload Current File**: Re-reads the active dataset. If reloading fails, the previous dataset is preserved intact without data loss.
8. **Exit**: Releases all dynamic memory cleanly and exits with code 0.

---

## 4. Algorithms and Operation Metrics

### Sorting Algorithms
All sorting operations maintain the **Record Integrity Invariant**: every swap or move transfers the **complete `LogEntry` structure (~360 bytes)**, never isolated fields.
- **Bubble Sort**: Sweeps adjacent pairs with an adaptive early-exit flag (`bool swapped`) terminating immediately if a pass completes with 0 swaps.
- **Selection Sort**: Identifies the minimum/maximum record per pass and performs at most one swap per pass ($O(n)$ moves), avoiding redundant self-swaps.
- **Insertion Sort**: Shifts existing records rightward to open a placement slot, preserving all fields of the inserted record.
- **Deterministic Tie-Breaking**: When primary keys are equal, ties are **always broken by ascending `source_line`**, regardless of whether the primary sort direction is ascending or descending.

### Search Engine
- **Linear Search**: Scans the array in $O(n)$ time returning all matching records.
- **Query Capabilities**:
  1. Exact response time (`response_time_ms == target_ms`).
  2. Response-time threshold (`response_time_ms >= threshold_ms`).
  3. Exact HTTP status code (`status_code == target_status`).
  4. Route substring matching.
  5. Route prefix matching.

### Metrics Definitions
- **`comparisons`**: Count of pairwise record comparisons evaluated during sorting or search. Loop boundary checks are not counted.
- **`record_swaps`**: Number of complete two-record exchanges (Bubble & Selection Sort).
- **`record_shifts`**: Number of single-record displacement moves (Insertion Sort).
- **`matches_found`**: Total records satisfying a search query.
- **`elapsed_sec`**: Informational wall-clock duration measured via C11 `timespec_get` (or `clock()` fallback).

---

## 5. Web Frontend: Clone-and-Run Setup

LogWatch features an interactive browser frontend that executes the authoritative ISO C11 engine compiled to WebAssembly. It offers an Ingestion Accounting Overview, a high-density Log Explorer, and an interactive Algorithm Lab (Bubble/Selection/Insertion Sort across 5 keys, 5-mode Linear Search, and a non-destructive 3-way benchmark comparing operations across cloned inputs). A **clone-and-run setup** is enforced: end users and evaluators can clone and launch the web interface using only Node.js and npm without installing Emscripten or a native C compiler.

### 5.1 End-User & Evaluator Workflow

#### Prerequisites
- **Git**
- **Node.js**: Version `20.9` or later (tested on Node.js v24)
- **npm**: (tested on npm 11)
- *No C compiler (`gcc`) or Emscripten SDK (`emcc`) required.*

#### Quickstart (Fresh Clone)
```bash
# 1. Clone repository
git clone https://github.com/SM33-07/LogWatch.git
cd LogWatch/web

# 2. Clean install dependencies from committed lockfile
npm ci

# 3. Launch local development server
npm run dev
```
Open **`http://localhost:3000`** in your browser.

#### Runtime Asset Guarantee & Error Policy
- **Committed Runtime Assets**: The pre-compiled WebAssembly binary (`web/public/wasm/logwatch_wasm.wasm`) and JavaScript loader (`web/public/wasm/logwatch_wasm.js`) are committed directly to the repository and tracked by Git.
- **Deterministic Dependencies**: `web/package-lock.json` is committed to ensure reproducible installations across platforms.
- **Explicit Error Reporting**: If required WebAssembly assets are missing or fail to load, LogWatch immediately surfaces a fatal, descriptive error explaining how to restore or build them.
- **Strict Zero-JS-Fallback Policy**: LogWatch **never** silently substitutes JavaScript implementations for the C sorting, searching, parsing, or dynamic array algorithms. All algorithms execute directly in compiled WebAssembly.

---

### 5.2 Contributor Workflow (WebAssembly & C Bridge)

Contributors who modify the C algorithms or bridge interface must recompile and commit the WebAssembly assets.

#### Contributor Prerequisites
- **Emscripten SDK**: `emcc` 3.1+ or 6.0+ (installed at `C:\emsdk` or available on `PATH`)
- **GCC**: MinGW-w64 or GCC supporting ISO C11 (`-std=c11`)
- **Node.js**: Version `20.9+` and **npm**

#### Contributor Commands (inside `web/`)
```bash
cd web

# 1. Recompile C source files to WebAssembly
npm run build:wasm

# 2. Execute WebAssembly bridge integration tests
npm run test:wasm

# 3. Build production web bundle
npm run build
```

#### Asset Regeneration Contract
Whenever modifying C files in `src/` (`sort.c`, `search.c`, `log_array.c`, `parser.c`, `log_entry.c`), headers in `include/`, or bridge code (`src/wasm_bridge.c`, `include/wasm_bridge.h`):
1. Run `npm run build:wasm` to recompile the C codebase into `web/public/wasm/logwatch_wasm.wasm` and `logwatch_wasm.js`.
2. Run `npm run test:wasm` to verify that all 12 WebAssembly bridge integration tests pass cleanly.
3. Commit the updated `web/public/wasm/` assets to Git so end-users maintain a seamless clone-and-run experience.

---

## 6. Native C11 CLI: Build and Execution (Independent)

The native ISO C11 terminal interface remains completely independent of the web frontend and Node.js toolchain.

### Native Prerequisites
- **GCC** supporting ISO C11 (`gcc -std=c11`)
- Optional: **GNU Make** (`make` or `mingw32-make`)

### Build and Run with Windows Batch (`build.bat`)
```cmd
build.bat all     :: Compiles bin\logwatch.exe
build.bat test    :: Compiles and executes native automated test suites
build.bat clean   :: Cleans all build artifacts under bin\
```

### Build and Run with PowerShell (`build.ps1`)
```powershell
.\build.ps1 -Target all     # Compiles bin\logwatch.exe
.\build.ps1 -Target test    # Compiles and executes native tests + WASM bridge tests
.\build.ps1 -Target clean   # Cleans build artifacts under bin\
```

### Build and Run with GNU Make
```bash
make all          # Compiles bin/logwatch.exe
make test         # Compiles and runs native test suites
make clean        # Removes bin/ directory
```

### Running the Native Application
```cmd
:: Run with interactive file prompt:
bin\logwatch.exe

:: Run with initial file loaded:
bin\logwatch.exe samples\synthetic_access.log
bin\logwatch.exe tests\fixtures\synthetic_valid.log
```

---

## 7. Automated Test Suites

The repository contains 8 automated test suites totaling **81 tests** and **2,329 assertions** with zero warnings under `-std=c11 -Wall -Wextra -pedantic`:

| Suite | Executable / Runner | Tests | Assertions | Focus Area |
|---|---|:---:|:---:|---|
| Smoke Test | `bin/test_dummy.exe` | 3 | 6 | Test harness macros (`ASSERT`, `ASSERT_EQ`, `ASSERT_STR_EQ`) |
| Parser Test | `bin/test_parser.exe` | 22 | 144 | Field tokenization, date validation, line limits, EOF |
| Array Storage Test | `bin/test_array_store.exe` | 10 | 1,145 | Dynamic doubling, cloning, memory safety, safe realloc |
| Sorting Test | `bin/test_sort.exe` | 10 | 630 | Bubble early exit, Selection moves, Insertion shifts, tie-breaking |
| Linear Search Test | `bin/test_search.exe` | 10 | 131 | Exact, threshold, status, prefix/substring, buffer bounds, aliasing |
| Interactive CLI Test | `bin/test_cli.exe` | 7 | 74 | In-memory stream test of menu loop, error recovery, scripted stream, EOF |
| Executable Acceptance | `bin/test_cli_executable.exe` | 7 | 12 | End-to-end OS process tests of `bin/logwatch.exe` via stdin pipes across all fixtures |
| WebAssembly Bridge | `node tests/test_wasm_bridge.js` | 12 | 187 | C-to-WASM bridge ABI, VFS file loading, record integrity, JSON escaping, zero-leak contract |

