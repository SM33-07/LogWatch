#include "test_utils.h"
#include "log_array.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>

/* Helper to generate a deterministic synthetic record */
static void make_test_entry(LogEntry *entry, int id, int response_time) {
    snprintf(entry->timestamp, sizeof(entry->timestamp), "2026-10-09T10:00:%02dZ", id % 60);
    snprintf(entry->route, sizeof(entry->route), "/api/test/%d", id);
    entry->status_code = 200 + (id % 5);
    entry->response_time_ms = response_time;
    snprintf(entry->client_id, sizeof(entry->client_id), "client-%03d", id);
    entry->source_line = id;
}

/* 1. Initialization and empty-state invariants */
static void test_array_init_and_empty(void) {
    LogArray arr;
    bool ok = log_array_init(&arr, 16);
    ASSERT(ok, "Array initialization with capacity 16 succeeds");
    ASSERT_EQ(log_array_size(&arr), 0, "Initial size is 0");
    ASSERT_EQ(log_array_capacity(&arr), 16, "Initial capacity is 16");
    ASSERT(log_array_is_empty(&arr), "Array reports is_empty true");
    ASSERT(arr.entries != NULL, "Entries buffer is non-null");
    ASSERT_EQ(arr.provenance, PROV_SYNTHETIC, "Default provenance is PROV_SYNTHETIC");

    log_array_free(&arr);

    /* Default initialization (initial_capacity == 0 defaults to LW_INITIAL_CAPACITY) */
    ok = log_array_init(&arr, 0);
    ASSERT(ok, "Array initialization with capacity 0 succeeds");
    ASSERT_EQ(log_array_capacity(&arr), LW_INITIAL_CAPACITY, "Capacity defaults to LW_INITIAL_CAPACITY (16)");
    log_array_free(&arr);
}

/* 2. Appending one record */
static void test_array_append_single(void) {
    LogArray arr;
    log_array_init(&arr, 16);

    LogEntry entry;
    make_test_entry(&entry, 1, 42);

    bool ok = log_array_append(&arr, &entry);
    ASSERT(ok, "Appending single record succeeds");
    ASSERT_EQ(log_array_size(&arr), 1, "Size is 1 after append");
    ASSERT(!log_array_is_empty(&arr), "Array is not empty");
    ASSERT_EQ(log_array_capacity(&arr), 16, "Capacity remains 16");

    const LogEntry *retrieved = log_array_get_const(&arr, 0);
    ASSERT(retrieved != NULL, "Retrieved element 0 is non-null");
    ASSERT_STR_EQ(retrieved->route, "/api/test/1", "Route preserved");
    ASSERT_EQ(retrieved->response_time_ms, 42, "Response time preserved");
    ASSERT_STR_EQ(retrieved->client_id, "client-001", "Client ID preserved");

    log_array_free(&arr);
}

/* 3 & 4. Multiple capacity growths and field preservation */
static void test_array_growth_and_field_preservation(void) {
    LogArray arr;
    log_array_init(&arr, 16);

    /* Append 17 items into capacity 16 to trigger first resize to 32 */
    for (int i = 0; i < 17; i++) {
        LogEntry e;
        make_test_entry(&e, i + 1, (i + 1) * 10);
        bool ok = log_array_append(&arr, &e);
        ASSERT(ok, "Append in loop succeeds");
    }

    ASSERT_EQ(log_array_size(&arr), 17, "Size is 17");
    ASSERT_EQ(log_array_capacity(&arr), 32, "Capacity doubled to 32");

    /* Verify all 17 items retained their fields without corruption */
    for (int i = 0; i < 17; i++) {
        const LogEntry *e = log_array_get_const(&arr, (size_t)i);
        char expected_route[64];
        snprintf(expected_route, sizeof(expected_route), "/api/test/%d", i + 1);
        ASSERT_STR_EQ(e->route, expected_route, "Route matches after resize");
        ASSERT_EQ(e->response_time_ms, (i + 1) * 10, "Response time matches after resize");
        ASSERT_EQ(e->source_line, i + 1, "Source line preserved");
    }

    /* Continue appending up to 1000 items (triggers resizes to 64, 128, 256, 512, 1024) */
    for (int i = 17; i < 1000; i++) {
        LogEntry e;
        make_test_entry(&e, i + 1, (i + 1) * 5);
        bool ok = log_array_append(&arr, &e);
        ASSERT(ok, "Large append succeeds");
    }

    ASSERT_EQ(log_array_size(&arr), 1000, "Size is 1000");
    ASSERT(log_array_capacity(&arr) >= 1000, "Capacity is >= 1000");
    ASSERT_EQ(log_array_capacity(&arr), 1024, "Exact capacity is 1024");

    /* Check boundary elements: first, middle, last */
    const LogEntry *first = log_array_get_const(&arr, 0);
    ASSERT_STR_EQ(first->route, "/api/test/1", "First element preserved");

    const LogEntry *mid = log_array_get_const(&arr, 499);
    ASSERT_STR_EQ(mid->route, "/api/test/500", "Middle element preserved");

    const LogEntry *last = log_array_get_const(&arr, 999);
    ASSERT_STR_EQ(last->route, "/api/test/1000", "Last element preserved");
    ASSERT_EQ(last->response_time_ms, 5000, "Last response time preserved");

    log_array_free(&arr);
}

/* 5 & 6. Indexed access (first, last, out-of-range, get_at) */
static void test_array_indexed_access(void) {
    LogArray arr;
    log_array_init(&arr, 16);

    for (int i = 0; i < 5; i++) {
        LogEntry e;
        make_test_entry(&e, i + 1, (i + 1) * 100);
        log_array_append(&arr, &e);
    }

    /* First valid index (0) */
    LogEntry *e0 = log_array_get(&arr, 0);
    ASSERT(e0 != NULL, "Index 0 is valid");
    ASSERT_STR_EQ(e0->route, "/api/test/1", "Index 0 route matches");

    /* Last valid index (size - 1 = 4) */
    LogEntry *e4 = log_array_get(&arr, 4);
    ASSERT(e4 != NULL, "Index 4 is valid");
    ASSERT_STR_EQ(e4->route, "/api/test/5", "Index 4 route matches");

    /* Out of range index == size (5) */
    LogEntry *e5 = log_array_get(&arr, 5);
    ASSERT(e5 == NULL, "Index == size returns NULL");

    /* Out of range index > size (999) */
    LogEntry *e999 = log_array_get(&arr, 999);
    ASSERT(e999 == NULL, "Index > size returns NULL");

    /* get_at safe copy */
    LogEntry copied;
    bool ok = log_array_get_at(&arr, 2, &copied);
    ASSERT(ok, "log_array_get_at succeeds for index 2");
    ASSERT_STR_EQ(copied.route, "/api/test/3", "Copied route matches");

    ok = log_array_get_at(&arr, 5, &copied);
    ASSERT(!ok, "log_array_get_at returns false for index 5");

    log_array_free(&arr);
}

/* 7. Clearing and reusing an array */
static void test_array_clear_and_reuse(void) {
    LogArray arr;
    log_array_init(&arr, 16);

    for (int i = 0; i < 20; i++) {
        LogEntry e;
        make_test_entry(&e, i + 1, 10);
        log_array_append(&arr, &e);
    }

    ASSERT_EQ(log_array_size(&arr), 20, "Size is 20 before clear");
    size_t prev_capacity = log_array_capacity(&arr);
    ASSERT(prev_capacity >= 32, "Capacity is at least 32");

    /* Clear */
    log_array_clear(&arr);
    ASSERT_EQ(log_array_size(&arr), 0, "Size is 0 after clear");
    ASSERT(log_array_is_empty(&arr), "Array is empty after clear");
    ASSERT_EQ(log_array_capacity(&arr), prev_capacity, "Capacity retained after clear");
    ASSERT(log_array_get(&arr, 0) == NULL, "Access to index 0 on cleared array returns NULL");

    /* Reuse after clear */
    LogEntry new_e;
    make_test_entry(&new_e, 99, 999);
    bool ok = log_array_append(&arr, &new_e);
    ASSERT(ok, "Append after clear succeeds");
    ASSERT_EQ(log_array_size(&arr), 1, "Size is 1 after reuse append");
    ASSERT_STR_EQ(log_array_get(&arr, 0)->route, "/api/test/99", "New element accessible at index 0");

    log_array_free(&arr);
}

/* 8. Cleanup and repeated free */
static void test_array_cleanup_repeated(void) {
    LogArray arr;
    log_array_init(&arr, 16);

    LogEntry e;
    make_test_entry(&e, 1, 10);
    log_array_append(&arr, &e);

    /* First cleanup */
    log_array_free(&arr);
    ASSERT_EQ(arr.entries, NULL, "Entries pointer reset to NULL");
    ASSERT_EQ(arr.size, 0, "Size reset to 0");
    ASSERT_EQ(arr.capacity, 0, "Capacity reset to 0");

    /* Repeated cleanup must be safe no-op */
    log_array_free(&arr);
    ASSERT_EQ(arr.entries, NULL, "Entries remains NULL after second free");

    /* Cleanup of uninitialized/zeroed struct */
    LogArray zeroed = { NULL, 0, 0, PROV_SYNTHETIC };
    log_array_free(&zeroed);
    ASSERT_EQ(zeroed.entries, NULL, "Free on zeroed struct is safe");
}

/* 9. Cloning populated and empty arrays */
static void test_array_clone(void) {
    LogArray src;
    log_array_init(&src, 16);

    for (int i = 0; i < 5; i++) {
        LogEntry e;
        make_test_entry(&e, i + 1, (i + 1) * 50);
        log_array_append(&src, &e);
    }

    LogArray dest;
    bool ok = log_array_clone(&src, &dest);
    ASSERT(ok, "Cloning populated array succeeds");
    ASSERT_EQ(dest.size, 5, "Cloned size matches source");
    ASSERT(dest.capacity >= 5, "Cloned capacity is sufficient");
    ASSERT(dest.entries != src.entries, "Cloned array has distinct heap block");

    /* Field equality across all cloned records */
    for (size_t i = 0; i < 5; i++) {
        ASSERT_STR_EQ(dest.entries[i].route, src.entries[i].route, "Cloned route matches");
        ASSERT_EQ(dest.entries[i].response_time_ms, src.entries[i].response_time_ms, "Cloned response time matches");
    }

    /* Mutating dest must not affect src */
    dest.entries[0].response_time_ms = 9999;
    ASSERT_EQ(src.entries[0].response_time_ms, 50, "Mutating clone does not modify source");

    log_array_free(&src);
    log_array_free(&dest);

    /* Clone empty array */
    LogArray empty_src;
    log_array_init(&empty_src, 16);

    LogArray empty_dest;
    ok = log_array_clone(&empty_src, &empty_dest);
    ASSERT(ok, "Cloning empty array succeeds");
    ASSERT_EQ(empty_dest.size, 0, "Cloned empty size is 0");
    ASSERT(empty_dest.entries != NULL, "Cloned empty array has valid buffer");

    log_array_free(&empty_src);
    log_array_free(&empty_dest);
}

/* 10. Self-referencing aliased append */
static void test_array_self_referencing_append(void) {
    LogArray arr;
    log_array_init(&arr, 2); /* Small capacity 2 */

    LogEntry e1, e2;
    make_test_entry(&e1, 1, 100);
    make_test_entry(&e2, 2, 200);
    log_array_append(&arr, &e1);
    log_array_append(&arr, &e2);
    ASSERT_EQ(arr.size, 2, "Size is 2 (at capacity)");

    /*
     * Append an element that points directly into arr.entries[0].
     * Because the array is full, appending will realloc the buffer.
     * The implementation must safely buffer the record before realloc moves the memory.
     */
    bool ok = log_array_append(&arr, &arr.entries[0]);
    ASSERT(ok, "Self-referencing append during reallocation succeeds");
    ASSERT_EQ(arr.size, 3, "Size increased to 3");
    ASSERT_EQ(arr.capacity, 4, "Capacity doubled to 4");
    ASSERT_STR_EQ(arr.entries[2].route, "/api/test/1", "Self-appended record route preserved");
    ASSERT_EQ(arr.entries[2].response_time_ms, 100, "Self-appended response time preserved");

    log_array_free(&arr);
}

/* 11. Null argument handling */
static void test_array_null_arguments(void) {
    LogEntry dummy;
    make_test_entry(&dummy, 1, 10);

    ASSERT(!log_array_init(NULL, 16), "log_array_init(NULL) returns false");
    ASSERT(!log_array_append(NULL, &dummy), "log_array_append(NULL, entry) returns false");

    LogArray arr;
    log_array_init(&arr, 16);
    ASSERT(!log_array_append(&arr, NULL), "log_array_append(arr, NULL) returns false");
    ASSERT(log_array_get(NULL, 0) == NULL, "log_array_get(NULL, 0) returns NULL");
    ASSERT(log_array_get_const(NULL, 0) == NULL, "log_array_get_const(NULL, 0) returns NULL");
    ASSERT(!log_array_get_at(NULL, 0, &dummy), "log_array_get_at(NULL) returns false");
    ASSERT(!log_array_get_at(&arr, 0, NULL), "log_array_get_at(..., NULL) returns false");
    ASSERT_EQ(log_array_size(NULL), 0, "log_array_size(NULL) returns 0");
    ASSERT_EQ(log_array_capacity(NULL), 0, "log_array_capacity(NULL) returns 0");
    ASSERT(log_array_is_empty(NULL), "log_array_is_empty(NULL) returns true");

    log_array_clear(NULL); /* Safe no-op */
    log_array_free(NULL);  /* Safe no-op */

    ASSERT(!log_array_clone(NULL, &arr), "log_array_clone(NULL, dest) returns false");
    ASSERT(!log_array_clone(&arr, NULL), "log_array_clone(src, NULL) returns false");

    log_array_display(NULL, 5); /* Safe no-op */
    log_array_free(&arr);
}

/* 12. Parser Integration: Ingest entire 50-row synthetic sample into LogArray */
static bool array_stream_handler(const LogEntry *entry, void *user_data) {
    LogArray *arr = (LogArray *)user_data;
    return log_array_append(arr, entry);
}

static void test_array_parser_integration(void) {
    LogArray arr;
    log_array_init(&arr, 16);

    ParseStats stats;
    bool ok = parse_file("samples/synthetic_access.log", array_stream_handler, &arr, &stats);
    ASSERT(ok, "Stream-parsing samples/synthetic_access.log succeeds");
    ASSERT_EQ(stats.total_lines, 50, "50 total lines in sample");
    ASSERT_EQ(stats.valid_records, 50, "50 valid records parsed");
    ASSERT_EQ(log_array_size(&arr), 50, "LogArray contains exactly 50 records");
    ASSERT(log_array_capacity(&arr) >= 50, "LogArray capacity is >= 50");
    ASSERT_EQ(log_array_capacity(&arr), 64, "LogArray capacity grew to 64 (16 -> 32 -> 64)");

    /* Spot-check records */
    const LogEntry *first = log_array_get_const(&arr, 0);
    ASSERT_STR_EQ(first->route, "/api/health", "First record route in sample");
    ASSERT_EQ(first->response_time_ms, 5, "First record response time");

    const LogEntry *last = log_array_get_const(&arr, 49);
    ASSERT_STR_EQ(last->route, "/api/auth/logout", "50th record route in sample");
    ASSERT_EQ(last->response_time_ms, 9, "50th record response time");

    log_array_free(&arr);
}

int main(void) {
    TEST_BEGIN("LogArray Dynamic Array Test Suite");

    RUN_TEST(test_array_init_and_empty);
    RUN_TEST(test_array_append_single);
    RUN_TEST(test_array_growth_and_field_preservation);
    RUN_TEST(test_array_indexed_access);
    RUN_TEST(test_array_clear_and_reuse);
    RUN_TEST(test_array_cleanup_repeated);
    RUN_TEST(test_array_clone);
    RUN_TEST(test_array_self_referencing_append);
    RUN_TEST(test_array_null_arguments);
    RUN_TEST(test_array_parser_integration);

    return TEST_REPORT();
}
