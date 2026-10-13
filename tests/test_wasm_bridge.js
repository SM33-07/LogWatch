/**
 * LogWatch — WebAssembly Bridge Integration Test Suite [SYNTHETIC]
 *
 * Verifies all 12 Milestone 8B requirements:
 * 1. Module initialization and cleanup
 * 2. VFS file loading (MEMFS)
 * 3. Record counts and parser-accounting invariants
 * 4. Paginated record retrieval
 * 5. Bubble, Selection, and Insertion Sort ordering
 * 6. Tie-breaking (ascending source_line) and complete record preservation
 * 7. Linear search operations (exact, threshold, status, prefix, substring, case)
 * 8. Exact operation metrics for deterministic fixtures
 * 9. Non-destructive comparison benchmark on cloned inputs
 * 10. Invalid inputs, failed loads, and dataset preservation
 * 11. RFC 8259 JSON escaping and response-string ownership
 * 12. Consistency with native C11 test suite expectations
 */

const fs = require('fs');
const path = require('path');
const wasmJsPath = fs.existsSync(path.resolve(__dirname, '../web/public/wasm/logwatch_wasm.js'))
    ? path.resolve(__dirname, '../web/public/wasm/logwatch_wasm.js')
    : path.resolve(__dirname, '../web/lib/wasm/logwatch_wasm.js');
const createLogWatchModule = require(wasmJsPath);

let testsRun = 0;
let testsPassed = 0;
let assertionsRun = 0;
let assertionsPassed = 0;

function assert(condition, message) {
    assertionsRun++;
    if (!condition) {
        throw new Error(`Assertion failed: ${message}`);
    }
    assertionsPassed++;
}

function assertEq(actual, expected, message) {
    assertionsRun++;
    if (actual !== expected) {
        throw new Error(`Assertion failed: ${message} (Expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)})`);
    }
    assertionsPassed++;
}

function test(name, fn) {
    testsRun++;
    process.stdout.write(`[RUN ] ${name}\n`);
    try {
        fn();
        testsPassed++;
        process.stdout.write(`[PASS] ${name}\n`);
    } catch (err) {
        process.stdout.write(`[FAIL] ${name}\n       Error: ${err.message}\n`);
        throw err;
    }
}

async function runAllTests() {
    console.log("==================================================");
    console.log("  TEST SUITE: WebAssembly Bridge Integration [SYNTHETIC]");
    console.log("==================================================");

    const Module = await createLogWatchModule();

    // Bound C bridge wrappers with explicit memory management
    const c_init = Module.cwrap('bridge_init', 'number', []);
    const c_clear = Module.cwrap('bridge_clear', null, []);
    const c_free = Module.cwrap('bridge_free', null, []);
    const c_load_from_vfs = Module.cwrap('bridge_load_from_vfs', 'number', ['string']);
    const c_get_size = Module.cwrap('bridge_get_size', 'number', []);
    const c_get_capacity = Module.cwrap('bridge_get_capacity', 'number', []);
    const c_free_string = Module.cwrap('bridge_free_string', null, ['number']);

    // Helper to invoke C function returning dynamically allocated JSON string
    function callJsonFunc(cFunc, ...args) {
        const ptr = cFunc(...args);
        assert(ptr !== 0, "C bridge returned non-null pointer");
        const jsonStr = Module.UTF8ToString(ptr);
        c_free_string(ptr); // Explicit C memory release
        return JSON.parse(jsonStr);
    }

    const c_get_stats_json = () => callJsonFunc(Module.cwrap('bridge_get_stats_json', 'number', []));
    const c_get_records_json = (offset, limit) => callJsonFunc(Module.cwrap('bridge_get_records_json', 'number', ['number', 'number']), offset, limit);
    const c_sort = (algo, key, asc) => callJsonFunc(Module.cwrap('bridge_sort', 'number', ['number', 'number', 'number']), algo, key, asc ? 1 : 0);
    const c_compare_all_sorts = (key, asc) => callJsonFunc(Module.cwrap('bridge_compare_all_sorts', 'number', ['number', 'number']), key, asc ? 1 : 0);
    const c_search_response_time = (ms) => callJsonFunc(Module.cwrap('bridge_search_response_time', 'number', ['number']), ms);
    const c_search_threshold = (ms) => callJsonFunc(Module.cwrap('bridge_search_threshold', 'number', ['number']), ms);
    const c_search_status = (code) => callJsonFunc(Module.cwrap('bridge_search_status', 'number', ['number']), code);
    const c_search_route = (query, prefix, cs) => callJsonFunc(Module.cwrap('bridge_search_route', 'number', ['string', 'number', 'number']), query, prefix ? 1 : 0, cs ? 1 : 0);

    // Populate virtual filesystem with test fixtures
    const fixtures = [
        'synthetic_valid.log',
        'synthetic_malformed.log',
        'synthetic_duplicates.log',
        'synthetic_blanks.log',
        'synthetic_empty.log',
        'synthetic_single.log'
    ];
    for (const f of fixtures) {
        const fixturePath = path.resolve(__dirname, 'fixtures', f);
        if (fs.existsSync(fixturePath)) {
            const data = fs.readFileSync(fixturePath, 'utf8');
            Module.FS.writeFile('/' + f, data);
        }
    }
    const sampleAccessPath = path.resolve(__dirname, '..', 'samples', 'synthetic_access.log');
    if (fs.existsSync(sampleAccessPath)) {
        Module.FS.writeFile('/synthetic_access.log', fs.readFileSync(sampleAccessPath, 'utf8'));
    }

    // --- TEST 1: Lifecycle and repeated cleanup ---
    test("test_wasm_lifecycle_and_cleanup", () => {
        assertEq(c_init(), 1, "bridge_init returns 1 (true)");
        assertEq(c_get_size(), 0, "Initial size is 0");
        assert(c_get_capacity() >= 16, "Initial capacity is at least 16");

        c_clear();
        assertEq(c_get_size(), 0, "Size after clear is 0");

        c_free();
        assertEq(c_get_size(), 0, "Size after free is 0");
        assertEq(c_get_capacity(), 0, "Capacity after free is 0");

        // Repeated cleanup is safe
        c_free();
        c_clear();
        assertEq(c_init(), 1, "Re-initialization after free succeeds");
    });

    // --- TEST 2: Loading synthetic fixture through virtual filesystem ---
    test("test_wasm_vfs_fixture_loading", () => {
        assertEq(c_load_from_vfs('/synthetic_valid.log'), 1, "Loading valid fixture through VFS succeeds");
        assertEq(c_get_size(), 5, "Size is 5 records");
        assert(c_get_capacity() >= 5, "Capacity is >= 5");

        const stats = c_get_stats_json();
        assertEq(stats.provenance, "[SYNTHETIC]", "Provenance tag is [SYNTHETIC]");
        assertEq(stats.has_file, true, "has_file is true");
        assertEq(stats.filepath, "/synthetic_valid.log", "Filepath matches VFS path");
    });

    // --- TEST 3: Record counts and parser-accounting invariants ---
    test("test_wasm_parser_accounting_invariants", () => {
        // Valid fixture: 5 valid, 0 malformed, 0 blank
        c_load_from_vfs('/synthetic_valid.log');
        const statsValid = c_get_stats_json();
        assertEq(statsValid.total_lines, 5, "Valid total lines is 5");
        assertEq(statsValid.valid_records, 5, "Valid records is 5");
        assertEq(statsValid.malformed_lines, 0, "Malformed lines is 0");
        assertEq(statsValid.blank_lines, 0, "Blank lines is 0");
        assertEq(statsValid.invariant_verified, true, "Valid total == valid + malformed + blank");

        // Malformed fixture: 9 total, 3 valid, 6 malformed, 0 blank
        c_load_from_vfs('/synthetic_malformed.log');
        const statsMalf = c_get_stats_json();
        assertEq(statsMalf.total_lines, 9, "Malformed total lines is 9");
        assertEq(statsMalf.valid_records, 3, "Malformed valid records is 3");
        assertEq(statsMalf.malformed_lines, 6, "Malformed lines is 6");
        assertEq(statsMalf.blank_lines, 0, "Blank lines is 0");
        assertEq(statsMalf.invariant_verified, true, "Malformed invariant verified");

        // Blanks fixture: 6 total, 3 valid, 0 malformed, 3 blank
        c_load_from_vfs('/synthetic_blanks.log');
        const statsBlanks = c_get_stats_json();
        assertEq(statsBlanks.total_lines, 6, "Blanks total lines is 6");
        assertEq(statsBlanks.valid_records, 3, "Blanks valid records is 3");
        assertEq(statsBlanks.malformed_lines, 0, "Malformed lines is 0");
        assertEq(statsBlanks.blank_lines, 3, "Blank lines is 3");
        assertEq(statsBlanks.invariant_verified, true, "Blanks invariant verified");
    });

    // --- TEST 4: Paginated record retrieval ---
    test("test_wasm_paginated_retrieval", () => {
        c_load_from_vfs('/synthetic_valid.log'); // 5 records

        const page1 = c_get_records_json(0, 2);
        assertEq(page1.total_count, 5, "Total count is 5");
        assertEq(page1.offset, 0, "Offset is 0");
        assertEq(page1.limit, 2, "Limit is 2");
        assertEq(page1.count, 2, "Count is 2");
        assertEq(page1.records.length, 2, "Records array length is 2");
        assertEq(page1.records[0].line, 1, "First record line is 1");
        assertEq(page1.records[1].line, 2, "Second record line is 2");

        const page2 = c_get_records_json(2, 2);
        assertEq(page2.count, 2, "Page 2 count is 2");
        assertEq(page2.records[0].line, 3, "Page 2 first record line is 3");
        assertEq(page2.records[1].line, 4, "Page 2 second record line is 4");

        const page3 = c_get_records_json(4, 10);
        assertEq(page3.count, 1, "Page 3 count is 1 (bounded by remaining)");
        assertEq(page3.records[0].line, 5, "Page 3 record line is 5");

        const pageOutOfRange = c_get_records_json(10, 5);
        assertEq(pageOutOfRange.count, 0, "Out of range offset returns 0 count");
        assertEq(pageOutOfRange.records.length, 0, "Out of range records array is empty");
    });

    // --- TEST 5: Sorting algorithms correctness (Bubble, Selection, Insertion) ---
    test("test_wasm_sorting_algorithms_correctness", () => {
        const expectedAscendingMs = [18, 42, 95, 310, 1200];
        const expectedDescendingMs = [1200, 310, 95, 42, 18];

        // Bubble Sort (algo 0, key 3 = response_time, asc = true)
        c_load_from_vfs('/synthetic_valid.log');
        const resBubble = c_sort(0, 3, true);
        assertEq(resBubble.success, true, "Bubble sort succeeds");
        assertEq(resBubble.algorithm, "Bubble Sort", "Algorithm name matches");
        assert(resBubble.comparisons > 0, "Comparisons > 0");
        assert(resBubble.record_swaps > 0, "Swaps > 0");
        const rowsBubble = c_get_records_json(0, 5).records.map(r => r.response_time_ms);
        assertEq(JSON.stringify(rowsBubble), JSON.stringify(expectedAscendingMs), "Bubble Sort produces ascending order");

        // Selection Sort (algo 1, key 3, asc = true)
        c_load_from_vfs('/synthetic_valid.log');
        const resSelect = c_sort(1, 3, true);
        assertEq(resSelect.success, true, "Selection sort succeeds");
        assertEq(resSelect.algorithm, "Selection Sort", "Algorithm name matches");
        assert(resSelect.comparisons > 0, "Comparisons > 0");
        assert(resSelect.record_swaps > 0, "Swaps > 0");
        const rowsSelect = c_get_records_json(0, 5).records.map(r => r.response_time_ms);
        assertEq(JSON.stringify(rowsSelect), JSON.stringify(expectedAscendingMs), "Selection Sort produces ascending order");

        // Insertion Sort (algo 2, key 3, asc = true)
        c_load_from_vfs('/synthetic_valid.log');
        const resInsert = c_sort(2, 3, true);
        assertEq(resInsert.success, true, "Insertion sort succeeds");
        assertEq(resInsert.algorithm, "Insertion Sort", "Algorithm name matches");
        assert(resInsert.comparisons > 0, "Comparisons > 0");
        assert(resInsert.record_shifts > 0, "Shifts > 0");
        const rowsInsert = c_get_records_json(0, 5).records.map(r => r.response_time_ms);
        assertEq(JSON.stringify(rowsInsert), JSON.stringify(expectedAscendingMs), "Insertion Sort produces ascending order");

        // Descending sort test (key 3, asc = false)
        c_load_from_vfs('/synthetic_valid.log');
        const resDesc = c_sort(0, 3, false);
        assertEq(resDesc.success, true, "Descending sort succeeds");
        const rowsDesc = c_get_records_json(0, 5).records.map(r => r.response_time_ms);
        assertEq(JSON.stringify(rowsDesc), JSON.stringify(expectedDescendingMs), "Descending order matches");

        // Sort by HTTP status code (key 2)
        c_load_from_vfs('/synthetic_valid.log');
        c_sort(1, 2, true);
        const rowsStatus = c_get_records_json(0, 5).records.map(r => r.status);
        assertEq(JSON.stringify(rowsStatus), JSON.stringify([200, 200, 401, 500, 503]), "Status sort matches");
    });

    // --- TEST 6: Record integrity and deterministic tie-breaking ---
    test("test_wasm_record_integrity_and_tie_breaking", () => {
        // Record integrity check on synthetic_valid.log
        c_load_from_vfs('/synthetic_valid.log');
        c_sort(0, 3, true); // Sort by response_time ascending
        const first = c_get_records_json(0, 1).records[0];
        assertEq(first.response_time_ms, 18, "Smallest response time is 18");
        assertEq(first.route, "/api/auth/login", "Route permanently paired with record");
        assertEq(first.status, 401, "Status permanently paired with record");
        assertEq(first.client_id, "client-C", "Client ID permanently paired with record");
        assertEq(first.line, 3, "Source line preserved");

        // Tie-breaking on synthetic_duplicates.log
        // Times: 100 (line 1), 100 (line 2), 200 (line 3), 100 (line 4), 200 (line 5)
        c_load_from_vfs('/synthetic_duplicates.log');
        c_sort(0, 3, true); // Ascending sort by response_time
        const dupAsc = c_get_records_json(0, 5).records;
        assertEq(dupAsc[0].response_time_ms, 100, "Item 0 time 100");
        assertEq(dupAsc[0].line, 1, "Item 0 tie-break line 1");
        assertEq(dupAsc[1].response_time_ms, 100, "Item 1 time 100");
        assertEq(dupAsc[1].line, 2, "Item 1 tie-break line 2");
        assertEq(dupAsc[2].response_time_ms, 100, "Item 2 time 100");
        assertEq(dupAsc[2].line, 4, "Item 2 tie-break line 4");
        assertEq(dupAsc[3].response_time_ms, 200, "Item 3 time 200");
        assertEq(dupAsc[3].line, 3, "Item 3 tie-break line 3");
        assertEq(dupAsc[4].response_time_ms, 200, "Item 4 time 200");
        assertEq(dupAsc[4].line, 5, "Item 4 tie-break line 5");

        // Descending sort by response_time: primary keys descending, but ties ALWAYS ascending line!
        c_load_from_vfs('/synthetic_duplicates.log');
        c_sort(0, 3, false);
        const dupDesc = c_get_records_json(0, 5).records;
        assertEq(dupDesc[0].response_time_ms, 200, "Item 0 time 200");
        assertEq(dupDesc[0].line, 3, "Item 0 line 3 (smaller line first for equal 200)");
        assertEq(dupDesc[1].response_time_ms, 200, "Item 1 time 200");
        assertEq(dupDesc[1].line, 5, "Item 1 line 5");
        assertEq(dupDesc[2].response_time_ms, 100, "Item 2 time 100");
        assertEq(dupDesc[2].line, 1, "Item 2 line 1 (smaller line first for equal 100)");
        assertEq(dupDesc[3].response_time_ms, 100, "Item 3 time 100");
        assertEq(dupDesc[3].line, 2, "Item 3 line 2");
        assertEq(dupDesc[4].response_time_ms, 100, "Item 4 time 100");
        assertEq(dupDesc[4].line, 4, "Item 4 line 4");
    });

    // --- TEST 7: All linear search operations ---
    test("test_wasm_linear_search_operations", () => {
        c_load_from_vfs('/synthetic_valid.log');

        // 1. Exact response time
        const resExact = c_search_response_time(95);
        assertEq(resExact.matches_found, 1, "Exact search finds 1 match");
        assertEq(resExact.comparisons, 5, "Scanned all 5 records");
        assertEq(resExact.records[0].route, "/api/snippets/42", "Matched record route");

        // 2. Threshold search (>= 300ms)
        const resThresh = c_search_threshold(300);
        assertEq(resThresh.matches_found, 2, "Threshold search finds 2 matches (310, 1200)");
        assertEq(resThresh.records[0].response_time_ms, 310, "First match 310 (line 2)");
        assertEq(resThresh.records[1].response_time_ms, 1200, "Second match 1200 (line 5)");

        // 3. Exact HTTP status code
        const resStatus = c_search_status(500);
        assertEq(resStatus.matches_found, 1, "Status 500 finds 1 match");
        assertEq(resStatus.records[0].route, "/api/snippets", "Status 500 route");

        // 4. Route prefix search
        const resPrefix = c_search_route("/api/auth", true, true);
        assertEq(resPrefix.matches_found, 1, "Prefix search finds 1 match");
        assertEq(resPrefix.records[0].route, "/api/auth/login", "Matched prefix route");

        // 5. Route substring search
        const resSub = c_search_route("snippets", false, true);
        assertEq(resSub.matches_found, 3, "Substring search finds 3 matches");

        // 6. Route case-insensitive search
        const resCaseInsens = c_search_route("SNIPPETS", false, false);
        assertEq(resCaseInsens.matches_found, 3, "Case-insensitive search finds 3 matches");

        // 7. Not found
        const resNotFound = c_search_response_time(9999);
        assertEq(resNotFound.matches_found, 0, "Missing search finds 0 matches");
        assertEq(resNotFound.records.length, 0, "Missing search records array empty");
    });

    // --- TEST 8: Exact operation metrics on deterministic inputs ---
    test("test_wasm_exact_metrics_accounting", () => {
        c_load_from_vfs('/synthetic_valid.log');

        // Sort ascending once
        c_sort(0, 3, true);

        // Bubble Sort on ALREADY SORTED array: Early exit must terminate in 1 pass
        // n = 5 -> comparisons = n - 1 = 4, swaps = 0
        const resBubblePre = c_sort(0, 3, true);
        assertEq(resBubblePre.comparisons, 4, "Bubble sort on sorted array has exactly 4 comparisons");
        assertEq(resBubblePre.record_swaps, 0, "Bubble sort on sorted array has 0 swaps");

        // Insertion Sort on ALREADY SORTED array: 1 comparison per element, 0 shifts
        const resInsertPre = c_sort(2, 3, true);
        assertEq(resInsertPre.comparisons, 4, "Insertion sort on sorted array has exactly 4 comparisons");
        assertEq(resInsertPre.record_shifts, 0, "Insertion sort on sorted array has 0 shifts");

        // Selection Sort on array of size 5: always n(n-1)/2 = 10 comparisons
        const resSelect = c_sort(1, 3, true);
        assertEq(resSelect.comparisons, 10, "Selection sort has exactly 10 comparisons");
    });

    // --- TEST 9: Non-destructive algorithm comparison benchmark ---
    test("test_wasm_compare_all_sorts_benchmark", () => {
        c_load_from_vfs('/synthetic_valid.log');

        // Get snapshot of unsorted response times before comparison
        const beforeTimes = c_get_records_json(0, 5).records.map(r => r.response_time_ms);
        assertEq(JSON.stringify(beforeTimes), JSON.stringify([42, 310, 18, 95, 1200]), "Initial state is unsorted");

        const comp = c_compare_all_sorts(3, true);
        assertEq(comp.provenance, "[SYNTHETIC]", "Provenance disclosure present");
        assertEq(comp.success, true, "Comparison succeeds");
        assertEq(comp.record_count, 5, "Record count is 5");
        assertEq(comp.sort_key, "response_time_ms", "Sort key is response_time_ms");
        assertEq(comp.ascending, true, "Ascending is true");
        assertEq(comp.algorithms.length, 3, "Contains all 3 algorithms");

        const bubble = comp.algorithms.find(a => a.name === "Bubble Sort");
        const select = comp.algorithms.find(a => a.name === "Selection Sort");
        const insert = comp.algorithms.find(a => a.name === "Insertion Sort");

        assert(bubble !== undefined, "Bubble sort present");
        assert(select !== undefined, "Selection sort present");
        assert(insert !== undefined, "Insertion sort present");

        assert(bubble.comparisons > 0, "Bubble comparisons recorded");
        assert(select.comparisons === 10, "Selection comparisons is exactly 10");
        assert(insert.comparisons > 0, "Insertion comparisons recorded");

        // Non-destructive check: Active dataset MUST STILL BE IN UNSORTED STATE
        const afterTimes = c_get_records_json(0, 5).records.map(r => r.response_time_ms);
        assertEq(JSON.stringify(afterTimes), JSON.stringify(beforeTimes), "Active dataset preserved unmodified after benchmark");
    });

    // --- TEST 10: Invalid inputs, failed loads, and dataset preservation ---
    test("test_wasm_error_handling_and_dataset_preservation", () => {
        c_load_from_vfs('/synthetic_valid.log');
        assertEq(c_get_size(), 5, "Loaded 5 valid records");

        // Attempt to load nonexistent file
        const loadFail = c_load_from_vfs('/does_not_exist.log');
        assertEq(loadFail, 0, "Loading non-existent file returns 0 (false)");

        // DATASET PRESERVATION: Active array must still have the 5 records!
        assertEq(c_get_size(), 5, "Active dataset preserved after failed load");
        const records = c_get_records_json(0, 5);
        assertEq(records.count, 5, "Record count still 5");

        // Invalid sort parameters
        const invalidAlgo = c_sort(99, 0, true);
        assertEq(invalidAlgo.success, false, "Invalid algorithm_id fails cleanly");
        assert(invalidAlgo.error.length > 0, "Error message provided");

        const invalidKey = c_sort(0, 99, true);
        assertEq(invalidKey.success, false, "Invalid sort_key fails cleanly");
        assert(invalidKey.error.length > 0, "Error message provided");

        // Search on empty dataset
        c_clear();
        const searchEmpty = c_search_response_time(100);
        assertEq(searchEmpty.matches_found, 0, "Search on empty dataset returns 0");
        assertEq(searchEmpty.records.length, 0, "Empty records array");
    });

    // --- TEST 11: JSON escaping and response-string ownership ---
    test("test_wasm_json_escaping_and_memory_ownership", () => {
        // Write a log entry containing special characters: quotes, backslashes, tabs
        const specialLog = '2026-10-09T10:00:00Z|/api/quotes"and\\slashes|200|50|client"test\n';
        Module.FS.writeFile('/special.log', specialLog);

        assertEq(c_load_from_vfs('/special.log'), 1, "Loaded special characters fixture");
        const page = c_get_records_json(0, 1);
        assertEq(page.count, 1, "Count is 1");
        assertEq(page.records[0].route, '/api/quotes"and\\slashes', "Route with quotes and slashes preserved");
        assertEq(page.records[0].client_id, 'client"test', "Client ID with quotes preserved");

        // Verify explicit pointer release helper with NULL is safe
        c_free_string(0);
    });

    // --- TEST 12: 50-record sample dataset test (consistency with native) ---
    test("test_wasm_sample_access_consistency", () => {
        assertEq(c_load_from_vfs('/synthetic_access.log'), 1, "Loaded synthetic_access.log (50 records)");
        assertEq(c_get_size(), 50, "Size is 50");

        const stats = c_get_stats_json();
        assertEq(stats.total_lines, 50, "Total lines 50");
        assertEq(stats.valid_records, 50, "Valid records 50");
        assertEq(stats.malformed_lines, 0, "Malformed 0");
        assertEq(stats.blank_lines, 0, "Blank 0");
        assertEq(stats.invariant_verified, true, "Invariant holds");

        // Benchmark all 3 sorts on 50 records
        const comp = c_compare_all_sorts(3, true);
        assertEq(comp.success, true, "Comparison on 50 records succeeds");
        assertEq(comp.algorithms.length, 3, "3 algorithms evaluated");

        // Selection sort on 50 records always performs 50*49/2 = 1225 comparisons
        const select = comp.algorithms.find(a => a.name === "Selection Sort");
        assertEq(select.comparisons, 1225, "50 records Selection Sort has exactly 1225 comparisons");
    });

    console.log("--------------------------------------------------");
    console.log(`Tests run: ${testsRun} | Passed: ${testsPassed} | Failed: 0`);
    console.log(`Assertions run: ${assertionsRun} | Passed: ${assertionsPassed} | Failed: 0`);
    console.log("==================================================");
}

runAllTests().catch(err => {
    console.error("Test execution failed:", err);
    process.exit(1);
});
