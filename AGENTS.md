# LogWatch — Workspace Operational Rules & Constraints

**Project:** LogWatch (CSC2101 DSA Lab Project)  
**Standard:** ISO C11 (`-std=c11 -Wall -Wextra -pedantic`)  
**Target Environment:** GCC (MinGW / Linux)  
**Output Directory:** `bin/`

---

## 1. Core Principles & Hard Constraints

1. **Strict Language Standard**: All code must conform to C11 with `-std=c11 -Wall -Wextra -pedantic`. Code must compile cleanly with zero warnings.
2. **Zero External Dependencies**: Standard C library only (`stdio.h`, `stdlib.h`, `string.h`, `stdbool.h`, `time.h`). No external DSA libraries, third-party test runners, or GUI packages.
3. **Build Artifact Isolation**: All binaries and object files belong in `bin/`. Never pollute the workspace root or source directories with build artifacts.
4. **Data Provenance Mandate**: All test logs, demo fixtures, CLI screens, and output reports must display `[SYNTHETIC]`. Under no circumstances should synthetic data be claimed as real CodeVault production traffic.
5. **Record Integrity Invariant**: All sorting and swapping operations move the complete `LogEntry` structure. Swapping isolated key fields is strictly forbidden.
6. **Non-Destructive Algorithm Comparisons**: Sorting comparisons across Bubble, Selection, and Insertion Sort must operate on distinct cloned copies (`log_array_clone`) so all algorithms process identical raw inputs.
7. **Authoritative Syllabus Gating**:
   - **Core Initial Release**: Dynamic Array (`LogArray`), Bubble Sort (with early exit), Selection Sort, Insertion Sort, Linear Search (multi-match), operation counters (`SortMetrics`, `SearchMetrics`), and interactive menu CLI.
   - **Deferred**: Divide-and-conquer algorithms (Binary Search, Merge Sort, Quick Sort) scheduled at Lecture 43+ are strictly deferred until taught.
   - **Gated Extensions**: Singly Linked Lists, Stacks, Queues, Recursion Demos, BST, AVL Trees, Heaps/Heap Sort, Hash Tables, and Graphs are optional extensions gated post-core.
8. **Memory Safety & Defensive Coding**:
   - Every `malloc`/`realloc` must have a corresponding deallocator (`log_array_free`, `parse_stats_free`, etc.).
   - All pointer parameters must be checked against `NULL`.
   - Never use unbounded input functions (no `gets()`, no `%s` without width in `scanf`). Use `fgets` with explicit buffer sizes.
   - Zero memory leaks.
9. **Testing Rigor**:
   - Automated tests are written using `tests/test_utils.h` macros (`ASSERT`, `ASSERT_EQ`, `ASSERT_STR_EQ`, `TEST`).
   - Tests compile to standalone executables in `bin/` and return exit code 0 only when all assertions pass.
   - Never mark a test as passing without actual execution.
10. **Phased Milestone Discipline**:
    - Implement only the components assigned to the active milestone.
    - Never jump ahead to implement core algorithms, CLI, or parsers before their designated milestone.
    - Preserve all existing user work; do not overwrite or delete unrelated files.
