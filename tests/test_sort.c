#include "test_utils.h"
#include "sort.h"
#include "parser.h"
#include "log_array.h"

static bool array_stream_handler(const LogEntry *entry, void *user_data) {
    LogArray *arr = (LogArray *)user_data;
    return log_array_append(arr, entry);
}

/*
 * Helper to populate a sample LogEntry record
 */
static LogEntry make_entry(const char *ts, const char *route, int status, int resp_time, const char *client, int line) {
    LogEntry e;
    memset(&e, 0, sizeof(e));
    strncpy(e.timestamp, ts, sizeof(e.timestamp) - 1);
    strncpy(e.route, route, sizeof(e.route) - 1);
    e.status_code = status;
    e.response_time_ms = resp_time;
    strncpy(e.client_id, client, sizeof(e.client_id) - 1);
    e.source_line = line;
    return e;
}

/*
 * Helper to verify that two LogArray instances are identical
 */
static bool arrays_equal(const LogArray *a, const LogArray *b) {
    if (a->size != b->size) return false;
    for (size_t i = 0; i < a->size; i++) {
        const LogEntry *e1 = &a->entries[i];
        const LogEntry *e2 = &b->entries[i];
        if (strcmp(e1->timestamp, e2->timestamp) != 0) return false;
        if (strcmp(e1->route, e2->route) != 0) return false;
        if (e1->status_code != e2->status_code) return false;
        if (e1->response_time_ms != e2->response_time_ms) return false;
        if (strcmp(e1->client_id, e2->client_id) != 0) return false;
        if (e1->source_line != e2->source_line) return false;
    }
    return true;
}

/*
 * S-01, S-02, S-03: Baseline sort correctness on synthetic_valid.log
 */
static void test_sort_baseline_valid_log(void) {
    LogArray base;
    ASSERT(log_array_init(&base, 16), "Init base array");

    LogArray arr_bubble, arr_select, arr_insert;
    ASSERT(log_array_init(&arr_bubble, 16), "Init bubble array");
    ASSERT(log_array_init(&arr_select, 16), "Init select array");
    ASSERT(log_array_init(&arr_insert, 16), "Init insert array");

    ParseStats stats;
    parse_stats_init(&stats);
    ASSERT(parse_file("tests/fixtures/synthetic_valid.log", array_stream_handler, &base, &stats), "Parse synthetic_valid.log");
    ASSERT_EQ(base.size, 5, "5 valid records ingested");

    ASSERT(log_array_clone(&base, &arr_bubble), "Clone to bubble");
    ASSERT(log_array_clone(&base, &arr_select), "Clone to select");
    ASSERT(log_array_clone(&base, &arr_insert), "Clone to insert");

    SortMetrics m_b, m_s, m_i;
    ASSERT(sort_bubble(&arr_bubble, SORT_KEY_RESPONSE_TIME, true, &m_b), "Bubble sort valid");
    ASSERT(sort_selection(&arr_select, SORT_KEY_RESPONSE_TIME, true, &m_s), "Selection sort valid");
    ASSERT(sort_insertion(&arr_insert, SORT_KEY_RESPONSE_TIME, true, &m_i), "Insertion sort valid");

    /* Expected ascending response times: 18, 42, 95, 310, 1200 */
    int expected_times[5] = {18, 42, 95, 310, 1200};
    for (size_t i = 0; i < 5; i++) {
        ASSERT_EQ(arr_bubble.entries[i].response_time_ms, expected_times[i], "Bubble sorted response time");
        ASSERT_EQ(arr_select.entries[i].response_time_ms, expected_times[i], "Select sorted response time");
        ASSERT_EQ(arr_insert.entries[i].response_time_ms, expected_times[i], "Insert sorted response time");
    }

    ASSERT(arrays_equal(&arr_bubble, &arr_select), "Bubble and Selection produce identical results");
    ASSERT(arrays_equal(&arr_bubble, &arr_insert), "Bubble and Insertion produce identical results");

    log_array_free(&base);
    log_array_free(&arr_bubble);
    log_array_free(&arr_select);
    log_array_free(&arr_insert);
}

/*
 * S-04, S-05, S-06: Strict Record Integrity Invariant Verification
 */
static void test_sort_record_integrity(void) {
    LogArray base;
    ASSERT(log_array_init(&base, 16), "Init base array");

    ParseStats stats;
    parse_stats_init(&stats);
    ASSERT(parse_file("tests/fixtures/synthetic_valid.log", array_stream_handler, &base, &stats), "Parse synthetic_valid.log");

    LogArray arr_b, arr_s, arr_i;
    ASSERT(log_array_init(&arr_b, 16), "Init arr_b");
    ASSERT(log_array_init(&arr_s, 16), "Init arr_s");
    ASSERT(log_array_init(&arr_i, 16), "Init arr_i");
    ASSERT(log_array_clone(&base, &arr_b), "Clone arr_b");
    ASSERT(log_array_clone(&base, &arr_s), "Clone arr_s");
    ASSERT(log_array_clone(&base, &arr_i), "Clone arr_i");

    sort_bubble(&arr_b, SORT_KEY_RESPONSE_TIME, true, NULL);
    sort_selection(&arr_s, SORT_KEY_RESPONSE_TIME, true, NULL);
    sort_insertion(&arr_i, SORT_KEY_RESPONSE_TIME, true, NULL);

    /*
     * Check smallest item (18ms):
     * Must be: timestamp 2026-10-09T10:00:02Z, route /api/auth/login, status 401, client-C, line 3
     */
    LogArray *arrays[3] = {&arr_b, &arr_s, &arr_i};
    const char *names[3] = {"Bubble", "Selection", "Insertion"};

    for (int k = 0; k < 3; k++) {
        const LogEntry *e0 = &arrays[k]->entries[0];
        ASSERT_EQ(e0->response_time_ms, 18, "Integrity: 18ms at index 0");
        ASSERT_STR_EQ(e0->route, "/api/auth/login", "Integrity: route preserved");
        ASSERT_EQ(e0->status_code, 401, "Integrity: status preserved");
        ASSERT_STR_EQ(e0->client_id, "client-C", "Integrity: client preserved");
        ASSERT_STR_EQ(e0->timestamp, "2026-10-09T10:00:02Z", "Integrity: timestamp preserved");
        ASSERT_EQ(e0->source_line, 3, "Integrity: source_line preserved");

        /* Largest item (1200ms) at index 4 */
        const LogEntry *e4 = &arrays[k]->entries[4];
        ASSERT_EQ(e4->response_time_ms, 1200, "Integrity: 1200ms at index 4");
        ASSERT_STR_EQ(e4->route, "/api/health", "Integrity: route preserved for largest");
        ASSERT_EQ(e4->status_code, 503, "Integrity: status preserved for largest");
        ASSERT_STR_EQ(e4->client_id, "client-E", "Integrity: client preserved for largest");
        ASSERT_STR_EQ(e4->timestamp, "2026-10-09T10:00:04Z", "Integrity: timestamp preserved for largest");
        ASSERT_EQ(e4->source_line, 5, "Integrity: source_line preserved for largest");
        (void)names;
    }

    log_array_free(&base);
    log_array_free(&arr_b);
    log_array_free(&arr_s);
    log_array_free(&arr_i);
}

/*
 * S-07, S-08: Empty and single-record array boundaries
 */
static void test_sort_empty_and_single(void) {
    LogArray empty;
    ASSERT(log_array_init(&empty, 8), "Init empty array");
    SortMetrics m;

    ASSERT(sort_bubble(&empty, SORT_KEY_RESPONSE_TIME, true, &m), "Bubble sort empty");
    ASSERT_EQ(m.comparisons, 0, "Empty: 0 comparisons");
    ASSERT_EQ(m.record_swaps, 0, "Empty: 0 swaps");

    ASSERT(sort_selection(&empty, SORT_KEY_RESPONSE_TIME, true, &m), "Selection sort empty");
    ASSERT_EQ(m.comparisons, 0, "Empty: 0 comparisons");
    ASSERT_EQ(m.record_swaps, 0, "Empty: 0 swaps");

    ASSERT(sort_insertion(&empty, SORT_KEY_RESPONSE_TIME, true, &m), "Insertion sort empty");
    ASSERT_EQ(m.comparisons, 0, "Empty: 0 comparisons");
    ASSERT_EQ(m.record_shifts, 0, "Empty: 0 shifts");

    /* Single element */
    LogArray single;
    ASSERT(log_array_init(&single, 8), "Init single array");
    LogEntry e = make_entry("2026-10-09T10:00:00Z", "/api/test", 200, 50, "c-1", 1);
    ASSERT(log_array_append(&single, &e), "Append single");

    ASSERT(sort_bubble(&single, SORT_KEY_RESPONSE_TIME, true, &m), "Bubble single");
    ASSERT_EQ(m.comparisons, 0, "Single: 0 comparisons");
    ASSERT_EQ(m.record_swaps, 0, "Single: 0 swaps");
    ASSERT_EQ(single.entries[0].response_time_ms, 50, "Single entry unchanged");

    ASSERT(sort_selection(&single, SORT_KEY_RESPONSE_TIME, true, &m), "Selection single");
    ASSERT_EQ(m.comparisons, 0, "Single: 0 comparisons");
    ASSERT_EQ(m.record_swaps, 0, "Single: 0 swaps");

    ASSERT(sort_insertion(&single, SORT_KEY_RESPONSE_TIME, true, &m), "Insertion single");
    ASSERT_EQ(m.comparisons, 0, "Single: 0 comparisons");
    ASSERT_EQ(m.record_shifts, 0, "Single: 0 shifts");

    log_array_free(&empty);
    log_array_free(&single);
}

/*
 * S-09, S-15, S-16: Already sorted array, Bubble early-exit, Selection 0-swaps, Insertion 0-shifts
 */
static void test_sort_already_sorted_metrics(void) {
    LogArray arr;
    ASSERT(log_array_init(&arr, 16), "Init sorted array");
    for (int i = 0; i < 10; i++) {
        LogEntry e = make_entry("2026-10-09T10:00:00Z", "/api/item", 200, (i + 1) * 10, "client", i + 1);
        ASSERT(log_array_append(&arr, &e), "Append sorted item");
    }

    LogArray b_arr, s_arr, i_arr;
    ASSERT(log_array_init(&b_arr, 16), "Init b");
    ASSERT(log_array_init(&s_arr, 16), "Init s");
    ASSERT(log_array_init(&i_arr, 16), "Init i");
    ASSERT(log_array_clone(&arr, &b_arr), "Clone b");
    ASSERT(log_array_clone(&arr, &s_arr), "Clone s");
    ASSERT(log_array_clone(&arr, &i_arr), "Clone i");

    SortMetrics mb, ms, mi;
    ASSERT(sort_bubble(&b_arr, SORT_KEY_RESPONSE_TIME, true, &mb), "Bubble on sorted");
    /* Bubble sort early exit: exactly 1 pass = n - 1 = 9 comparisons, 0 swaps */
    ASSERT_EQ(mb.comparisons, 9, "Bubble early exit: exactly n - 1 comparisons");
    ASSERT_EQ(mb.record_swaps, 0, "Bubble on sorted: 0 record swaps");

    ASSERT(sort_selection(&s_arr, SORT_KEY_RESPONSE_TIME, true, &ms), "Selection on sorted");
    /* Selection sort: n*(n-1)/2 = 10*9/2 = 45 comparisons, 0 self-swaps */
    ASSERT_EQ(ms.comparisons, 45, "Selection on sorted: n*(n-1)/2 comparisons");
    ASSERT_EQ(ms.record_swaps, 0, "Selection avoids unnecessary self-swaps: 0 swaps");

    ASSERT(sort_insertion(&i_arr, SORT_KEY_RESPONSE_TIME, true, &mi), "Insertion on sorted");
    /* Insertion sort best-case: n - 1 comparisons, 0 record shifts */
    ASSERT_EQ(mi.comparisons, 9, "Insertion on sorted: n - 1 comparisons");
    ASSERT_EQ(mi.record_shifts, 0, "Insertion on sorted: 0 record shifts");

    log_array_free(&arr);
    log_array_free(&b_arr);
    log_array_free(&s_arr);
    log_array_free(&i_arr);
}

/*
 * S-10: Reverse sorted array metrics and order
 */
static void test_sort_reverse_sorted(void) {
    const size_t n = 5;
    LogArray base;
    ASSERT(log_array_init(&base, 8), "Init base array");
    for (size_t i = 0; i < n; i++) {
        LogEntry e = make_entry("2026-10-09T10:00:00Z", "/api/item", 200, (int)(n - i) * 100, "client", (int)(i + 1));
        ASSERT(log_array_append(&base, &e), "Append reverse item");
    }

    LogArray b_arr, s_arr, i_arr;
    ASSERT(log_array_init(&b_arr, 8), "Init b");
    ASSERT(log_array_init(&s_arr, 8), "Init s");
    ASSERT(log_array_init(&i_arr, 8), "Init i");
    ASSERT(log_array_clone(&base, &b_arr), "Clone b");
    ASSERT(log_array_clone(&base, &s_arr), "Clone s");
    ASSERT(log_array_clone(&base, &i_arr), "Clone i");

    SortMetrics mb, ms, mi;
    ASSERT(sort_bubble(&b_arr, SORT_KEY_RESPONSE_TIME, true, &mb), "Bubble reverse");
    ASSERT(sort_selection(&s_arr, SORT_KEY_RESPONSE_TIME, true, &ms), "Selection reverse");
    ASSERT(sort_insertion(&i_arr, SORT_KEY_RESPONSE_TIME, true, &mi), "Insertion reverse");

    /* n*(n-1)/2 = 5*4/2 = 10 */
    ASSERT_EQ(mb.comparisons, 10, "Bubble worst-case comparisons");
    ASSERT_EQ(mb.record_swaps, 10, "Bubble worst-case swaps");

    ASSERT_EQ(ms.comparisons, 10, "Selection comparisons");
    ASSERT(ms.record_swaps <= n - 1, "Selection swaps <= n - 1");

    ASSERT_EQ(mi.comparisons, 10, "Insertion worst-case comparisons");
    ASSERT_EQ(mi.record_shifts, 10, "Insertion worst-case shifts");

    /* Verify correct ascending order for all three */
    for (size_t i = 0; i < n; i++) {
        int expected = (int)(i + 1) * 100;
        ASSERT_EQ(b_arr.entries[i].response_time_ms, expected, "Bubble sorted order");
        ASSERT_EQ(s_arr.entries[i].response_time_ms, expected, "Select sorted order");
        ASSERT_EQ(i_arr.entries[i].response_time_ms, expected, "Insert sorted order");
    }

    log_array_free(&base);
    log_array_free(&b_arr);
    log_array_free(&s_arr);
    log_array_free(&i_arr);
}

/*
 * S-11: Duplicate keys and deterministic tie-breaker by ascending source_line
 */
static void test_sort_duplicate_keys_and_tie_breaking(void) {
    LogArray base;
    ASSERT(log_array_init(&base, 16), "Init base array");

    /* Insert records with identical response_time 100, but differing source_line */
    LogEntry e1 = make_entry("2026-10-09T10:00:00Z", "/api/snippets", 200, 100, "client-A", 10);
    LogEntry e2 = make_entry("2026-10-09T10:00:01Z", "/api/snippets", 200, 50,  "client-B", 20);
    LogEntry e3 = make_entry("2026-10-09T10:00:02Z", "/api/snippets", 200, 100, "client-C", 5);
    LogEntry e4 = make_entry("2026-10-09T10:00:03Z", "/api/snippets", 200, 100, "client-D", 15);
    LogEntry e5 = make_entry("2026-10-09T10:00:04Z", "/api/snippets", 200, 200, "client-E", 2);

    ASSERT(log_array_append(&base, &e1), "Append 1");
    ASSERT(log_array_append(&base, &e2), "Append 2");
    ASSERT(log_array_append(&base, &e3), "Append 3");
    ASSERT(log_array_append(&base, &e4), "Append 4");
    ASSERT(log_array_append(&base, &e5), "Append 5");

    LogArray b_arr, s_arr, i_arr;
    ASSERT(log_array_init(&b_arr, 16), "Init b");
    ASSERT(log_array_init(&s_arr, 16), "Init s");
    ASSERT(log_array_init(&i_arr, 16), "Init i");

    /* Test Ascending Sort */
    ASSERT(log_array_clone(&base, &b_arr), "Clone b");
    ASSERT(log_array_clone(&base, &s_arr), "Clone s");
    ASSERT(log_array_clone(&base, &i_arr), "Clone i");

    ASSERT(sort_bubble(&b_arr, SORT_KEY_RESPONSE_TIME, true, NULL), "Bubble sort ascending");
    ASSERT(sort_selection(&s_arr, SORT_KEY_RESPONSE_TIME, true, NULL), "Selection sort ascending");
    ASSERT(sort_insertion(&i_arr, SORT_KEY_RESPONSE_TIME, true, NULL), "Insertion sort ascending");

    /*
     * Expected order:
     * Index 0: 50ms (line 20)
     * Index 1: 100ms (line 5, client-C)
     * Index 2: 100ms (line 10, client-A)
     * Index 3: 100ms (line 15, client-D)
     * Index 4: 200ms (line 2)
     */
    LogArray *arrays[3] = {&b_arr, &s_arr, &i_arr};
    for (int k = 0; k < 3; k++) {
        ASSERT_EQ(arrays[k]->entries[0].response_time_ms, 50, "Ascending index 0 response time");
        ASSERT_EQ(arrays[k]->entries[1].response_time_ms, 100, "Ascending index 1 response time");
        ASSERT_EQ(arrays[k]->entries[1].source_line, 5, "Ascending tie-break index 1 line 5");
        ASSERT_STR_EQ(arrays[k]->entries[1].client_id, "client-C", "Ascending client-C preserved");

        ASSERT_EQ(arrays[k]->entries[2].response_time_ms, 100, "Ascending index 2 response time");
        ASSERT_EQ(arrays[k]->entries[2].source_line, 10, "Ascending tie-break index 2 line 10");
        ASSERT_STR_EQ(arrays[k]->entries[2].client_id, "client-A", "Ascending client-A preserved");

        ASSERT_EQ(arrays[k]->entries[3].response_time_ms, 100, "Ascending index 3 response time");
        ASSERT_EQ(arrays[k]->entries[3].source_line, 15, "Ascending tie-break index 3 line 15");
        ASSERT_STR_EQ(arrays[k]->entries[3].client_id, "client-D", "Ascending client-D preserved");

        ASSERT_EQ(arrays[k]->entries[4].response_time_ms, 200, "Ascending index 4 response time");
    }

    /* Test Descending Sort: Tie-breaker MUST STILL BE ascending source_line! */
    ASSERT(log_array_clone(&base, &b_arr), "Clone b for desc");
    ASSERT(log_array_clone(&base, &s_arr), "Clone s for desc");
    ASSERT(log_array_clone(&base, &i_arr), "Clone i for desc");

    ASSERT(sort_bubble(&b_arr, SORT_KEY_RESPONSE_TIME, false, NULL), "Bubble sort descending");
    ASSERT(sort_selection(&s_arr, SORT_KEY_RESPONSE_TIME, false, NULL), "Selection sort descending");
    ASSERT(sort_insertion(&i_arr, SORT_KEY_RESPONSE_TIME, false, NULL), "Insertion sort descending");

    /*
     * Expected order for descending:
     * Index 0: 200ms (line 2)
     * Index 1: 100ms (line 5, client-C)
     * Index 2: 100ms (line 10, client-A)
     * Index 3: 100ms (line 15, client-D)
     * Index 4: 50ms (line 20)
     */
    for (int k = 0; k < 3; k++) {
        ASSERT_EQ(arrays[k]->entries[0].response_time_ms, 200, "Descending index 0 response time");
        ASSERT_EQ(arrays[k]->entries[1].response_time_ms, 100, "Descending index 1 response time");
        ASSERT_EQ(arrays[k]->entries[1].source_line, 5, "Descending tie-break index 1 line 5");
        ASSERT_EQ(arrays[k]->entries[2].response_time_ms, 100, "Descending index 2 response time");
        ASSERT_EQ(arrays[k]->entries[2].source_line, 10, "Descending tie-break index 2 line 10");
        ASSERT_EQ(arrays[k]->entries[3].response_time_ms, 100, "Descending index 3 response time");
        ASSERT_EQ(arrays[k]->entries[3].source_line, 15, "Descending tie-break index 3 line 15");
        ASSERT_EQ(arrays[k]->entries[4].response_time_ms, 50, "Descending index 4 response time");
    }

    log_array_free(&base);
    log_array_free(&b_arr);
    log_array_free(&s_arr);
    log_array_free(&i_arr);
}

/*
 * S-12, S-13, S-14: Test every supported key in both ascending and descending directions
 */
static void test_sort_all_keys_both_directions(void) {
    LogArray base;
    ASSERT(log_array_init(&base, 16), "Init base");
    ParseStats stats;
    parse_stats_init(&stats);
    ASSERT(parse_file("tests/fixtures/synthetic_valid.log", array_stream_handler, &base, &stats), "Parse valid log");

    SortKey keys[5] = {
        SORT_KEY_TIMESTAMP,
        SORT_KEY_ROUTE,
        SORT_KEY_STATUS_CODE,
        SORT_KEY_RESPONSE_TIME,
        SORT_KEY_CLIENT_ID
    };

    for (int k = 0; k < 5; k++) {
        SortKey key = keys[k];
        for (int dir = 0; dir < 2; dir++) {
            bool ascending = (dir == 0);

            LogArray b, s, i;
            ASSERT(log_array_init(&b, 16), "Init b");
            ASSERT(log_array_init(&s, 16), "Init s");
            ASSERT(log_array_init(&i, 16), "Init i");
            ASSERT(log_array_clone(&base, &b), "Clone b");
            ASSERT(log_array_clone(&base, &s), "Clone s");
            ASSERT(log_array_clone(&base, &i), "Clone i");

            ASSERT(sort_bubble(&b, key, ascending, NULL), "Bubble sort key");
            ASSERT(sort_selection(&s, key, ascending, NULL), "Selection sort key");
            ASSERT(sort_insertion(&i, key, ascending, NULL), "Insertion sort key");

            /* Confirm all 3 algorithms produced the exact identical final permutation */
            ASSERT(arrays_equal(&b, &s), "Bubble == Selection for key/direction");
            ASSERT(arrays_equal(&b, &i), "Bubble == Insertion for key/direction");

            /* Verify sorted invariant across all pairs */
            for (size_t idx = 0; idx + 1 < b.size; idx++) {
                int cmp = log_entry_compare(&b.entries[idx], &b.entries[idx + 1], key, ascending);
                ASSERT(cmp <= 0, "Pairwise non-decreasing order satisfied");
            }

            log_array_free(&b);
            log_array_free(&s);
            log_array_free(&i);
        }
    }

    log_array_free(&base);
}

/*
 * S-17: Non-destructive sort comparison using clones
 */
static void test_sort_non_destructive_clones(void) {
    LogArray original;
    ASSERT(log_array_init(&original, 16), "Init original");
    ParseStats stats;
    parse_stats_init(&stats);
    ASSERT(parse_file("tests/fixtures/synthetic_valid.log", array_stream_handler, &original, &stats), "Parse log");

    LogArray snapshot;
    ASSERT(log_array_init(&snapshot, 16), "Init snapshot");
    ASSERT(log_array_clone(&original, &snapshot), "Clone snapshot");

    LogArray clone1, clone2, clone3;
    ASSERT(log_array_init(&clone1, 16), "Init c1");
    ASSERT(log_array_init(&clone2, 16), "Init c2");
    ASSERT(log_array_init(&clone3, 16), "Init c3");
    ASSERT(log_array_clone(&original, &clone1), "Clone c1");
    ASSERT(log_array_clone(&original, &clone2), "Clone c2");
    ASSERT(log_array_clone(&original, &clone3), "Clone c3");

    sort_bubble(&clone1, SORT_KEY_RESPONSE_TIME, true, NULL);
    sort_selection(&clone2, SORT_KEY_RESPONSE_TIME, true, NULL);
    sort_insertion(&clone3, SORT_KEY_RESPONSE_TIME, true, NULL);

    /* Original array must be completely untouched */
    ASSERT(arrays_equal(&original, &snapshot), "Original array unaltered by sorts on clones");

    log_array_free(&original);
    log_array_free(&snapshot);
    log_array_free(&clone1);
    log_array_free(&clone2);
    log_array_free(&clone3);
}

/*
 * Large deterministic synthetic dataset test (100 records)
 */
static void test_sort_large_dataset(void) {
    const size_t count = 100;
    LogArray base;
    ASSERT(log_array_init(&base, count), "Init base array for 100 records");

    /* Deterministic pseudo-random sequence using linear congruential values */
    unsigned long seed = 42;
    for (size_t idx = 0; idx < count; idx++) {
        seed = seed * 1103515245 + 12345;
        int resp_time = (int)(seed % 5000);
        int status = 100 + (int)((seed / 10) % 500);
        char ts[32];
        snprintf(ts, sizeof(ts), "2026-10-09T10:%02d:%02dZ", (int)(idx / 60), (int)(idx % 60));
        char route[64];
        snprintf(route, sizeof(route), "/api/resource/%d", (int)(idx % 10));
        char client[32];
        snprintf(client, sizeof(client), "client-%d", (int)(idx % 7));

        LogEntry e = make_entry(ts, route, status, resp_time, client, (int)(idx + 1));
        ASSERT(log_array_append(&base, &e), "Append to large dataset");
    }
    ASSERT_EQ(base.size, count, "Appended 100 elements");

    LogArray b, s, i;
    ASSERT(log_array_init(&b, count), "Init b");
    ASSERT(log_array_init(&s, count), "Init s");
    ASSERT(log_array_init(&i, count), "Init i");
    ASSERT(log_array_clone(&base, &b), "Clone b");
    ASSERT(log_array_clone(&base, &s), "Clone s");
    ASSERT(log_array_clone(&base, &i), "Clone i");

    SortMetrics mb, ms, mi;
    ASSERT(sort_bubble(&b, SORT_KEY_RESPONSE_TIME, true, &mb), "Bubble large");
    ASSERT(sort_selection(&s, SORT_KEY_RESPONSE_TIME, true, &ms), "Selection large");
    ASSERT(sort_insertion(&i, SORT_KEY_RESPONSE_TIME, true, &mi), "Insertion large");

    ASSERT(arrays_equal(&b, &s), "Bubble == Selection on 100 elements");
    ASSERT(arrays_equal(&b, &i), "Bubble == Insertion on 100 elements");

    /* Verify order */
    for (size_t idx = 0; idx + 1 < count; idx++) {
        int cmp = log_entry_compare(&b.entries[idx], &b.entries[idx + 1], SORT_KEY_RESPONSE_TIME, true);
        ASSERT(cmp <= 0, "Large dataset non-decreasing order verified");
    }

    /* Verify Selection Sort comparison formula: n*(n-1)/2 = 100*99/2 = 4950 */
    ASSERT_EQ(ms.comparisons, 4950, "Selection sort exact comparisons: 4950");
    ASSERT(ms.record_swaps <= count - 1, "Selection swaps <= n - 1");

    log_array_free(&base);
    log_array_free(&b);
    log_array_free(&s);
    log_array_free(&i);
}

/*
 * Null inputs and invalid options error handling
 */
static void test_sort_null_and_error_handling(void) {
    SortMetrics m;
    ASSERT(!sort_bubble(NULL, SORT_KEY_RESPONSE_TIME, true, &m), "Bubble rejects NULL arr");
    ASSERT(!sort_selection(NULL, SORT_KEY_RESPONSE_TIME, true, &m), "Selection rejects NULL arr");
    ASSERT(!sort_insertion(NULL, SORT_KEY_RESPONSE_TIME, true, &m), "Insertion rejects NULL arr");

    LogArray arr;
    ASSERT(log_array_init(&arr, 8), "Init arr");
    LogEntry e = make_entry("2026-10-09T10:00:00Z", "/api/test", 200, 50, "c-1", 1);
    ASSERT(log_array_append(&arr, &e), "Append 1");
    ASSERT(log_array_append(&arr, &e), "Append 2");

    ASSERT(!sort_bubble(&arr, (SortKey)-1, true, &m), "Bubble rejects invalid key < 0");
    ASSERT(!sort_selection(&arr, (SortKey)99, true, &m), "Selection rejects invalid key 99");
    ASSERT(!sort_insertion(&arr, (SortKey)100, true, &m), "Insertion rejects invalid key 100");

    /* NULL metrics pointer is handled gracefully without crash */
    ASSERT(sort_bubble(&arr, SORT_KEY_RESPONSE_TIME, true, NULL), "Bubble handles NULL metrics");
    ASSERT(sort_selection(&arr, SORT_KEY_RESPONSE_TIME, true, NULL), "Selection handles NULL metrics");
    ASSERT(sort_insertion(&arr, SORT_KEY_RESPONSE_TIME, true, NULL), "Insertion handles NULL metrics");

    /* Key to string mapping */
    ASSERT_STR_EQ(sort_key_to_string(SORT_KEY_TIMESTAMP), "timestamp", "Key to string timestamp");
    ASSERT_STR_EQ(sort_key_to_string(SORT_KEY_ROUTE), "route", "Key to string route");
    ASSERT_STR_EQ(sort_key_to_string(SORT_KEY_STATUS_CODE), "status_code", "Key to string status");
    ASSERT_STR_EQ(sort_key_to_string(SORT_KEY_RESPONSE_TIME), "response_time_ms", "Key to string response_time");
    ASSERT_STR_EQ(sort_key_to_string(SORT_KEY_CLIENT_ID), "client_id", "Key to string client");
    ASSERT_STR_EQ(sort_key_to_string((SortKey)99), "unknown", "Key to string unknown");

    log_array_free(&arr);
}

int main(void) {
    TEST_BEGIN("LogWatch Sorting & Record Integrity Test Suite");

    RUN_TEST(test_sort_empty_and_single);
    RUN_TEST(test_sort_baseline_valid_log);
    RUN_TEST(test_sort_record_integrity);
    RUN_TEST(test_sort_already_sorted_metrics);
    RUN_TEST(test_sort_reverse_sorted);
    RUN_TEST(test_sort_duplicate_keys_and_tie_breaking);
    RUN_TEST(test_sort_all_keys_both_directions);
    RUN_TEST(test_sort_non_destructive_clones);
    RUN_TEST(test_sort_large_dataset);
    RUN_TEST(test_sort_null_and_error_handling);

    return TEST_REPORT();
}
