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
| **S-01** | Bubble Sort correctness | Unsorted `synthetic_valid.log` by response time | Records in non-decreasing order: 18, 42, 95, 310, 1200 | DRAFT |
| **S-02** | Selection Sort correctness | Unsorted `synthetic_valid.log` by response time | Records in non-decreasing order: 18, 42, 95, 310, 1200 | DRAFT |
| **S-03** | Insertion Sort correctness | Unsorted `synthetic_valid.log` by response time | Records in non-decreasing order: 18, 42, 95, 310, 1200 | DRAFT |
| **S-04** | **Record Integrity Check (Bubble)** | Sort `synthetic_valid.log` by response time | For record 18: `route == "/api/auth/login"`, `status == 401`, `client == "client-C"`. Association strictly preserved! | DRAFT |
| **S-05** | **Record Integrity Check (Select)** | Sort `synthetic_valid.log` by response time | Complete struct fields match source record for every item | DRAFT |
| **S-06** | **Record Integrity Check (Insert)** | Sort `synthetic_valid.log` by response time | Complete struct fields match source record for every item | DRAFT |
| **S-07** | Empty array boundary | Array with 0 records | Sort completes safely without null dereference | DRAFT |
| **S-08** | Single element boundary | Array with 1 record | Sort completes safely, record unchanged | DRAFT |
| **S-09** | Already-sorted array | 5 sorted records | Array remains sorted; Bubble Sort exits in 1 pass ($n-1$ comparisons) | DRAFT |
| **S-10** | Reverse-sorted array | 5 reverse-sorted records | Array correctly sorted in ascending order | DRAFT |
| **S-11** | Duplicate keys handling | `synthetic_duplicates.log` (100, 100, 200, 100, 200) | Sorted: 100, 100, 100, 200, 200. Zero records lost. Stable tie-break by `source_line`. | DRAFT |
| **S-12** | Sort by HTTP Status Code | `synthetic_valid.log` by status | Sorted: 200, 200, 401, 500, 503 | DRAFT |
| **S-13** | Sort by Timestamp | `synthetic_valid.log` by timestamp | Chronological order maintained | DRAFT |
| **S-14** | Descending sort order | `synthetic_valid.log`, `ascending = false` | Sorted in non-increasing order: 1200, 310, 95, 42, 18 | DRAFT |
| **S-15** | Selection Sort moves count | Array of size $n$ | `SortMetrics.moves <= n - 1` ($O(n)$ swaps) | DRAFT |
| **S-16** | Insertion Sort best-case moves | Already-sorted array of size $n$ | `SortMetrics.moves == 0`, `comparisons == n - 1` | DRAFT |
| **S-17** | Non-destructive comparison | Run Bubble, Selection, and Insertion Sort on clones of same array | All 3 sorts produce identical ordered output from fresh copies | DRAFT |

### 3.4 Linear Search Test Suite (`test_search.c`)

| ID | Description | Input / Condition | Expected Output | Status |
|---|---|---|---|---|
| **L-01** | Search exact response time (single match) | Target: 95ms in `synthetic_valid.log` | `matches == 1`, index points to `/api/snippets/42` | DRAFT |
| **L-02** | Search exact response time (multi match) | Target: 100ms in `synthetic_duplicates.log` | `matches == 3`, indices point to lines 1, 2, 4 | DRAFT |
| **L-03** | Search response time not found | Target: 9999ms | `matches == 0`, `out_indices == NULL` | DRAFT |
| **L-04** | Search by status code | Target: 500 in `synthetic_valid.log` | `matches == 1`, `/api/snippets` 500 found | DRAFT |
| **L-05** | Search by route prefix/substring | Target: `"/snippets"` | `matches == 3`, all matching route records returned | DRAFT |
| **L-06** | Search empty array | Array with 0 records | `matches == 0`, safe return | DRAFT |
| **L-07** | Search metrics comparison count | Unsorted array of $n$ elements | `SearchMetrics.comparisons == n` (full linear scan) | DRAFT |

### 3.5 Memory Safety and System Resilience

| ID | Description | Tool / Condition | Expected Output | Status |
|---|---|---|---|---|
| **M-01** | Parser memory cleanup | Valgrind / ASan on parsing fixtures | Zero memory leaks, zero errors | DRAFT |
| **M-02** | Array growth & clone cleanup | Valgrind / ASan on `test_array_store` | Zero memory leaks, zero errors | DRAFT |
| **M-03** | Sorting memory cleanup | Valgrind / ASan on `test_sort` | Zero memory leaks, zero errors | DRAFT |
| **M-04** | Search results allocation free | Valgrind / ASan on `test_search` | Caller frees `out_indices`; zero leaks | DRAFT |
| **C-01** | CLI invalid menu input | Enter characters (`"abc"`, `"-1"`, `"99"`) | Re-prompts menu gracefully; zero crashes | DRAFT |
| **C-02** | CLI EOF handling | Feed EOF to menu prompt | Exits cleanly with return code 0 | DRAFT |

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
