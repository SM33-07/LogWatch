# LogWatch — Design Decision Log

**Project:** LogWatch (CSC2101 DSA Lab Project)  
**Last updated:** 2026-10-12  
**Status:** Planning phase — Revised for Scoped Core Release

---

## Decision Index

| ID | Title | Status | Sprint/Milestone |
|---|---|---|---|
| **D-01** | Course Code: CSC2101 | ACCEPTED | All |
| **D-02** | Binary Search Deferred to Lecture 43 | ACCEPTED | Deferred |
| **D-03** | Log Format: Synthetic Pipe-Delimited | ACCEPTED | Core M1 |
| **D-04** | Timestamp Format and Lexicographical Ordering | ACCEPTED | Core M1 |
| **D-05** | Stack-Based Session Pairing: Optional Extension | ACCEPTED | Optional Ext |
| **D-06** | Sorting Strategy: Complete Record Preservation | ACCEPTED | Core M4 |
| **D-07** | Duplicate-Key Handling in Tree Structures | ACCEPTED | Optional Ext |
| **D-08** | Heap Ranking Modes | ACCEPTED | Optional Ext |
| **D-09** | Hash Table Collision Resolution | ACCEPTED | Optional Ext |
| **D-10** | Provenance Default: Strictly SYNTHETIC | ACCEPTED | Core M1 |
| **D-11** | File Path Input: Argument + Interactive Fallback | ACCEPTED | Core M6 |
| **D-12** | Test Framework: Custom Macros (No External Lib) | ACCEPTED | Core M1 |
| **D-13** | No Recurrence-Solving Methods Until Taught | ACCEPTED | Deferred |
| **D-14** | Graph Route Navigation: Optional Extension | ACCEPTED | Optional Ext |
| **D-15** | Initial Release Scoping to Core DSA Baseline | ACCEPTED | Core M1–M7 |
| **D-16** | Record Integrity Invariant During All Sorts | ACCEPTED | Core M4 |
| **D-17** | Operation-Count Metrics Architecture | ACCEPTED | Core M4, M5 |
| **D-18** | Non-Destructive Sort Comparison via Cloned Arrays | ACCEPTED | Core M4, M6 |
| **D-19** | Authoritative Syllabus Classification Protocol | ACCEPTED | All |

---

## Detailed Decisions

### D-01 — Course Code: CSC2101
- **Status**: ACCEPTED
- **Decision**: Use course code **CSC2101** (July–November 2026 handout) throughout all documentation, the final report, and the viva.
- **Rationale**: Confirmed as the verified course code in the approved syllabus. Earlier draft label CSC2130 is obsolete.

### D-02 — Binary Search Deferred to Lecture 43
- **Status**: ACCEPTED
- **Decision**: Sprint 1 / Core Release uses **Linear Search** exclusively. Binary Search is deferred until divide-and-conquer binary search is formally covered at Lecture 43 or approved by instructor.
- **Rationale**: The approved course handout schedules divide-and-conquer binary search at Lecture 43.

### D-03 — Log Format: Synthetic Pipe-Delimited
- **Status**: ACCEPTED
- **Decision**: Use the synthetic pipe-delimited format documented in PRD/SRS §5.2: `timestamp|route|status_code|response_time_ms|client_id`.
- **Rationale**: No CodeVault production log sample exists in the workspace. Using this format unblocks all core parser and analytics development.
- **Constraint**: All data loaded under this format is explicitly tagged `[SYNTHETIC]`. Never claim synthetic data is real CodeVault traffic.

### D-04 — Timestamp Format and Lexicographical Ordering
- **Status**: ACCEPTED
- **Decision**: Synthetic timestamps use ISO-8601 UTC format: `YYYY-MM-DDTHH:MM:SSZ`. These fixed-width, zero-padded strings sort chronologically via `strcmp()`.

### D-05 — Stack-Based Session Pairing: Optional Extension
- **Status**: ACCEPTED
- **Decision**: Move stack-based event pairing to an Optional Extension.
- **Rationale**: Completed-request access logs record response events only. They lack paired `START`/`END` events. Moving this to an optional synthetic demonstration keeps the Core Release focused on genuine access-log analytics.

### D-06 — Sorting Strategy: Complete Record Preservation
- **Status**: ACCEPTED
- **Decision**: Baseline sorting algorithms (Bubble Sort, Selection Sort, Insertion Sort) operate on arrays of complete `LogEntry` structures (~360 bytes each).
- **Rationale**: Swapping complete structs ensures that route, status code, response time, and client ID remain permanently bound together. At lab scale ($\le 10,000$ entries), struct copying in cache memory is extremely fast and pedagogically clear.

### D-07 — Duplicate-Key Handling in Tree Structures
- **Status**: ACCEPTED (Optional Extension)
- **Decision**: If BST or AVL extensions are implemented, nodes must store bucket arrays of indices into the `LogArray` to handle duplicate keys without discarding records.

### D-08 — Heap Ranking Modes
- **Status**: ACCEPTED (Optional Extension)
- **Decision**: Heap ranking for Top-N slow requests and Top-N error-prone routes is classified as an Optional Extension.

### D-09 — Hash Table Collision Resolution
- **Status**: ACCEPTED (Optional Extension)
- **Decision**: Hash table lookups using separate chaining are classified as an Optional Extension.

### D-10 — Provenance Default: Strictly SYNTHETIC
- **Status**: ACCEPTED
- **Decision**: All loaded data defaults to `PROV_SYNTHETIC`. All CLI banners, summary tables, and exported reports must display `[SYNTHETIC]`.
- **Rationale**: Prevents any unintentional misrepresentation of synthetic test data as live production traffic.

### D-11 — File Path Input: Argument + Interactive Fallback
- **Status**: ACCEPTED
- **Decision**: Support file path via command-line argument (`./logwatch <path>`). If omitted, provide an interactive prompt. Allow reloading files via the CLI menu.

### D-12 — Test Framework: Custom Macros (No External Lib)
- **Status**: ACCEPTED
- **Decision**: Use a custom `tests/test_utils.h` header with minimal assert macros (`ASSERT`, `ASSERT_EQ`, `ASSERT_STR_EQ`, `TEST`). Each test suite compiles to a standalone test executable.
- **Rationale**: Strict adherence to the constraint prohibiting external third-party libraries.

### D-13 — No Recurrence-Solving Methods Until Taught
- **Status**: ACCEPTED
- **Decision**: Recurrence methods (Lectures 38–41: Master Theorem, recursion tree) are deferred. The project uses standard Big-O/Ω/Θ asymptotic notation for complexity documentation.

### D-14 — Graph Route Navigation: Optional Extension
- **Status**: ACCEPTED
- **Decision**: Graph representations and BFS/DFS traversal are strictly Optional Extensions.

### D-15 — Initial Release Scoping to Core DSA Baseline
- **Status**: ACCEPTED
- **Decision**: Scope the initial release strictly to: file parsing, dynamic array (`LogArray`), Bubble Sort, Selection Sort, Insertion Sort, Linear Search, complexity/operation-count analysis, menu-driven CLI, and automated tests.
- **Rationale**: Small-team deliverable constraint. Prioritizes correctness, robust error handling, explainability, and thorough testing over an unmanageable breadth of unfinished features.

### D-16 — Record Integrity Invariant During All Sorts
- **Status**: ACCEPTED
- **Decision**: Every swap or element move during sorting must swap the entire `LogEntry` structure:
  ```c
  LogEntry temp = arr->entries[i];
  arr->entries[i] = arr->entries[j];
  arr->entries[j] = temp;
  ```
  Never swap only key fields.
- **Rationale**: Preserves relational integrity of the log row. Verified by dedicated automated test assertions.

### D-17 — Operation-Count Metrics Architecture
- **Status**: ACCEPTED
- **Decision**: Introduce explicit `SortMetrics` and `SearchMetrics` structs to track exact runtime comparisons and record moves.
- **Rationale**: Provides empirical operation counts for pedagogical comparison with theoretical Big-O notation, without making invalid wall-clock benchmarking claims.

### D-18 — Non-Destructive Sort Comparison via Cloned Arrays
- **Status**: ACCEPTED
- **Decision**: The CLI comparison feature will clone the loaded `LogArray` before running each sort algorithm, so all three algorithms are evaluated against identical uncorrupted input.
- **Rationale**: Running sequential in-place sorts on the same array would result in the second and third algorithms executing on an already-sorted array, invalidating comparison results.

### D-19 — Authoritative Syllabus Classification Protocol
- **Status**: ACCEPTED
- **Decision**: Classify all proposed data structures and algorithms into three explicit tiers:
  1. **Covered (Core Release)**: Dynamic arrays, Bubble Sort, Selection Sort, Insertion Sort, Linear Search, Complexity Analysis.
  2. **Not Yet Covered (Deferred)**: Divide-and-conquer binary search, Merge Sort, Quick Sort, recurrence-solving methods.
  3. **Optional (Extension)**: Linked lists, stacks, queues, recursion demos, BST, AVL, heaps, hashing, graphs.
- **Rationale**: Strictly respects the course syllabus and teacher lecture progression.
