#include "test_utils.h"
#include "search.h"
#include "parser.h"
#include "log_array.h"

static bool array_stream_handler(const LogEntry *entry, void *user_data) {
    LogArray *arr = (LogArray *)user_data;
    return log_array_append(arr, entry);
}

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
 * L-06: Empty and single-record array boundaries
 */
static void test_search_empty_and_single(void) {
    LogArray empty;
    ASSERT(log_array_init(&empty, 8), "Init empty");
    SearchMetrics m;
    size_t out[5];

    size_t count = search_linear_by_response_time(&empty, 100, out, 5, &m);
    ASSERT_EQ(count, 0, "Empty response time search returns 0");
    ASSERT_EQ(m.comparisons, 0, "Empty comparisons is 0");
    ASSERT_EQ(m.matches_found, 0, "Empty matches is 0");

    count = search_linear_by_threshold(&empty, 50, out, 5, &m);
    ASSERT_EQ(count, 0, "Empty threshold search returns 0");

    count = search_linear_by_status(&empty, 200, out, 5, &m);
    ASSERT_EQ(count, 0, "Empty status search returns 0");

    count = search_linear_by_route(&empty, "/api", out, 5, &m);
    ASSERT_EQ(count, 0, "Empty route search returns 0");

    /* Single record */
    LogArray single;
    ASSERT(log_array_init(&single, 8), "Init single");
    LogEntry e = make_entry("2026-10-09T10:00:00Z", "/api/item", 200, 42, "client-1", 1);
    ASSERT(log_array_append(&single, &e), "Append single");

    /* Match single */
    count = search_linear_by_response_time(&single, 42, out, 5, &m);
    ASSERT_EQ(count, 1, "Single matches exact response time");
    ASSERT_EQ(out[0], 0, "Single matched index is 0");
    ASSERT_EQ(m.comparisons, 1, "Single comparisons is 1");
    ASSERT_EQ(m.matches_found, 1, "Single matches found is 1");

    /* Non-match single */
    count = search_linear_by_response_time(&single, 999, out, 5, &m);
    ASSERT_EQ(count, 0, "Single non-matching response time returns 0");
    ASSERT_EQ(m.comparisons, 1, "Single comparisons is 1 even on non-match");
    ASSERT_EQ(m.matches_found, 0, "Single non-match found is 0");

    log_array_free(&empty);
    log_array_free(&single);
}

/*
 * L-01, L-03: Exact response-time matching (first, middle, last, absent)
 */
static void test_search_exact_response_time(void) {
    LogArray arr;
    ASSERT(log_array_init(&arr, 16), "Init arr");
    ParseStats stats;
    parse_stats_init(&stats);
    ASSERT(parse_file("tests/fixtures/synthetic_valid.log", array_stream_handler, &arr, &stats), "Parse valid");
    ASSERT_EQ(arr.size, 5, "Loaded 5 records");
    /* Records in synthetic_valid.log:
     * 0: 42ms
     * 1: 310ms
     * 2: 18ms
     * 3: 95ms
     * 4: 1200ms
     */

    SearchMetrics m;
    size_t out[5];

    /* First match: 42ms */
    size_t count = search_linear_by_response_time(&arr, 42, out, 5, &m);
    ASSERT_EQ(count, 1, "Exact match for first element 42ms");
    ASSERT_EQ(out[0], 0, "First element index is 0");
    ASSERT_EQ(m.comparisons, 5, "Scanned all 5 records");

    /* Middle match: 18ms (index 2) */
    count = search_linear_by_response_time(&arr, 18, out, 5, &m);
    ASSERT_EQ(count, 1, "Exact match for middle element 18ms");
    ASSERT_EQ(out[0], 2, "Middle element index is 2");

    /* Target 95ms (L-01) */
    count = search_linear_by_response_time(&arr, 95, out, 5, &m);
    ASSERT_EQ(count, 1, "L-01: Target 95ms found");
    ASSERT_EQ(out[0], 3, "L-01: Index is 3");
    ASSERT_STR_EQ(arr.entries[out[0]].route, "/api/snippets/42", "L-01: Route points to /api/snippets/42");

    /* Last match: 1200ms (index 4) */
    count = search_linear_by_response_time(&arr, 1200, out, 5, &m);
    ASSERT_EQ(count, 1, "Exact match for last element 1200ms");
    ASSERT_EQ(out[0], 4, "Last element index is 4");

    /* L-03: Absent match (9999ms) */
    count = search_linear_by_response_time(&arr, 9999, out, 5, &m);
    ASSERT_EQ(count, 0, "L-03: Target 9999ms returns 0 matches");
    ASSERT_EQ(m.matches_found, 0, "L-03: matches_found is 0");
    ASSERT_EQ(m.comparisons, 5, "L-03: comparisons is 5");

    log_array_free(&arr);
}

/*
 * Threshold search: response_time_ms >= threshold
 */
static void test_search_threshold_boundaries(void) {
    LogArray arr;
    ASSERT(log_array_init(&arr, 16), "Init arr");
    ParseStats stats;
    parse_stats_init(&stats);
    ASSERT(parse_file("tests/fixtures/synthetic_valid.log", array_stream_handler, &arr, &stats), "Parse valid");

    SearchMetrics m;
    size_t out[10];

    /* Threshold 0: matches all records (42, 310, 18, 95, 1200 all >= 0) */
    size_t count = search_linear_by_threshold(&arr, 0, out, 10, &m);
    ASSERT_EQ(count, 5, "Threshold 0 matches all 5 records");
    for (size_t i = 0; i < 5; i++) {
        ASSERT_EQ(out[i], i, "Threshold 0 preserves index sequence");
    }

    /* Threshold 100: matches 310 (idx 1) and 1200 (idx 4) */
    count = search_linear_by_threshold(&arr, 100, out, 10, &m);
    ASSERT_EQ(count, 2, "Threshold 100 matches 2 records");
    ASSERT_EQ(out[0], 1, "First >= 100 match is idx 1 (310ms)");
    ASSERT_EQ(out[1], 4, "Second >= 100 match is idx 4 (1200ms)");

    /* Threshold 1200: exact boundary match (idx 4) */
    count = search_linear_by_threshold(&arr, 1200, out, 10, &m);
    ASSERT_EQ(count, 1, "Threshold 1200 matches 1 record");
    ASSERT_EQ(out[0], 4, "Match index is 4");

    /* Threshold 1201: higher than all records */
    count = search_linear_by_threshold(&arr, 1201, out, 10, &m);
    ASSERT_EQ(count, 0, "Threshold 1201 matches 0 records");

    log_array_free(&arr);
}

/*
 * L-04: Exact HTTP status code matching
 */
static void test_search_status_code(void) {
    LogArray arr;
    ASSERT(log_array_init(&arr, 16), "Init arr");
    ParseStats stats;
    parse_stats_init(&stats);
    ASSERT(parse_file("tests/fixtures/synthetic_valid.log", array_stream_handler, &arr, &stats), "Parse valid");

    SearchMetrics m;
    size_t out[5];

    /* Status 500 (L-04): line 2, idx 1 */
    size_t count = search_linear_by_status(&arr, 500, out, 5, &m);
    ASSERT_EQ(count, 1, "L-04: Status 500 returns 1 match");
    ASSERT_EQ(out[0], 1, "L-04: Match index is 1");
    ASSERT_STR_EQ(arr.entries[out[0]].route, "/api/snippets", "L-04: Route is /api/snippets");

    /* Status 200: two matches (idx 0 and idx 3) */
    count = search_linear_by_status(&arr, 200, out, 5, &m);
    ASSERT_EQ(count, 2, "Status 200 returns 2 matches");
    ASSERT_EQ(out[0], 0, "First status 200 is idx 0");
    ASSERT_EQ(out[1], 3, "Second status 200 is idx 3");

    /* Status 404: 0 matches */
    count = search_linear_by_status(&arr, 404, out, 5, &m);
    ASSERT_EQ(count, 0, "Status 404 returns 0 matches");

    log_array_free(&arr);
}

/*
 * L-05: Route substring and prefix search, including case sensitivity
 */
static void test_search_route_behavior(void) {
    LogArray arr;
    ASSERT(log_array_init(&arr, 16), "Init arr");
    ParseStats stats;
    parse_stats_init(&stats);
    ASSERT(parse_file("tests/fixtures/synthetic_valid.log", array_stream_handler, &arr, &stats), "Parse valid");

    SearchMetrics m;
    size_t out[10];

    /* L-05: Substring "/snippets" -> matches idx 0, 1, 3 */
    size_t count = search_linear_by_route(&arr, "/snippets", out, 10, &m);
    ASSERT_EQ(count, 3, "L-05: Substring /snippets matches 3 records");
    ASSERT_EQ(out[0], 0, "First snippet match is idx 0");
    ASSERT_EQ(out[1], 1, "Second snippet match is idx 1");
    ASSERT_EQ(out[2], 3, "Third snippet match is idx 3");

    /* Prefix search: "/api" matches all 5 records */
    count = search_linear_by_route_prefix(&arr, "/api", out, 10, &m);
    ASSERT_EQ(count, 5, "Prefix /api matches all 5 records");

    /* Prefix search: "/api/auth" matches idx 2 */
    count = search_linear_by_route_prefix(&arr, "/api/auth", out, 10, &m);
    ASSERT_EQ(count, 1, "Prefix /api/auth matches 1 record");
    ASSERT_EQ(out[0], 2, "Match index is 2");

    /* Prefix search: "snippets" is NOT a prefix (route begins with "/api") */
    count = search_linear_by_route_prefix(&arr, "snippets", out, 10, &m);
    ASSERT_EQ(count, 0, "Prefix 'snippets' matches 0 records");

    /* Case sensitivity testing via search_linear_by_route_ext */
    /* Case-sensitive search for uppercase "SNIPPETS" should find 0 */
    count = search_linear_by_route_ext(&arr, "SNIPPETS", false, true, out, 10, &m);
    ASSERT_EQ(count, 0, "Case-sensitive 'SNIPPETS' matches 0");

    /* Case-insensitive search for uppercase "SNIPPETS" should find 3 */
    count = search_linear_by_route_ext(&arr, "SNIPPETS", false, false, out, 10, &m);
    ASSERT_EQ(count, 3, "Case-insensitive 'SNIPPETS' matches 3");
    ASSERT_EQ(out[0], 0, "Insensitive first match is idx 0");
    ASSERT_EQ(out[1], 1, "Insensitive second match is idx 1");
    ASSERT_EQ(out[2], 3, "Insensitive third match is idx 3");

    log_array_free(&arr);
}

/*
 * L-02: Multi-match duplicate keys and source-array order preservation
 */
static void test_search_duplicates_and_order(void) {
    LogArray arr;
    ASSERT(log_array_init(&arr, 16), "Init arr");
    ParseStats stats;
    parse_stats_init(&stats);
    ASSERT(parse_file("tests/fixtures/synthetic_duplicates.log", array_stream_handler, &arr, &stats), "Parse dup");
    ASSERT_EQ(arr.size, 5, "5 records in duplicates fixture");
    /* synthetic_duplicates.log:
     * line 1 (idx 0): 100ms
     * line 2 (idx 1): 100ms
     * line 3 (idx 2): 200ms
     * line 4 (idx 3): 100ms
     * line 5 (idx 4): 200ms
     */

    SearchMetrics m;
    size_t out[10];

    /* L-02: Target 100ms returns 3 matches at lines 1, 2, 4 (indices 0, 1, 3) */
    size_t count = search_linear_by_response_time(&arr, 100, out, 10, &m);
    ASSERT_EQ(count, 3, "L-02: Target 100ms matches 3 records");
    ASSERT_EQ(out[0], 0, "L-02: First match index is 0");
    ASSERT_EQ(out[1], 1, "L-02: Second match index is 1");
    ASSERT_EQ(out[2], 3, "L-02: Third match index is 3");

    /* Verify order strictly preserved */
    ASSERT(out[0] < out[1] && out[1] < out[2], "Source order strictly preserved");

    log_array_free(&arr);
}

/*
 * Buffer capacity safety: zero capacity and insufficient capacity
 */
static void test_search_buffer_capacity_safety(void) {
    LogArray arr;
    ASSERT(log_array_init(&arr, 16), "Init arr");
    ParseStats stats;
    parse_stats_init(&stats);
    ASSERT(parse_file("tests/fixtures/synthetic_duplicates.log", array_stream_handler, &arr, &stats), "Parse dup");

    SearchMetrics m;
    size_t out[5];

    /* Zero capacity with non-NULL out buffer */
    out[0] = 9999;
    size_t count = search_linear_by_response_time(&arr, 100, out, 0, &m);
    ASSERT_EQ(count, 3, "Zero capacity still returns total match count 3");
    ASSERT_EQ(out[0], 9999, "Zero capacity did not write to buffer");

    /* NULL out buffer with 0 capacity */
    count = search_linear_by_response_time(&arr, 100, NULL, 0, &m);
    ASSERT_EQ(count, 3, "NULL buffer returns total match count 3");

    /* Insufficient capacity: buffer size 2 when there are 3 matches */
    out[0] = 9999;
    out[1] = 9999;
    out[2] = 8888; /* Canary value to verify no overflow */
    count = search_linear_by_response_time(&arr, 100, out, 2, &m);
    ASSERT_EQ(count, 3, "Insufficient capacity returns total match count 3");
    ASSERT_EQ(out[0], 0, "First match stored");
    ASSERT_EQ(out[1], 1, "Second match stored");
    ASSERT_EQ(out[2], 8888, "Canary untouched - buffer write never overflowed!");
    ASSERT_EQ(m.matches_found, 3, "metrics.matches_found reflects total matches 3");

    log_array_free(&arr);
}

/*
 * Null inputs and invalid options error handling
 */
static void test_search_null_and_error_handling(void) {
    SearchMetrics m;
    size_t out[5];

    /* NULL arr */
    ASSERT_EQ(search_linear_by_response_time(NULL, 100, out, 5, &m), 0, "NULL arr returns 0");
    ASSERT_EQ(m.comparisons, 0, "NULL comparisons is 0");
    ASSERT_EQ(search_linear_by_threshold(NULL, 100, out, 5, &m), 0, "NULL arr threshold returns 0");
    ASSERT_EQ(search_linear_by_status(NULL, 200, out, 5, &m), 0, "NULL arr status returns 0");
    ASSERT_EQ(search_linear_by_route(NULL, "/api", out, 5, &m), 0, "NULL arr route returns 0");

    /* NULL route query */
    LogArray arr;
    ASSERT(log_array_init(&arr, 8), "Init arr");
    LogEntry e = make_entry("2026-10-09T10:00:00Z", "/api/item", 200, 42, "c-1", 1);
    ASSERT(log_array_append(&arr, &e), "Append");

    ASSERT_EQ(search_linear_by_route(&arr, NULL, out, 5, &m), 0, "NULL query returns 0");
    ASSERT_EQ(search_linear_by_route_prefix(&arr, NULL, out, 5, &m), 0, "NULL prefix query returns 0");

    /* NULL metrics pointer does not crash */
    ASSERT_EQ(search_linear_by_response_time(&arr, 42, out, 5, NULL), 1, "NULL metrics handled safely");
    ASSERT_EQ(search_linear_by_status(&arr, 200, out, 5, NULL), 1, "NULL metrics handled safely");
    search_metrics_reset(NULL); /* Safe no-op */

    log_array_free(&arr);
}

/*
 * Populate results into destination LogArray with complete field integrity
 */
static void test_search_populate_results(void) {
    LogArray src;
    ASSERT(log_array_init(&src, 16), "Init src");
    ParseStats stats;
    parse_stats_init(&stats);
    ASSERT(parse_file("tests/fixtures/synthetic_valid.log", array_stream_handler, &src, &stats), "Parse valid");

    size_t out[5];
    size_t count = search_linear_by_status(&src, 200, out, 5, NULL);
    ASSERT_EQ(count, 2, "2 records with status 200");

    LogArray results;
    ASSERT(log_array_init(&results, 8), "Init results");
    ASSERT(search_populate_results(&src, out, count, &results), "Populate results succeeds");
    ASSERT_EQ(results.size, 2, "Results array size is 2");

    /* Verify all fields of the two populated records */
    const LogEntry *r0 = &results.entries[0];
    ASSERT_EQ(r0->response_time_ms, 42, "Record 0 response time");
    ASSERT_EQ(r0->status_code, 200, "Record 0 status");
    ASSERT_STR_EQ(r0->route, "/api/snippets", "Record 0 route");
    ASSERT_STR_EQ(r0->client_id, "client-A", "Record 0 client");
    ASSERT_EQ(r0->source_line, 1, "Record 0 source line");

    const LogEntry *r1 = &results.entries[1];
    ASSERT_EQ(r1->response_time_ms, 95, "Record 1 response time");
    ASSERT_EQ(r1->status_code, 200, "Record 1 status");
    ASSERT_STR_EQ(r1->route, "/api/snippets/42", "Record 1 route");
    ASSERT_STR_EQ(r1->client_id, "client-D", "Record 1 client");
    ASSERT_EQ(r1->source_line, 4, "Record 1 source line");

    /* Error handling in populate */
    ASSERT(!search_populate_results(NULL, out, count, &results), "Populate rejects NULL src");
    ASSERT(!search_populate_results(&src, out, count, NULL), "Populate rejects NULL dest");
    ASSERT(!search_populate_results(&src, NULL, 1, &results), "Populate rejects NULL indices when count > 0");

    size_t invalid_idx[1] = {9999};
    size_t prev_size = results.size;
    ASSERT(!search_populate_results(&src, invalid_idx, 1, &results), "Populate rejects out-of-range index");
    ASSERT_EQ(results.size, prev_size, "Atomic failure: results size preserved when invalid index rejected");

    /* Self-aliasing test: src == dest */
    ASSERT(search_populate_results(&src, out, count, &src), "Self-aliasing populate (src == dest) succeeds");
    ASSERT_EQ(src.size, 7, "Self-aliasing append increased size from 5 to 7");
    ASSERT_EQ(src.entries[5].response_time_ms, 42, "Aliased item 0 matches");
    ASSERT_EQ(src.entries[6].response_time_ms, 95, "Aliased item 1 matches");

    log_array_free(&src);
    log_array_free(&results);
}

/*
 * L-07: Search metrics comparison count on larger sample
 */
static void test_search_metrics_invariants(void) {
    LogArray arr;
    ASSERT(log_array_init(&arr, 64), "Init arr");
    ParseStats stats;
    parse_stats_init(&stats);
    ASSERT(parse_file("samples/synthetic_access.log", array_stream_handler, &arr, &stats), "Parse sample 50");
    ASSERT_EQ(arr.size, 50, "Loaded 50 records");

    SearchMetrics m;
    size_t out[50];

    /* Multi-match search: exact count of comparisons must be 50 (arr->size) */
    size_t count = search_linear_by_status(&arr, 200, out, 50, &m);
    ASSERT(count > 0, "Found 200 responses in sample");
    ASSERT_EQ(m.comparisons, 50, "L-07: Exactly 50 comparisons performed");
    ASSERT_EQ(m.matches_found, count, "matches_found equals returned count");
    ASSERT(m.elapsed_sec >= 0.0, "Elapsed sec is non-negative");

    log_array_free(&arr);
}

int main(void) {
    TEST_BEGIN("LogWatch Linear Search Engine Test Suite");

    RUN_TEST(test_search_empty_and_single);
    RUN_TEST(test_search_exact_response_time);
    RUN_TEST(test_search_threshold_boundaries);
    RUN_TEST(test_search_status_code);
    RUN_TEST(test_search_route_behavior);
    RUN_TEST(test_search_duplicates_and_order);
    RUN_TEST(test_search_buffer_capacity_safety);
    RUN_TEST(test_search_null_and_error_handling);
    RUN_TEST(test_search_populate_results);
    RUN_TEST(test_search_metrics_invariants);

    return TEST_REPORT();
}
