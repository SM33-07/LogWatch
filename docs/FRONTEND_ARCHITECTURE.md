# LogWatch — Frontend Architecture Assessment (Milestone 8A)

**Document Version:** 1.0.0  
**Status:** Approved / Assessment Complete  
**Date:** 2026-10-13  
**Target Stack:** Next.js (App Router), TypeScript, Tailwind CSS, WebAssembly (Emscripten)  
**Authoritative Core:** LogWatch C11 Data Structures & Algorithms Library  

---

## 1. Executive Summary & Core Mission

LogWatch has successfully delivered all seven core milestones: robust pipe-delimited log parsing, the dynamically resizable `LogArray` storage engine, baseline sorting algorithms (Bubble Sort with early exit, Selection Sort, Insertion Sort) maintaining complete record integrity, a multi-match linear search engine, operation metrics accounting (`SortMetrics`, `SearchMetrics`), and an interactive ANSI terminal CLI.

The objective of **Milestone 8A** is to evaluate the technical feasibility, architectural design, data bridging, and user interface structure for a modern browser frontend. The frontend will serve as an interactive developer tool and DSA pedagogical laboratory, visualizing log data analytics and algorithm performance metrics in real time.

### The Non-Negotiable Core Constraint: Authoritative C11 Mandate
The LogWatch web frontend **must not reimplement** the sorting, searching, parsing, or dynamic array algorithms in JavaScript or TypeScript. All DSA logic, empirical operation counters (`comparisons`, `record_swaps`, `record_shifts`, `matches_found`), tie-breaking semantics, and dataset ingestion invariants **must execute directly within the compiled C11 engine**. The browser layer functions solely as a presentation, orchestration, and interactive control shell.

---

## 2. Evaluation of Bridging Architectures

Three primary architectural topologies were analyzed to bridge the web frontend with the authoritative C11 engine:

```
[Option 1: In-Browser WebAssembly (WASM)]
+-------------------------------------------------------------+
| Browser Sandbox (Next.js Static / CSR / SSR)                |
|                                                             |
|   +-------------------+          +-----------------------+  |
|   | Next.js UI (TS)   | <------> | logwatch.wasm (C11)   |  |
|   | Tailwind + Canvas |  cwrap   | Emscripten Engine     |  |
|   +-------------------+          +-----------------------+  |
|                                         ^                   |
|                                         | MEMFS / SharedBuf |
|                                         v                   |
|                                  [Synthetic Logs]           |
+-------------------------------------------------------------+

[Option 2: Native Process Spawning (Node.js child_process)]
+---------------------------+          +-----------------------+
| Next.js Server (Node.js)  |  stdin   | Native C Executable   |
| API Route / Server Action | -------> | bin/logwatch.exe      |
|                           | <------- | (GCC / MinGW Native)  |
|                           |  stdout  +-----------------------+
+---------------------------+
              ^
              | HTTP / WebSocket
              v
+---------------------------+
| Client Browser            |
+---------------------------+

[Option 3: Embedded C Micro-Daemon]
+---------------------------+          +-----------------------+
| Client Browser            |   HTTP   | C Micro-Daemon        |
| Next.js Frontend          | -------> | (Mongoose / Libmicro) |
|                           | <------- | Native C Process      |
+---------------------------+  REST    +-----------------------+
```

### 2.1 Comparative Architecture Matrix

| Dimension | Option 1: WebAssembly (Emscripten) | Option 2: Native Child Process Spawning | Option 3: Local C Micro-Daemon |
|---|---|---|---|
| **Authoritative Fidelity** | **100% Exact** (identical C11 source compiled to `wasm32`) | **100% Exact** (executes `bin/logwatch.exe`) | **100% Exact** (links C modules directly) |
| **Execution Context** | In-browser client sandbox (zero server required) | Next.js server runtime (Node.js environment) | Dedicated background OS process on localhost |
| **Deployment Portability** | **Universal** (Runs on Vercel, Netlify, GitHub Pages, offline browser) | **Restricted** (Requires Windows/Linux host with compiled C binary) | **Restricted** (Requires background service management, port negotiation) |
| **User Toolchain Prerequisite** | **Zero for end users** (Pre-compiled WebAssembly runtime committed under `web/public/wasm/`). Fresh clone runs with Git, Node.js $\ge$ 20.9, and npm via `npm ci` + `npm run dev` without Emscripten or a native C compiler. | GCC/MinGW or pre-compiled OS binary in server path | GCC/MinGW and network socket permissions |
| **Execution Latency** | Sub-millisecond synchronous or WebWorker calls | High process spawn overhead (10–60ms per invocation) | Medium (HTTP loopback overhead: 1–3ms) |
| **State Persistence** | Persistent in WASM linear memory (`LogArray` stays loaded) | Stateless per spawn unless long-running IPC pipe is maintained | Stateful in C daemon heap |
| **UI Interactivity & Scrubbing** | Real-time interactive controls, live search-as-you-type | Laggy for search keystrokes and slider adjustments | Moderate latency; polling or WebSocket required |
| **Complexity Score** | Low-Medium (Bridge layer required) | Medium-High (IPC protocol, pipe synchronization, OS timeouts) | High (Networking, threading, socket teardown, port conflicts) |

### 2.2 Recommendation & Architectural Decision

**Verdict: Adopt Option 1 (In-Browser WebAssembly via Emscripten) with an optional Option 2 Local Dev Fallback.**

*Rationale:*
1. **Serverless & Edge Deployment:** Compiling the C11 modules to WebAssembly allows LogWatch to be hosted as a completely static web application (e.g. on Vercel, GitHub Pages, Cloudflare Pages) with zero server infrastructure.
2. **Instant UI Reactivity:** With the `LogArray` residing in WASM linear memory, linear searches over 5,000+ records and multi-algorithm sort benchmarks execute in `< 2 ms`, enabling instant live updates as users type search queries or toggle sort keys.
3. **Pedagogical Transparency:** Users and evaluators can upload custom synthetic logs or select pre-bundled fixtures directly in the browser without sending data over a network, perfectly aligning with data privacy and sandbox integrity.

---

## 3. Emscripten & WebAssembly Feasibility Analysis

### 3.1 C11 Standard Library Compatibility
The LogWatch codebase was architected under strict ISO C11 guidelines (`-std=c11 -Wall -Wextra -pedantic`) with zero external dependencies, utilizing only standard headers:
- `<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<stdbool.h>`, `<ctype.h>`, `<limits.h>`, `<time.h>`

Emscripten (`emcc`) provides complete Musl libc compatibility for all these facilities:
- Dynamic heap management (`malloc`, `realloc`, `free`) maps directly into the WebAssembly linear memory allocator (`dlmalloc` / `emmalloc`).
- String operations (`strcmp`, `strncmp`, `strstr`, `memcpy`, `strlen`) compile to high-performance SIMD-accelerated WASM bytecode.
- High-resolution timing (`timespec_get` / `clock`) in `sort.c` and `search.c` maps directly to browser high-resolution monotonic timers (`performance.now()`).

### 3.2 File I/O Bridging: Virtual MEMFS vs. In-Memory Buffer Parsing
In the current C implementation, `parse_file(filepath, ...)` operates on a filesystem path using standard `FILE *` and `fgets`. In WebAssembly, this is accommodated via two frictionless paths:

1. **Virtual Filesystem (`MEMFS` / `FS.writeFile`):**
   Emscripten's built-in memory filesystem allows JavaScript to populate virtual files before invoking existing C functions:
   ```javascript
   // JavaScript side in Next.js
   Module.FS.writeFile('/workspace/sample.log', logTextContent);
   // C function called without ANY code modifications
   Module.ccall('bridge_load_file', 'number', ['string'], ['/workspace/sample.log']);
   ```
2. **Buffer-Based Ingestion (`parse_buffer`):**
   To optimize throughput and avoid virtual filesystem overhead, an optional zero-copy C bridge helper `parse_buffer(const char *buf, size_t len, ...)` can parse directly from a heap pointer allocated via `Module._malloc()`.

Both methods work seamlessly without altering the core `parser.c` logic.

### 3.3 Data Model & ABI Alignment
The core `LogEntry` structure is defined as:
```c
typedef struct {
    char timestamp[32];      /* offset  0, size  32 */
    char route[256];         /* offset 32, size 256 */
    int  status_code;        /* offset 288, size  4 */
    int  response_time_ms;   /* offset 292, size  4 */
    char client_id[64];      /* offset 296, size 64 */
    int  source_line;        /* offset 360, size  4 */
} LogEntry;                  /* total size: 364 bytes, 4-byte aligned */
```

In the target `wasm32-unknown-emscripten` architecture:
- 32-bit pointers and integers (4 bytes).
- 4-byte struct alignment matches standard x86 32-bit conventions.
- Total `sizeof(LogEntry)` is **identically 364 bytes** in both native GCC and WASM.
- Endianness is Little-Endian across both x86/x64 and WebAssembly, guaranteeing zero byte-swapping complications.

---

## 4. C Bridge Layer Specification (`wasm_bridge`)

To isolate the core C modules from WebAssembly glue code, a dedicated bridge translation unit will be introduced in Milestone 8B: `include/wasm_bridge.h` and `src/wasm_bridge.c`.

```
+---------------------------------------------------------------+
|                      JavaScript / TypeScript                  |
|          Module.ccall / Module.cwrap / TypedArray             |
+-------------------------------+-------------------------------+
                                |
                                v
+---------------------------------------------------------------+
|                  LogWatch WASM Bridge Layer                   |
|                      (wasm_bridge.c)                          |
|  - JSON Serialization       - Global State Management         |
|  - Boundary Error Handling  - Memory Allocator Wrappers       |
+-------------------------------+-------------------------------+
                                |
        +-----------------------+-----------------------+
        |                       |                       |
        v                       v                       v
+---------------+       +---------------+       +---------------+
|    parser     |       |   log_array   |       |  sort/search  |
| (include/     |       | (include/     |       | (include/     |
|  parser.h)    |       |  log_array.h) |       |  sort.h, etc) |
+---------------+       +---------------+       +---------------+
```

### 4.1 Exported Bridge Functions

```c
#ifndef WASM_BRIDGE_H
#define WASM_BRIDGE_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define EMSCRIPTEN_KEEPALIVE_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define EMSCRIPTEN_KEEPALIVE_EXPORT
#endif

/* 1. Lifecycle and Dataset Ingestion */
EMSCRIPTEN_KEEPALIVE_EXPORT bool  bridge_init(void);
EMSCRIPTEN_KEEPALIVE_EXPORT bool  bridge_load_from_vfs(const char *filepath);
EMSCRIPTEN_KEEPALIVE_EXPORT bool  bridge_load_from_text(const char *content);
EMSCRIPTEN_KEEPALIVE_EXPORT void  bridge_clear(void);
EMSCRIPTEN_KEEPALIVE_EXPORT void  bridge_free(void);

/* 2. Metadata and Statistics */
EMSCRIPTEN_KEEPALIVE_EXPORT size_t bridge_get_size(void);
EMSCRIPTEN_KEEPALIVE_EXPORT size_t bridge_get_capacity(void);
EMSCRIPTEN_KEEPALIVE_EXPORT const char *bridge_get_stats_json(void);

/* 3. Record Data Retrieval */
EMSCRIPTEN_KEEPALIVE_EXPORT const char *bridge_get_records_json(size_t offset, size_t limit);
EMSCRIPTEN_KEEPALIVE_EXPORT const void *bridge_get_raw_entries_pointer(void);

/* 4. Authoritative Sorting Execution */
EMSCRIPTEN_KEEPALIVE_EXPORT const char *bridge_sort(int algorithm_id, int sort_key, bool ascending);
/* algorithm_id: 0 = Bubble, 1 = Selection, 2 = Insertion */

/* 5. Authoritative Algorithm Comparison Benchmark */
EMSCRIPTEN_KEEPALIVE_EXPORT const char *bridge_compare_all_sorts(int sort_key, bool ascending);

/* 6. Authoritative Linear Search Execution */
EMSCRIPTEN_KEEPALIVE_EXPORT const char *bridge_search_response_time(int target_ms);
EMSCRIPTEN_KEEPALIVE_EXPORT const char *bridge_search_threshold(int threshold_ms);
EMSCRIPTEN_KEEPALIVE_EXPORT const char *bridge_search_status(int status_code);
EMSCRIPTEN_KEEPALIVE_EXPORT const char *bridge_search_route(const char *route_query, bool prefix_only, bool case_sensitive);

/* 7. Memory Cleanup Helpers */
EMSCRIPTEN_KEEPALIVE_EXPORT void  bridge_free_string(char *str);

#endif /* WASM_BRIDGE_H */
```

### 4.2 Data Serialization Strategy

A hybrid serialization model is specified to balance rendering speed and ergonomic TypeScript integration:

1. **Lightweight JSON Protocol for Metadata, Metrics & Queries:**
   - Ingestion statistics, algorithm metrics (`SortMetrics`, `SearchMetrics`), and comparison benchmarks are returned as JSON strings generated in the C bridge via a bounded heap buffer.
   - Example `bridge_compare_all_sorts` response:
     ```json
     {
       "provenance": "[SYNTHETIC]",
       "record_count": 50,
       "sort_key": "response_time",
       "ascending": true,
       "algorithms": [
         {
           "name": "Bubble Sort",
           "comparisons": 1225,
           "swaps": 582,
           "shifts": 0,
           "elapsed_sec": 0.000184,
           "complexity": "O(N^2)"
         },
         {
           "name": "Selection Sort",
           "comparisons": 1225,
           "swaps": 48,
           "shifts": 0,
           "elapsed_sec": 0.000142,
           "complexity": "O(N^2)"
         },
         {
           "name": "Insertion Sort",
           "comparisons": 612,
           "swaps": 0,
           "shifts": 582,
           "elapsed_sec": 0.000088,
           "complexity": "O(N^2)"
         }
       ]
     }
     ```
2. **Paginated JSON or Direct TypedArray for Records:**
   - Table previews display bounded pages (e.g., 20 to 100 rows per page).
   - `bridge_get_records_json(offset, limit)` formats only the requested slice into JSON, keeping serialization cost under `0.1 ms`.
   - Direct memory mapping (`HEAPU8.subarray(...)`) remains available for advanced virtualization if dataset sizes exceed 20,000 records.

### 4.3 Memory Ownership Contract Across the C/JS Boundary
- **WASM Owns C Heap:** All `LogArray` structs, clones, and parse accounting buffers are owned and freed by the C runtime.
- **Bridge String Lifecycle:** Dynamic strings returned by the bridge (such as JSON payloads) are allocated via `malloc()` and freed either automatically on the subsequent bridge call (via internal static scratch buffer) or explicitly by TypeScript via `bridge_free_string()`.
- **Zero Leak Guarantee:** The C bridge provides `bridge_free()` to completely release all stored records, resetting heap occupancy to zero.

### 4.4 Clone-and-Run Architecture & Contributor Workflows

To ensure zero-friction onboarding for evaluators, students, and end users while maintaining rigorous systems-engineering discipline for contributors, LogWatch strictly partitions frontend operations into two decoupled workflows:

```
+-----------------------------------------------------------------------------------+
| NORMAL USER WORKFLOW (Zero-Toolchain Clone-and-Run)                               |
| Git Clone -> cd web -> npm ci -> npm run dev                                      |
|                                                                                   |
| Requires: Node.js >= 20.9 + npm                                                   |
| Prohibits: Requiring Emscripten SDK or native GCC compiler                        |
| Runtime Assets: Pre-compiled web/public/wasm/logwatch_wasm.js and .wasm (COMMITTED)|
| Lockfile: web/package-lock.json (COMMITTED)                                       |
| Error Policy: Explicit fatal error if WASM missing; ZERO JS DSA SUBSTITUTION      |
+-----------------------------------------------------------------------------------+
                                          |
                                          | C source code modified
                                          v
+-----------------------------------------------------------------------------------+
| CONTRIBUTOR WORKFLOW (WebAssembly Recompilation & C Verification)                 |
| 1. Edit C code (src/, include/)                                                   |
| 2. Recompile WASM: npm run build:wasm (requires Emscripten SDK / emcc)             |
| 3. Verify Bridge: npm run test:wasm (executes 12 automated bridge tests)          |
| 4. Test Native C: .\build.ps1 -Target test (independent GCC regression suite)     |
| 5. Commit updated web/public/wasm/ runtime assets alongside C changes             |
+-----------------------------------------------------------------------------------+
```

#### 1. Normal User Workflow (Clone-and-Run)
- **Prerequisites**: Git, Node.js 20.9 or later, npm. (No C compiler, no Emscripten, no Python SDK).
- **Setup & Run**:
  ```bash
  git clone https://github.com/SM33-07/LogWatch.git
  cd LogWatch/web
  npm ci
  npm run dev
  ```
- **Asset Commitment**:
  - `web/package-lock.json` is committed to guarantee deterministic, reproducible dependencies across platforms.
  - The generated WebAssembly JavaScript loader (`web/public/wasm/logwatch_wasm.js`) and binary (`web/public/wasm/logwatch_wasm.wasm`) are committed to version control.
  - `.gitignore` explicitly whitelists `!web/public/wasm/` and `!web/public/wasm/*`.
- **Runtime Guard & Error Handling**:
  - `web/lib/wasm/bridge_client.ts` verifies the presence and integrity of `/wasm/logwatch_wasm.wasm`.
  - If required WebAssembly assets are missing, `bridge_client.ts` immediately halts and displays a clear, descriptive error banner explaining how to build or restore the assets.
  - **Zero JavaScript Substitution Rule**: LogWatch under no circumstances falls back to or substitutes JavaScript implementations of the sorting, searching, parsing, or dynamic array algorithms. If WASM is unavailable, execution fails loudly.

#### 2. Contributor Workflow
- **Prerequisites**: Emscripten SDK (`emcc` 3.1+ / 6.0+), GCC 14 (for native C core), Node.js 20.9+, npm.
- **Dedicated npm Scripts**:
  - `npm run build:wasm`: Invokes `web/scripts/build_wasm.js` which detects `emcc` (or activates `C:\emsdk\emsdk_env.bat`), compiles `src/wasm_bridge.c` and core C modules into `web/public/wasm/` and syncs to `web/lib/wasm/`.
  - `npm run test:wasm`: Runs Node.js bridge integration test suite (`tests/test_wasm_bridge.js`), exercising all 12 bridge contracts, memory deallocation, and accounting invariants.
- **Asset Regeneration Contract**:
  - Whenever C algorithms (`src/sort.c`, `src/search.c`, `src/log_array.c`, `src/parser.c`) or bridge functions (`src/wasm_bridge.c`, `include/wasm_bridge.h`) are altered, contributors must run `npm run build:wasm`, verify with `npm run test:wasm`, and commit the updated `web/public/wasm/` assets.
  - Existing native C build and test workflows (`build.ps1`, `build.bat`, `Makefile`) remain completely independent and unmodified.

---

## 5. Frontend UI/UX Architecture (Next.js + TypeScript + Tailwind)

### 5.1 Design System & Theme: Dark Developer-Tool Aesthetic
To reflect a high-performance systems engineering tool (similar to Datadog, Grafana, and Xcode Instruments), the frontend adopts a dark theme with restrained violet/purple accents:

| Token | CSS Variable / Tailwind Class | Hex / Value | Semantic Role |
|---|---|---|---|
| **Background Base** | `bg-zinc-950` | `#09090b` | Root viewport background |
| **Surface Card** | `bg-zinc-900/80` | `#18181b` | Cards, panels, modal dialogue surfaces |
| **Surface Raised** | `bg-zinc-800/60` | `#27272a` | Table headers, chip backgrounds, inputs |
| **Border Subdued** | `border-zinc-800` | `#27272a` | Subtle container and grid dividers |
| **Border Active** | `border-purple-500/50`| `#a855f7` | Active focus rings, selected tabs, indicators |
| **Accent Primary** | `bg-purple-600` / `text-purple-400` | `#9333ea` / `#c084fc` | Key interactive actions, primary badges |
| **Accent Glow** | `shadow-purple-500/10` | `rgba(168,85,247,0.1)` | Subtle neon halo for active metrics |
| **Status 2xx (Success)** | `text-emerald-400` / `bg-emerald-950/40` | `#34d399` | 200 OK badges and invariant verified badges |
| **Status 3xx (Redirect)** | `text-cyan-400` / `bg-cyan-950/40` | `#22d3ee` | 301/302 Redirect badges |
| **Status 4xx (Client Err)** | `text-amber-400` / `bg-amber-950/40` | `#fbbf24` | 400/404 Bad Request / Not Found |
| **Status 5xx (Server Err)** | `text-rose-400` / `bg-rose-950/40` | `#fb7185` | 500/503 Internal Errors |
| **Typography Display** | `font-sans` (`Inter` / `Outfit`) | - | Section titles, navigation labels |
| **Typography Numeric** | `font-mono` (`JetBrains Mono`) | `font-feature-settings: "tnum"` | Monospace numbers, metrics, code, timestamps |

### 5.2 Application Route Structure (Next.js App Router)

```
web/
├── app/
│   ├── layout.tsx                # Root shell, navbar, provenance header badge
│   ├── page.tsx                  # Dashboard overview: dataset KPIs & quick actions
│   ├── explorer/
│   │   └── page.tsx              # Full-table Log Explorer with search/sort controls
│   ├── compare/
│   │   └── page.tsx              # Pedagogical Algorithm Comparison Lab
│   └── globals.css               # Design tokens, custom scrollbars, monospace rules
├── components/
│   ├── layout/
│   │   ├── Header.tsx            # Sticky app header with synthetic provenance badge
│   │   └── SidebarNav.tsx        # Compact navigation icons & state indicators
│   ├── dashboard/
│   │   ├── IngestionStatsCard.tsx# Valid/malformed/blank counters + invariant status
│   │   ├── QuickMetricsTile.tsx  # Response time percentiles, route breakdown
│   │   └── FileUploadDropzone.tsx# Drag & drop log file parser trigger
│   ├── explorer/
│   │   ├── LogTable.tsx          # High-density, virtualized monospace table
│   │   ├── SortToolbar.tsx       # Bubble / Selection / Insertion sort buttons + field selector
│   │   └── SearchFilterBar.tsx   # Multi-match criteria (status, route prefix, threshold)
│   ├── compare/
│   │   ├── BenchmarkRunner.tsx   # One-click non-destructive comparison trigger
│   │   ├── MetricsComparisonChart.tsx # Operations bar charts (comparisons, swaps, shifts)
│   │   ├── BigOTable.tsx         # Theoretical vs empirical operation matrix
│   │   └── MetricDeltaCard.tsx   # Difference counters highlighting efficiency gains
│   └── ui/
│       ├── Badge.tsx             # Semantic badge (`[SYNTHETIC]`, HTTP codes)
│       ├── Button.tsx            # Accessible purple-accented push button
│       └── StatTile.tsx          # Monospace KPI tile with trend indicators
├── lib/
│   ├── wasm/
│   │   ├── logwatch_wasm.js      # Synchronized WASM JS loader
│   │   ├── logwatch_wasm.wasm    # Synchronized C11 WebAssembly binary
│   │   └── bridge_client.ts      # Strongly typed TypeScript wrapper around C exports (with missing-asset guard)
│   └── fixtures/
│       └── sample_logs.ts        # Built-in synthetic fixtures (50-row, malformed, duplicates)
├── public/
│   └── wasm/
│       ├── logwatch_wasm.js      # [COMMITTED] Pre-compiled WebAssembly JS runtime loader
│       └── logwatch_wasm.wasm    # [COMMITTED] Pre-compiled WebAssembly binary
├── scripts/
│   └── build_wasm.js             # [CONTRIBUTOR] Emscripten compilation pipeline script
├── package.json                  # Next.js scripts + npm run build:wasm & test:wasm
├── package-lock.json             # [COMMITTED] Deterministic lockfile for npm ci
└── hooks/
    ├── useLogWatch.ts            # Global React hook managing WASM state & log dataset
    └── useSearchSort.ts          # Search query & sort state management
```

### 5.3 User Experience Flows & Wireframe Specifications

#### View 1: Ingestion Overview & Dataset Analytics
- **Hero Provenance Bar:** Prominent persistent disclosure: `LOGWATCH ACCESS ANALYZER — [SYNTHETIC DATA DEMONSTRATOR]`.
- **Ingestion Accounting Cards:**
  - Total Lines Processed
  - Valid Records Stored
  - Malformed Lines Rejected
  - Blank Lines Skipped
  - Invariant Verification Status: `total == valid + malformed + blank` (Emphasized with green badge).
- **Dataset Quick Insights:** Average response time, 95th percentile latency, HTTP status distribution (2xx / 3xx / 4xx / 5xx), unique routes count.

#### View 2: High-Density Log Explorer & Record Preview
- Monospace tabular layout displaying all fields: `Line`, `Timestamp`, `Route`, `Status`, `Response Time (ms)`, `Client ID`.
- Integrated Sorting Controls:
  - Algorithm Selector: Bubble Sort (with early exit), Selection Sort, Insertion Sort.
  - Sort Field Dropdown: Timestamp, Route, Status Code, Response Time, Client ID.
  - Direction Toggle: Ascending vs. Descending (with explicit tie-breaker reminder: "Ties broken by source line number").
- Search & Filter Controls:
  - Exact Response Time match or Threshold filter (`>= X ms`).
  - HTTP Status Code filter.
  - Route Prefix vs. Substring search toggle (with Case Sensitivity flag).
  - Multi-match counter badge showing `matches_found` / `total_records`.

#### View 3: Non-Destructive Algorithm Comparison Lab
- Direct implementation of Decision **D-18**: compares all three baseline algorithms on identical cloned arrays (`log_array_clone`).
- Side-by-side or stacked comparative metric cards:
  - **Comparisons**: Visual bar chart showing exact pairwise element comparisons.
  - **Record Swaps**: Bubble vs Selection vs Insertion swaps (highlighting Selection Sort's $O(N)$ swap advantage).
  - **Record Shifts**: Dedicated metric card for Insertion Sort element shifts.
  - **Elapsed Execution Time**: Exact microseconds/milliseconds from high-resolution C timers.
  - **Theoretical Complexity Overlay**: $O(N^2)$ average/worst-case vs $O(N)$ best-case (Bubble early exit demonstration on presorted data).

#### View 4: Data Provenance Disclosure & Academic Integrity
- Persistent visual badge in navbar header: `[SYNTHETIC]`.
- Educational disclaimer banner on file upload: *"All sample datasets and demonstrations operate exclusively on synthetic mock access logs. No real CodeVault production logs or confidential telemetry data are loaded."*

---

## 6. Implementation Feasibility & Risk Matrix

### 6.1 Technical Risk Assessment

| Risk Description | Severity | Likelihood | Mitigation Strategy |
|---|---|---|---|
| **R-1: Emscripten Toolchain Dependency on Host** | Medium | High | Pre-compile the WebAssembly binary (`logwatch_wasm.wasm` and `logwatch_wasm.js`) and commit them directly under `web/public/wasm/`. Commit `web/package-lock.json`. End users and evaluators run `npm ci` and `npm run dev` with zero C compiler or Emscripten prerequisites. If WASM assets are missing, `bridge_client.ts` raises an explicit, descriptive error; silent fallback to JS algorithm implementations is strictly prohibited. Contributors recompile via `npm run build:wasm` and verify with `npm run test:wasm`. |
| **R-2: Large File JSON Serialization Bottleneck** | Medium | Low | For large datasets (>10,000 records), implement paginated JSON serialization (`bridge_get_records_json`) or direct zero-copy `ArrayBuffer` views. Baseline lab fixtures ($\le 500$ rows) execute serialization in under `0.1 ms`. |
| **R-3: Monolithic C State Corruption Across Runs** | High | Low | Encapsulate all bridge operations with defensive initialization checks (`bridge_init`), explicit reset procedures (`bridge_clear`), and dedicated memory tear-downs. |
| **R-4: TypeScript / C Struct Alignment Drift** | High | Low | Expose field offsets and constants directly from C headers via macro getters (`LW_MAX_ROUTE`, etc.) or enforce JSON-based transfer for structured data, eliminating ABI struct layout desynchronization. |
| **R-5: Inadvertent JS Sorting / Logic Duplication** | Critical | Low | Strict architectural linting and code reviews: all sorting and search actions must dispatch directly to `bridge_client.ts`. Zero Array.prototype.sort() calls in application logic. |

### 6.2 Effort & Phased Implementation Roadmap

```
+---------------------------------------------------------------------------------+
| ROADMAP                                                                         |
|                                                                                 |
| Milestone 8A: Assessment & Architectural Plan (COMPLETE)                        |
|                - Feasibility study, bridging options, UI specification          |
|                                                                                 |
| Milestone 8B: C WebAssembly Bridge & Clone-and-Run Pipeline (COMPLETE & VERIFIED)|
|                - Implemented include/wasm_bridge.h and src/wasm_bridge.c        |
|                - Configured emcc pipeline in build.ps1, build.bat, and Makefile |
|                - Pre-compiled & committed web/public/wasm/logwatch_wasm.wasm/.js|
|                - Committed web/package-lock.json (0 vulnerabilities)            |
|                - Implemented npm run build:wasm and npm run test:wasm scripts   |
|                - bridge_client.ts missing-asset guard & zero-JS fallback policy |
|                - 12 automated Node.js bridge tests, 187 assertions (100% pass)  |
|                - Verified isolated clone-and-run workflow (npm ci + build)      |
|                                                                                 |
| Milestone 8C: Next.js Frontend Foundation & Ingestion Explorer (COMPLETE)       |
|                - Next.js App Router setup with verified clone-and-run foundation|
|                - Tailwind dark theme tokens, layout shell, [SYNTHETIC] badge    |
|                - Ingestion accounting cards & high-density monospace LogTable   |
|                                                                                 |
| Milestone 8D: Sorting/Searching Controls & Comparison Lab (COMPLETE & VERIFIED) |
|                - Interactive sorting & linear search control panels             |
|                - Non-destructive 3-way algorithm comparison benchmark lab       |
|                - Real C operation counters, horizontal comparison bars          |
|                - Academic integrity disclosures & browser/test verification     |
+---------------------------------------------------------------------------------+
```

---

## 7. Conclusions & Next Steps

1. **Feasibility Verdict:** Implementing a Next.js + Tailwind dark developer frontend for LogWatch via WebAssembly is **fully feasible, highly performant, and architecturally elegant**.
2. **Algorithmic Integrity Preserved:** Because WebAssembly compiles the authentic C11 source directly, all sorting passes, comparisons, record swaps, shifts, and linear searches execute with **100% fidelity to the native C11 codebase**.
3. **Immediate Action:**
   - Formalize architectural decisions **D-20 through D-23** in `docs/DECISIONS.md`.
   - Update `docs/IMPLEMENTATION_PLAN.md` with the Milestone 8 roadmap.
   - Retain the C11 build and test suites completely intact.
