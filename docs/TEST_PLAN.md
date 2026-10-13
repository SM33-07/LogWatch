# LogWatch — Test Plan

**Project:** LogWatch (CSC2101 DSA Lab Project)  
**Last updated:** 2026-10-12  
**Status:** Planning Phase — Revised for Scoped Core Release  
**Test Framework:** Custom `tests/test_utils.h` macros (Standard C11, no external libraries)

---

## 1. Test Conventions and Verification Methodology

- **Status Definitions**:
  - `DRAFT`: Specified, pre-implementation (all tests start here).
  - `PASS`: Binary compiled, run, asserted with 0 failures and 0 memory leaks.
  - `FAIL`: Assertion failure or crash observed.
  - `DEFERRED`: Optional extension test gated by syllabus confirmation.
- **Record Integrity Invariant**: A sort test is only considered passing if, in addition to sorted keys, every secondary field (`route`, `status_code`, `client_id`, `source_line`) remains paired with its originating record.
- **Zero Third-Party Dependencies**: Tests run via native executables (`bin/test_parser`, `bin/test_sort`, etc.) returning exit code 0 on success.

---

## 2. Test Fixtures (All Synthetic)

All test files are stored in `tests/fixtures/` and explicitly tagged `[SYNTHETIC]`.

| Fixture File | Total Lines | Valid Rows | Malformed Rows | Blank Lines | Testing Purpose |
|---|---|---|---|---|---|
| `synthetic_valid.log` | 5 | 5 | 0 | 0 | Baseline parsing and happy path. |
| `synthetic_malformed.log` | 9 | 3 | 6 | 0 | Resiliency against corrupted data. |
| `synthetic_empty.log` | 0 | 0 | 0 | 0 | Empty file boundary handling. |
| `synthetic_single.log` | 1 | 1 | 0 | 0 | Single element sorting and searching. |
| `synthetic_duplicates.log` | 5 | 5 | 0 | 0 | Duplicate response times and tie-breaking. |
| `synthetic_blanks.log` | 6 | 3 | 0 | 3 | Toleration of blank/whitespace lines. |

---

## 3. Core Release Test Suites

### 3.1 Parser Test Suite (`test_parser.c`)

| ID | Description | Input / Condition | Expected Output | Status |
|---|---|---|---|---|
| **P-01** | Parse well-formed 5-field row | `"2026-10-09T10:00:00Z\|/api/snippets\|200\|42\|client-A"` | Returns `true`; all fields match target struct | PASS |
| **P-02** | Full fixture parse | `tests/fixtures/synthetic_valid.log` | `valid_records == 5`, `malformed_lines == 0`, `blank_lines == 0` | PASS |
| **P-03** | Missing delimiter / fields | `"2026-10-09T10:00:00Z\|/api/snippets\|200"` | Returns `false`; `malformed_lines` increments | PASS |
| **P-04** | Extra delimiter / fields | `"2026-10-09T10:00:00Z\|/api/snippets\|200\|42\|c-1\|extra"` | Returns `false`; `malformed_lines` increments | PASS |
| **P-05** | Non-numeric status code | `"2026-10-09T10:00:00Z\|/api/snippets\|STATUS_OK\|42\|c-1"` | Returns `false`; `malformed_lines` increments | PASS |
| **P-06** | Out-of-bounds status code (high) | `"2026-10-09T10:00:00Z\|/api/snippets\|700\|42\|c-1"` | Returns `false` (valid range 100–599) | PASS |
| **P-07** | Out-of-bounds status code (low) | `"2026-10-09T10:00:00Z\|/api/snippets\|99\|42\|c-1"` | Returns `false` (valid range 100–599) | PASS |
| **P-08** | Negative response time | `"2026-10-09T10:00:00Z\|/api/snippets\|200\|-15\|c-1"` | Returns `false` (response time must be $\ge 0$) | PASS |
| **P-09** | Empty route string | `"2026-10-09T10:00:00Z\|\|200\|42\|c-1"` | Returns `false`; `malformed_lines` increments | PASS |
| **P-10** | Route not starting with `/` | `"2026-10-09T10:00:00Z\|api/snippets\|200\|42\|c-1"` | Returns `false`; `malformed_lines` increments | PASS |
| **P-11** | Blank & whitespace lines | `tests/fixtures/synthetic_blanks.log` | `valid_records == 3`, `blank_lines == 3`, `malformed_lines == 0` | PASS |
| **P-12** | Corrupted mixed fixture | `tests/fixtures/synthetic_malformed.log`| `valid_records == 3`, `malformed_lines == 6`, zero crashes | PASS |
| **P-13** | Non-existent file | `"non_existent_file.log"` | Returns `false`; error reported gracefully | PASS |

### 3.2 Dynamic Array Test Suite (`test_array_store.c`)

| ID | Description | Input / Condition | Expected Output | Status |
|---|---|---|---|---|
| **A-01** | Initialize array | `initial_capacity = 16` | `size == 0`, `capacity == 16`, `entries != NULL` | PASS |
| **A-02** | Append single record | Valid `LogEntry` | `size == 1`, `entries[0]` matches entry | PASS |
| **A-03** | Dynamic capacity growth | Append 17 items into capacity 16 | `size == 17`, `capacity == 32`, zero memory corruption | PASS |
| **A-04** | Large append sequence | Append 1000 items | `size == 1000`, `capacity >= 1000`, all elements accessible | PASS |
| **A-05** | Clone array | Source array with 5 records | Cloned array has identical records, distinct heap pointers | PASS |
| **A-06** | Free array | Allocated `LogArray` | Heap block freed, pointer reset to `NULL`, `size == 0` | PASS |
| **A-07** | Clone empty array | Source array with 0 records | Cloned array initialized with 0 size, valid state | PASS |

### 3.3 Sorting & Record Integrity Test Suite (`test_sort.c`)

| ID | Description | Input / Condition | Expected Output | Status |
|---|---|---|---|---|
| **S-01** | Bubble Sort correctness | Unsorted `synthetic_valid.log` by response time | Records in non-decreasing order: 18, 42, 95, 310, 1200 | PASS |
| **S-02** | Selection Sort correctness | Unsorted `synthetic_valid.log` by response time | Records in non-decreasing order: 18, 42, 95, 310, 1200 | PASS |
| **S-03** | Insertion Sort correctness | Unsorted `synthetic_valid.log` by response time | Records in non-decreasing order: 18, 42, 95, 310, 1200 | PASS |
| **S-04** | **Record Integrity Check (Bubble)** | Sort `synthetic_valid.log` by response time | For record 18: `route == "/api/auth/login"`, `status == 401`, `client == "client-C"`. Association strictly preserved! | PASS |
| **S-05** | **Record Integrity Check (Select)** | Sort `synthetic_valid.log` by response time | Complete struct fields match source record for every item | PASS |
| **S-06** | **Record Integrity Check (Insert)** | Sort `synthetic_valid.log` by response time | Complete struct fields match source record for every item | PASS |
| **S-07** | Empty array boundary | Array with 0 records | Sort completes safely without null dereference | PASS |
| **S-08** | Single element boundary | Array with 1 record | Sort completes safely, record unchanged | PASS |
| **S-09** | Already-sorted array | 5 sorted records | Array remains sorted; Bubble Sort exits in 1 pass ($n-1$ comparisons, 0 swaps) | PASS |
| **S-10** | Reverse-sorted array | 5 reverse-sorted records | Array correctly sorted in ascending order | PASS |
| **S-11** | Duplicate keys handling | `synthetic_duplicates.log` (100, 100, 200, 100, 200) | Sorted: 100, 100, 100, 200, 200. Zero records lost. Stable tie-break by `source_line`. | PASS |
| **S-12** | Sort by HTTP Status Code | `synthetic_valid.log` by status | Sorted: 200, 200, 401, 500, 503 | PASS |
| **S-13** | Sort by Timestamp | `synthetic_valid.log` by timestamp | Chronological order maintained | PASS |
| **S-14** | Descending sort order | `synthetic_valid.log`, `ascending = false` | Sorted in non-increasing order: 1200, 310, 95, 42, 18 | PASS |
| **S-15** | Selection Sort moves count | Array of size $n$ | `SortMetrics.record_swaps <= n - 1` ($O(n)$ swaps; 0 on sorted) | PASS |
| **S-16** | Insertion Sort best-case moves | Already-sorted array of size $n$ | `SortMetrics.record_shifts == 0`, `comparisons == n - 1` | PASS |
| **S-17** | Non-destructive comparison | Run Bubble, Selection, and Insertion Sort on clones of same array | All 3 sorts produce identical ordered output from fresh copies | PASS |

### 3.4 Linear Search Test Suite (`test_search.c`)

| ID | Description | Input / Condition | Expected Output | Status |
|---|---|---|---|---|
| **L-01** | Search exact response time (single match) | Target: 95ms in `synthetic_valid.log` | `matches == 1`, index points to `/api/snippets/42` | PASS |
| **L-02** | Search exact response time (multi match) | Target: 100ms in `synthetic_duplicates.log` | `matches == 3`, indices point to lines 1, 2, 4 | PASS |
| **L-03** | Search response time not found | Target: 9999ms | `matches == 0`, buffer untouched | PASS |
| **L-04** | Search by status code | Target: 500 in `synthetic_valid.log` | `matches == 1`, `/api/snippets` 500 found | PASS |
| **L-05** | Search by route prefix/substring | Target: `"/snippets"` | `matches == 3`, all matching route records returned | PASS |
| **L-06** | Search empty array | Array with 0 records | `matches == 0`, safe return | PASS |
| **L-07** | Search metrics comparison count | Unsorted array of $n$ elements | `SearchMetrics.comparisons == n` (full linear scan) | PASS |

### 3.5 Memory Safety and System Resilience

| ID | Description | Tool / Condition | Expected Output | Status |
|---|---|---|---|---|
| **M-01** | Parser memory cleanup | Defensive code audit & `test_parser` fixture execution | `parse_stats_free` safe on NULL/populated stats; zero leaks, zero errors | PASS |
| **M-02** | Array growth & clone cleanup | Boundary testing & `test_array_store` execution | Capacity growth overflow safety, double-free safety, `log_array_free` safely clears buffer; zero leaks | PASS |
| **M-03** | Sorting memory cleanup | Record integrity audit & `test_sort` clone tests | `log_array_clone` paired with `log_array_free`; stack-allocated swap buffers; zero heap leakage | PASS |
| **M-04** | Search results allocation free | Aliasing and caller buffer audit in `test_search` | Callers manage target buffers safely; `search_populate_results` validates indices atomically and prevents aliasing corruption; zero leaks | PASS |

### 3.6 CLI Unit & Stream Test Suite (`test_cli.c`)

| ID | Description | Input / Condition | Expected Output | Status |
|---|---|---|---|---|
| **C-01** | CLI invalid menu input | Enter non-numeric string (`"abc"`, `"-1"`, `"99"`) | Re-prompts menu gracefully; zero crashes | PASS |
| **C-02** | CLI EOF handling | Feed EOF (`""`) to menu prompt | Exits cleanly with return code 0 | PASS |
| **C-03** | CLI state lifecycle | Initialize and free empty CLI state | State correctly tracks file status; clean memory free | PASS |
| **C-04** | CLI file loading & dataset preservation | Attempt load of invalid file after valid load | Rejects bad file, preserves previous valid dataset and stats | PASS |
| **C-05** | CLI sorting interactive flow | Execute sort option via simulated input stream | Sorts array, updates `SortMetrics`, displays summary | PASS |
| **C-06** | CLI searching interactive flow | Execute search option via simulated input stream | Finds matching records, updates `SearchMetrics`, displays matches | PASS |
| **C-07** | Scripted happy-path session | Full pipeline: load, preview, sort, search, metrics, stats, exit | Returns code 0; all reports display `[SYNTHETIC]` badge | PASS |

### 3.7 Executable-Level Acceptance Suite (`test_cli_executable.c`)

| ID | Description | Execution Method | Expected Output | Status |
|---|---|---|---|---|
| **E-01** | Process startup & clean exit | Launch `bin/logwatch.exe` via OS process, send `"8\n"` | Process exit code 0 | PASS |
| **E-02** | Process startup with file argument | Launch `bin/logwatch.exe tests\fixtures\synthetic_valid.log`, send `"8\n"` | Loads file on startup, process exit code 0 | PASS |
| **E-03** | Complete interactive workflow | Pipe full menu flow (load, preview, sort, search, metrics, stats, reload, exit) | Full stdout generation, process exit code 0 | PASS |
| **E-04** | Error recovery on invalid choices & files | Feed invalid inputs, blank lines, missing file paths, then valid file | Recovers gracefully without hanging or crash; exit code 0 | PASS |
| **E-05** | Immediate EOF on stdin | Pipe empty stdin stream (`""`) to process | Exits cleanly without hang; exit code 0 | PASS |
| **E-06** | EOF during sub-prompt | Pipe `"1\n"` followed by immediate EOF | Exits cleanly without hang; exit code 0 | PASS |

### 3.8 WebAssembly Bridge Integration Suite (`test_wasm_bridge.js`)

| ID | Description | Input / Execution | Expected Output | Status |
|---|---|---|---|---|
| **W-01** | Lifecycle and cleanup | `bridge_init`, `bridge_clear`, `bridge_free` | State safely initialized and released, repeated cleanup safe | PASS |
| **W-02** | Virtual filesystem fixture loading | `bridge_load_from_vfs('/synthetic_valid.log')` | Loads 5 records through Emscripten MEMFS | PASS |
| **W-03** | Ingestion invariant accounting | Valid, malformed (6 malf), and blanks (3 blanks) | `total_lines == valid + malformed + blank` verified across all fixtures | PASS |
| **W-04** | Paginated record retrieval | `bridge_get_records_json(offset, limit)` | Correct bounds, limit, offset, and RFC 8259 JSON serialization | PASS |
| **W-05** | Sorting algorithms correctness | Bubble, Selection, Insertion on `synthetic_valid.log` | Non-decreasing order: 18, 42, 95, 310, 1200; descending order verified | PASS |
| **W-06** | Record integrity & tie-breaking | `synthetic_duplicates.log` sorted ascending and descending | Identical times tie-broken by ascending `source_line`; all struct fields bound | PASS |
| **W-07** | Linear search queries | Exact, threshold, status, prefix, substring, case-insensitive | Exact record matches returned with `matches_found` and record payload | PASS |
| **W-08** | Deterministic metrics verification | Pre-sorted data sort and selection sort | Bubble best-case 4 comparisons; Selection exactly 10 comparisons | PASS |
| **W-09** | Non-destructive comparison benchmark | `bridge_compare_all_sorts` on cloned arrays | 3 algorithms benchmarked; active dataset preserved completely unmodified | PASS |
| **W-10** | Error handling & dataset preservation | Failed load of `/does_not_exist.log`, invalid params | Fails cleanly; pre-existing loaded dataset preserved without corruption | PASS |
| **W-11** | JSON escaping & memory ownership | Special characters (`"`, `\`, control chars) | Clean JSON parse; explicit pointer release via `bridge_free_string` | PASS |
| **W-12** | Native baseline consistency | `samples/synthetic_access.log` (50 records) | Exact match with native C11 expectations (Selection Sort = 1225 comparisons) | PASS |

---

## 4. Optional Extension Test Suites (Gated Post-Core)

*The following test suites are strictly optional and will not be implemented until their respective extensions are approved.*

| Suite | Module | Purpose | Status |
|---|---|---|---|
| `test_linked_list.c` | Singly Linked List | Node insertion, deletion, and traversal | DEFERRED |
| `test_stack.c` | Stack ADT | Synthetic event push/pop and pair check | DEFERRED |
| `test_queue.c` | Queue ADT | Arrival-order FIFO buffering | DEFERRED |
| `test_recursion.c` | Recursion Utils | Factorial, Fibonacci, Tower of Hanoi | DEFERRED |
| `test_bst.c` | Binary Search Tree | Ordered insertion and range query with buckets | DEFERRED |
| `test_avl.c` | AVL Tree | Rotation balance invariant checks | DEFERRED |
| `test_heap.c` | Max-Heap | Top-N ranking and heap sort | DEFERRED |
| `test_hash_table.c` | Hash Table | Route/status lookups and chaining collision checks | DEFERRED |
| `test_graph.c` | Route Graph | Adjacency list and BFS/DFS traversal | DEFERRED |
