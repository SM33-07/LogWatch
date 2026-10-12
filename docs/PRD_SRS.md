# LogWatch — Product Requirements Document & Software Requirements Specification

**Project title:** LogWatch — A DSA-Driven Log Analysis and Monitoring Tool for Web Applications  
**Project type:** Application-based Data Structures and Algorithms laboratory project  
**Implementation language:** C (C11 standard)  
**Runtime:** Offline command-line application (menu-driven)  
**Status:** Revised Specification for Scoped Core Release and Extension Roadmap  
**Course code:** CSC2101 (Data Structures and Algorithms, July–November 2026)

---

## 1. Course-Coverage Gate and Algorithm Classification

The actual course handout (CSC2101) and topics completed in class are the authoritative source for syllabus eligibility. An algorithm or data structure must not be assumed covered solely because of its scheduled lecture number.

### 1.1 Course-Coverage Classification Table

| Topic / Algorithm | Status in Class | Project Classification | Rationale & Inclusion Decision |
|---|---|---|---|
| **Arrays & Dynamic Arrays** | Covered | **Core (Initial Release)** | Foundational storage mechanism; models contiguous memory, dynamic growth, and cache locality. |
| **Bubble Sort** | Covered | **Core (Initial Release)** | Elementary $O(n^2)$ comparison sort; baseline for adaptive comparisons and swaps moving full records. |
| **Selection Sort** | Covered | **Core (Initial Release)** | Elementary $O(n^2)$ sort; minimizes record write/swap operations ($O(n)$ moves). |
| **Insertion Sort** | Covered | **Core (Initial Release)** | Elementary $O(n^2)$ adaptive sort; excellent on nearly-sorted data ($O(n)$ best case). |
| **Linear Search** | Covered | **Core (Initial Release)** | Baseline search ($O(n)$) requiring no pre-sorting; handles unsorted collections and duplicates. |
| **Complexity & Operation Counting** | Covered | **Core (Initial Release)** | Asymptotic notation ($O, \Omega, \Theta$) plus pedagogical comparison/swap counters. |
| **Singly Linked Lists** | Covered | **Optional Extension** | Dynamic node-based storage; evaluated as extension after core is stabilized. |
| **Stacks & Queues** | Covered | **Optional Extension** | LIFO/FIFO event buffers; deferred to extension due to access-log semantics (completed requests lack lifecycle pairs). |
| **Recursion Demos** | Covered | **Optional Extension** | Classic recursion demos (Factorial, Fibonacci, Hanoi); auxiliary pedagogical module. |
| **Binary Search Trees (BST)** | Covered | **Optional Extension** | Ordered tree indexing with bucketed duplicate handling; optional post-core extension. |
| **AVL Trees** | Covered | **Optional Extension** | Height-balanced search trees with rotations; optional post-core extension. |
| **Heaps & Heap Sort** | Covered | **Optional Extension** | Priority queue top-N ranking; optional post-core extension. |
| **Hashing & Hash Tables** | Covered | **Optional Extension** | $O(1)$ average route/status lookups with chaining; optional post-core extension. |
| **Graphs & BFS / DFS** | Covered | **Optional Extension** | Route navigation graph; strictly optional after all other milestones. |
| **Binary Search (Divide-and-Conquer)** | Scheduled (L43) | **Not Yet Covered** | Formal divide-and-conquer treatment scheduled at Lecture 43; not in core release. |
| **Merge Sort** | Scheduled (L44) | **Not Yet Covered** | Divide-and-conquer $O(n \log n)$ sort scheduled at Lecture 44; strictly deferred. |
| **Quick Sort** | Scheduled (L45) | **Not Yet Covered** | Divide-and-conquer partitioning scheduled at Lecture 45; strictly deferred. |
| **Recurrence Methods (Master Thm)** | Scheduled (L38–41) | **Not Yet Covered** | Substitution, recursion tree, Master Theorem analysis; strictly deferred. |

---

## 2. Product Summary

LogWatch reads server access/request logs, parses them into structured records, and provides diagnostic insights: error frequencies, response-time distribution, route-level traffic, and specific record queries through an interactive, menu-driven CLI.

LogWatch is an **offline CLI tool**. It operates strictly on local log files and never connects to live production services or CodeVault servers. All synthetic logs are explicitly tagged, preventing any confusion with production traffic.

---

## 3. Goals and Success Criteria

### 3.1 Primary Goals

1. **Robust Ingestion**: Safely parse pipe-delimited log files into structured memory records, tolerating malformed/blank lines without crashing.
2. **Pedagogical Integrity**: Implement dynamic arrays, elementary sorting algorithms, linear search, and operation counters completely from scratch in standard C11 without external DSA libraries.
3. **Record Association Integrity**: Guarantee that every sorting algorithm moves complete log records together, preserving the association between route, status code, response time, client ID, and source line.
4. **Transparent Analysis**: Provide both theoretical asymptotic complexity and runtime operation metrics (comparisons and swaps/moves).
5. **Usable Menu-Driven CLI**: Offer a clear, interactive terminal interface that displays data provenance and summary reports.
6. **Automated Verification**: Ensure comprehensive automated test coverage for parsing edge cases, record integrity, sort correctness, search accuracy, and memory safety.

### 3.2 Success Criteria

- Clean compilation with GCC (`-Wall -Wextra -pedantic -std=c11`) with zero warnings.
- 100% automated test pass rate for all core test suites.
- Complete memory cleanup verified (zero memory leaks).
- Malformed rows are counted and reported; invalid files do not cause crashes or undefined behavior.
- Every sorting algorithm produces verified non-decreasing order while preserving record field associations.

---

## 4. Scope

### 4.1 In Scope (Core Initial Release)

- Robust file parsing for the canonical pipe-delimited format.
- `LogEntry` canonical data model and `LogArray` dynamic array storage.
- In-place sorting of complete `LogEntry` records using:
  - Bubble Sort (with early-termination flag)
  - Selection Sort (minimum element selection)
  - Insertion Sort (element shifting)
- Multi-key sorting support (by `response_time_ms`, `status_code`, or `timestamp`).
- Linear Search over `LogArray` with multi-match reporting.
- Exact operation-count metrics: comparisons count, swaps/moves count, and elapsed time.
- Asymptotic complexity reference table displayed in CLI.
- Interactive, menu-driven command-line interface.
- CLI execution via file argument (`./logwatch <path>`) with interactive fallback prompt.
- Synthetic fixture generation and automated unit/integration test suite.

### 4.2 Out of Scope (Deferred to Optional Extensions)

- Singly linked list storage (Optional Extension 1).
- Stack and Queue event processing (Optional Extension 2).
- Standalone recursion utilities (Optional Extension 3).
- Binary Search Trees (BST) and AVL trees (Optional Extension 4 & 5).
- Max-Heap and Heap Sort for top-N ranking (Optional Extension 6).
- Hash Tables with collision resolution (Optional Extension 7).
- Route transition graphs and BFS/DFS (Optional Extension 8).
- Divide-and-conquer algorithms (Binary Search, Merge Sort, Quick Sort) — deferred until covered in class.
- Live log streaming, network sockets, GUI/web dashboard, external third-party libraries.

---

## 5. Data Source and Format Policy

### 5.1 Provenance Policy

- **Default Provenance**: All loaded data is treated and displayed as `[SYNTHETIC]` unless an official, verified production sample format is explicitly provided.
- **Labeling Mandate**: Every CLI screen, printed report, summary table, and test output must prominently display `[SYNTHETIC DATA]`.
- **Zero Production Claims**: Under no circumstances should synthetic data be claimed as real CodeVault traffic.

### 5.2 Canonical Pipe-Delimited Format

```text
timestamp|route|status_code|response_time_ms|client_id
```

Example valid records:
```text
2026-10-09T10:00:00Z|/api/snippets|200|42|client-A
2026-10-09T10:00:01Z|/api/snippets|500|310|client-B
2026-10-09T10:00:02Z|/api/auth/login|401|18|client-C
```

### 5.3 Field Specifications and Boundaries

| Field | Type | Constraints & Validation Rules |
|---|---|---|
| `timestamp` | String | ISO-8601 UTC format (`YYYY-MM-DDTHH:MM:SSZ`), 20 chars (`LW_MAX_TIMESTAMP = 32`). Fixed width allows lexicographical comparison. |
| `route` | String | URL path starting with `/`, non-empty, max 255 chars (`LW_MAX_ROUTE = 256`). |
| `status_code` | Integer | HTTP status code, strictly between 100 and 599 inclusive. |
| `response_time_ms`| Integer | Integer duration in milliseconds, strictly $\ge 0$. |
| `client_id` | String | Alphanumeric/hyphen identifier, non-empty, max 63 chars (`LW_MAX_CLIENT_ID = 64`). |
| `source_line` | Integer | 1-based line number in original file (for error reporting and stable tie-breaking). |

---

## 6. Functional Requirements (Core Release)

### FR-01: File Parsing and Validation
- The parser must open the specified file and read lines sequentially using `fgets`.
- Lines exceeding `LW_MAX_LINE` (1024 bytes) must be flagged as malformed and safely consumed.
- Blank and whitespace-only lines must be skipped and tracked as `blank_lines`.
- Lines with fewer or more than 5 pipe-delimited fields, invalid status codes (<100 or >599), negative response times, or empty routes must be rejected as `malformed_lines`.
- The parser must populate a `ParseStats` struct (`total_lines`, `valid_records`, `malformed_lines`, `blank_lines`).

### FR-02: Dynamic Array Storage (`LogArray`)
- Store valid `LogEntry` records in contiguous heap memory.
- Dynamic resizing: initial capacity = 16, doubling capacity whenever size reaches capacity ($16 \to 32 \to 64 \to \dots$).
- Provide functions to initialize, append, copy/clone, reset, and free the array.
- Guaranteed amortized $O(1)$ append time.

### FR-03: Sorting with Complete Record Association
- Support three baseline sorting algorithms:
  1. **Bubble Sort**: with early-exit flag if no swaps occur during a pass.
  2. **Selection Sort**: scans unsorted prefix/suffix to find minimum/maximum and performs at most $n-1$ swaps.
  3. **Insertion Sort**: inserts each element into its sorted position via record shifts.
- **Record Integrity Mandate**: Swaps and shifts MUST operate on the entire `LogEntry` struct. Individual fields must NEVER be swapped in isolation.
- Supported sort keys:
  - `response_time_ms` (ascending / descending)
  - `status_code` (ascending / descending)
  - `timestamp` (chronological)
- Stable tie-breaking: when keys are equal, break ties using original `source_line`.

### FR-04: Searching (Linear Search)
- Perform linear scan over `LogArray` without requiring sorted order.
- Support queries by:
  - Exact `response_time_ms`
  - Range `response_time_ms >= threshold`
  - Exact `status_code`
  - Route substring or prefix match
- Return an array/list of matching record indices, reporting the total number of matches found.

### FR-05: Complexity and Operation-Count Analysis
- Maintain an exact `SortMetrics` struct for every sort invocation:
  - `comparisons`: count of key comparisons executed.
  - `moves`: count of record swaps or element shifts executed.
  - `time_taken_sec`: wall-clock time in seconds (for informational observation only).
- Maintain a `SearchMetrics` struct:
  - `comparisons`: count of elements checked.
  - `matches_found`: count of records matching criteria.
- Display theoretical $O, \Omega, \Theta$ notation alongside observed operation counts.

### FR-06: Interactive Menu-Driven CLI
- Display an ASCII header with `[SYNTHETIC DATA]` provenance clearly visible.
- Interactive numbered menu:
  1. View loaded log summary & statistics
  2. Display log records (paginated / top N)
  3. Sort records (select algorithm & key, view operation counts)
  4. Compare sorting algorithms (runs all 3 on identical data copies, displays comparison table)
  5. Search records (Linear Search by response time, status code, route)
  6. Display DSA theoretical complexity reference table
  7. Load a different log file
  8. Exit
- Clean input validation: gracefully handle non-numeric or out-of-range menu inputs without infinite loops.

### FR-07: Automated Testing
- Self-contained test harness in `tests/test_utils.h` (no external libraries).
- Modular test suites covering parser, dynamic array, sorting, searching, metrics, and memory safety.

---

## 7. Explicit Acceptance Criteria Matrix

| Domain | Acceptance Criteria | Verification Method |
|---|---|---|
| **Parsing** | 1. Exactly 5 valid rows parsed from `synthetic_valid.log`.<br/>2. Malformed lines skipped without crash; `malformed_lines` count accurately incremented.<br/>3. Blank and whitespace lines skipped and counted as `blank_lines`.<br/>4. Status codes <100 or >599 rejected.<br/>5. Negative response times rejected. | `test_parser` automated test binary. |
| **Sorting** | 1. Bubble, Selection, and Insertion Sort all produce correctly ordered records.<br/>2. Empty array (0 records) and single-element array (1 record) handled without errors.<br/>3. **Record Integrity**: Every field (`route`, `status_code`, `client_id`, `source_line`) stays linked to its sorted `response_time_ms`.<br/>4. Identical keys (duplicates) are preserved without dropping or overwriting records.<br/>5. Already-sorted and reverse-sorted arrays sorted correctly. | `test_sort` automated test binary with record integrity validation checks. |
| **Searching** | 1. Linear Search finds all matching occurrences (including duplicates).<br/>2. Returns 0 matches when key is absent.<br/>3. Accurately reports comparison count ($n$ comparisons for full scan). | `test_search` automated test binary. |
| **Complexity** | 1. `SortMetrics.comparisons` and `SortMetrics.moves` are positive and match theoretical bounds.<br/>2. Bubble Sort best-case on sorted array exits in $n-1$ comparisons with early exit flag.<br/>3. Selection Sort always performs exactly $n(n-1)/2$ comparisons.<br/>4. Side-by-side comparison runs on clean copies of identical data without contaminating initial state. | `test_sort` metrics assertions. |
| **Error Handling** | 1. Non-existent file path returns clean error message without crash.<br/>2. Invalid menu selection reprompts user gracefully.<br/>3. Overly long lines (>1024 chars) truncated or skipped safely. | CLI error simulation tests. |
| **Memory Safety** | 1. Every `malloc`/`realloc` has a corresponding `free`.<br/>2. Zero memory leaks on program exit.<br/>3. Zero buffer overflows or out-of-bounds pointer reads. | Valgrind / AddressSanitizer (`-fsanitize=address`) verification. |

---

## 8. Non-Functional Requirements

- **C Standard**: Strict ISO C11 (`-std=c11`).
- **Compiler**: GCC on Linux/Windows MinGW.
- **Zero Third-Party Dependencies**: Standard C library (`stdio.h`, `stdlib.h`, `string.h`, `time.h`, `stdbool.h`) only.
- **Portability**: Code must compile and run on both Windows (MinGW) and Linux/macOS.
- **Team Maintainability**: Modular design where each module has a clear header file, implementation file, and dedicated test suite.
