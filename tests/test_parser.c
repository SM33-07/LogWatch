#include "test_utils.h"
#include "parser.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

/* P-01: Parse well-formed 5-field row */
static void test_parse_valid_row(void) {
    LogEntry entry;
    bool ok = parse_line("2026-10-09T10:00:00Z|/api/snippets|200|42|client-A", 1, &entry);
    ASSERT(ok, "P-01: Valid row should return true");
    ASSERT_STR_EQ(entry.timestamp, "2026-10-09T10:00:00Z", "Timestamp parsed correctly");
    ASSERT_STR_EQ(entry.route, "/api/snippets", "Route parsed correctly");
    ASSERT_EQ(entry.status_code, 200, "Status code parsed correctly");
    ASSERT_EQ(entry.response_time_ms, 42, "Response time parsed correctly");
    ASSERT_STR_EQ(entry.client_id, "client-A", "Client ID parsed correctly");
    ASSERT_EQ(entry.source_line, 1, "Source line preserved");
}

/* P-02: Full fixture parse of synthetic_valid.log */
static void test_fixture_valid(void) {
    ParseStats stats;
    LogEntry entries[10];
    size_t count = 0;

    bool ok = parse_file_to_buffer("tests/fixtures/synthetic_valid.log", entries, 10, &count, &stats);
    ASSERT(ok, "P-02: Parsing synthetic_valid.log should succeed");
    ASSERT_EQ(stats.total_lines, 5, "Total lines must be 5");
    ASSERT_EQ(stats.valid_records, 5, "Valid records must be 5");
    ASSERT_EQ(stats.malformed_lines, 0, "Malformed lines must be 0");
    ASSERT_EQ(stats.blank_lines, 0, "Blank lines must be 0");
    ASSERT_EQ(count, 5, "Buffer count must be 5");

    /* Verify field association for line 3 */
    ASSERT_STR_EQ(entries[2].timestamp, "2026-10-09T10:00:02Z", "Line 3 timestamp");
    ASSERT_STR_EQ(entries[2].route, "/api/auth/login", "Line 3 route");
    ASSERT_EQ(entries[2].status_code, 401, "Line 3 status code");
    ASSERT_EQ(entries[2].response_time_ms, 18, "Line 3 response time");
    ASSERT_STR_EQ(entries[2].client_id, "client-C", "Line 3 client id");
    ASSERT_EQ(entries[2].source_line, 3, "Line 3 source line");
}

/* P-03: Missing delimiter / fields */
static void test_parse_missing_fields(void) {
    LogEntry entry;
    bool ok = parse_line("2026-10-09T10:00:00Z|/api/snippets|200", 1, &entry);
    ASSERT(!ok, "P-03: Fewer than 4 delimiters must return false");
}

/* P-04: Extra delimiter / fields */
static void test_parse_extra_fields(void) {
    LogEntry entry;
    bool ok = parse_line("2026-10-09T10:00:00Z|/api/snippets|200|42|c-1|extra", 1, &entry);
    ASSERT(!ok, "P-04: More than 4 delimiters must return false");
}

/* P-05: Non-numeric status code */
static void test_parse_non_numeric_status(void) {
    LogEntry entry;
    bool ok = parse_line("2026-10-09T10:00:00Z|/api/snippets|STATUS_OK|42|c-1", 1, &entry);
    ASSERT(!ok, "P-05: Non-numeric status code must return false");
}

/* P-06: Out-of-bounds status code (high: 700) */
static void test_parse_status_too_high(void) {
    LogEntry entry;
    bool ok = parse_line("2026-10-09T10:00:00Z|/api/snippets|700|42|c-1", 1, &entry);
    ASSERT(!ok, "P-06: Status code > 599 must return false");
}

/* P-07: Out-of-bounds status code (low: 99) */
static void test_parse_status_too_low(void) {
    LogEntry entry;
    bool ok = parse_line("2026-10-09T10:00:00Z|/api/snippets|99|42|c-1", 1, &entry);
    ASSERT(!ok, "P-07: Status code < 100 must return false");
}

/* P-08: Negative response time */
static void test_parse_negative_response_time(void) {
    LogEntry entry;
    bool ok = parse_line("2026-10-09T10:00:00Z|/api/snippets|200|-15|c-1", 1, &entry);
    ASSERT(!ok, "P-08: Negative response time must return false");
}

/* P-09: Empty route string */
static void test_parse_empty_route(void) {
    LogEntry entry;
    bool ok = parse_line("2026-10-09T10:00:00Z||200|42|c-1", 1, &entry);
    ASSERT(!ok, "P-09: Empty route string must return false");
}

/* P-10: Route not starting with '/' */
static void test_parse_route_no_slash(void) {
    LogEntry entry;
    bool ok = parse_line("2026-10-09T10:00:00Z|api/snippets|200|42|c-1", 1, &entry);
    ASSERT(!ok, "P-10: Route without leading slash must return false");
}

/* P-11: Blank and whitespace-only lines fixture */
static void test_fixture_blanks(void) {
    ParseStats stats;
    bool ok = parse_file_stats("tests/fixtures/synthetic_blanks.log", &stats);
    ASSERT(ok, "P-11: Parsing synthetic_blanks.log should succeed");
    ASSERT_EQ(stats.total_lines, 6, "Total lines must be 6");
    ASSERT_EQ(stats.valid_records, 3, "Valid records must be 3");
    ASSERT_EQ(stats.blank_lines, 3, "Blank lines must be 3");
    ASSERT_EQ(stats.malformed_lines, 0, "Malformed lines must be 0");
    ASSERT_EQ(stats.total_lines, stats.valid_records + stats.malformed_lines + stats.blank_lines,
              "Accounting invariant holds on blanks fixture");
}

/* P-12: Corrupted mixed fixture */
static void test_fixture_malformed(void) {
    ParseStats stats;
    bool ok = parse_file_stats("tests/fixtures/synthetic_malformed.log", &stats);
    ASSERT(ok, "P-12: Parsing synthetic_malformed.log should succeed without crash");
    ASSERT_EQ(stats.total_lines, 9, "Total lines must be 9");
    ASSERT_EQ(stats.valid_records, 3, "Valid records must be 3");
    ASSERT_EQ(stats.malformed_lines, 6, "Malformed lines must be 6");
    ASSERT_EQ(stats.blank_lines, 0, "Blank lines must be 0");
    ASSERT_EQ(stats.total_lines, stats.valid_records + stats.malformed_lines + stats.blank_lines,
              "Accounting invariant holds on malformed fixture");
}

/* P-13: Non-existent file */
static void test_non_existent_file(void) {
    ParseStats stats;
    bool ok = parse_file_stats("non_existent_log_file.log", &stats);
    ASSERT(!ok, "P-13: Non-existent file must return false");
    ASSERT_EQ(stats.total_lines, 0, "Failed open leaves stats at zero");
}

/* Boundary: Empty file */
static void test_fixture_empty(void) {
    ParseStats stats;
    bool ok = parse_file_stats("tests/fixtures/synthetic_empty.log", &stats);
    ASSERT(ok, "Empty file parsing should succeed");
    ASSERT_EQ(stats.total_lines, 0, "Empty file total_lines == 0");
    ASSERT_EQ(stats.valid_records, 0, "Empty file valid_records == 0");
    ASSERT_EQ(stats.malformed_lines, 0, "Empty file malformed_lines == 0");
    ASSERT_EQ(stats.blank_lines, 0, "Empty file blank_lines == 0");
    ASSERT_EQ(stats.total_lines, stats.valid_records + stats.malformed_lines + stats.blank_lines,
              "Accounting invariant holds on empty file");
}

/* Boundary: Single record file */
static void test_fixture_single(void) {
    ParseStats stats;
    LogEntry entries[2];
    size_t count = 0;

    bool ok = parse_file_to_buffer("tests/fixtures/synthetic_single.log", entries, 2, &count, &stats);
    ASSERT(ok, "Single file parsing should succeed");
    ASSERT_EQ(stats.total_lines, 1, "Single line total_lines == 1");
    ASSERT_EQ(stats.valid_records, 1, "Single line valid_records == 1");
    ASSERT_EQ(stats.malformed_lines, 0, "Single line malformed_lines == 0");
    ASSERT_EQ(stats.blank_lines, 0, "Single line blank_lines == 0");
    ASSERT_EQ(count, 1, "Stored count == 1");
    ASSERT_STR_EQ(entries[0].route, "/api/snippets", "Single record route check");
    ASSERT_EQ(stats.total_lines, stats.valid_records + stats.malformed_lines + stats.blank_lines,
              "Accounting invariant holds on single file");
}

/* Boundary: Overlong lines and non-newline terminations */
static void test_overlong_and_partial_lines(void) {
    const char *tmp_filename = "tests/fixtures/temp_overlong_test.log";
    FILE *fp = fopen(tmp_filename, "w");
    ASSERT(fp != NULL, "Created temporary fixture for overlong line test");

    /* Line 1: Normal valid line */
    fprintf(fp, "2026-10-09T10:00:00Z|/api/valid1|200|10|client-1\n");

    /* Line 2: Overlong line (>1024 characters) */
    fprintf(fp, "2026-10-09T10:00:01Z|/api/overlong|");
    for (int i = 0; i < 1100; i++) {
        fputc('A', fp);
    }
    fprintf(fp, "|200|20|client-2\n");

    /* Line 3: Normal valid line following overlong line */
    fprintf(fp, "2026-10-09T10:00:02Z|/api/valid2|200|30|client-3\n");

    /* Line 4: Final valid line WITHOUT newline at EOF */
    fprintf(fp, "2026-10-09T10:00:03Z|/api/valid3|200|40|client-4");

    fclose(fp);

    ParseStats stats;
    LogEntry entries[10];
    size_t count = 0;

    bool ok = parse_file_to_buffer(tmp_filename, entries, 10, &count, &stats);
    ASSERT(ok, "Parsed overlong test file successfully");
    ASSERT_EQ(stats.total_lines, 4, "Total lines: 4 lines expected");
    ASSERT_EQ(stats.valid_records, 3, "Valid records: 3 valid records expected");
    ASSERT_EQ(stats.malformed_lines, 1, "Malformed lines: 1 overlong line rejected");
    ASSERT_EQ(stats.blank_lines, 0, "Blank lines: 0");
    ASSERT_EQ(count, 3, "Buffer count must be 3");

    /* Check that valid records before and after overlong line were preserved */
    ASSERT_STR_EQ(entries[0].route, "/api/valid1", "Record 1 route preserved");
    ASSERT_STR_EQ(entries[1].route, "/api/valid2", "Record 2 route preserved after overlong line");
    ASSERT_STR_EQ(entries[2].route, "/api/valid3", "Record 3 route preserved without EOF newline");
    ASSERT_EQ(stats.total_lines, stats.valid_records + stats.malformed_lines + stats.blank_lines,
              "Accounting invariant holds on overlong fixture");

    remove(tmp_filename);
}

/* Boundary: Overlong final line without newline */
static void test_overlong_final_line_no_newline(void) {
    const char *tmp_filename = "tests/fixtures/temp_overlong_eof.log";
    FILE *fp = fopen(tmp_filename, "w");
    ASSERT(fp != NULL, "Created temporary fixture for overlong EOF test");

    /* Line 1: Normal valid line */
    fprintf(fp, "2026-10-09T10:00:00Z|/api/valid1|200|10|client-1\n");

    /* Line 2: Overlong line without newline at EOF */
    fprintf(fp, "2026-10-09T10:00:01Z|/api/overlong|");
    for (int i = 0; i < 1100; i++) {
        fputc('B', fp);
    }

    fclose(fp);

    ParseStats stats;
    bool ok = parse_file_stats(tmp_filename, &stats);
    ASSERT(ok, "Parsed overlong EOF file successfully");
    ASSERT_EQ(stats.total_lines, 2, "Total lines: 2");
    ASSERT_EQ(stats.valid_records, 1, "Valid records: 1");
    ASSERT_EQ(stats.malformed_lines, 1, "Malformed lines: 1");
    ASSERT_EQ(stats.total_lines, stats.valid_records + stats.malformed_lines + stats.blank_lines,
              "Accounting invariant holds on overlong EOF fixture");

    remove(tmp_filename);
}

/* Check 1: Comprehensive Timestamp and Calendar Validation */
static void test_timestamp_calendar_validation(void) {
    LogEntry entry;

    /* Leap year February 29th (2024 is leap) */
    ASSERT(parse_line("2024-02-29T10:00:00Z|/api/test|200|10|client", 1, &entry),
           "2024-02-29 is valid leap day");

    /* Non-leap year February 29th (2026 is non-leap) */
    ASSERT(!parse_line("2026-02-29T10:00:00Z|/api/test|200|10|client", 1, &entry),
           "2026-02-29 is impossible calendar date (non-leap year)");

    /* February 30th */
    ASSERT(!parse_line("2024-02-30T10:00:00Z|/api/test|200|10|client", 1, &entry),
           "February 30th is impossible in any year");

    /* 30-day months with day 31: April (04), June (06), September (09), November (11) */
    ASSERT(!parse_line("2026-04-31T10:00:00Z|/api/test|200|10|client", 1, &entry),
           "April 31st is impossible");
    ASSERT(!parse_line("2026-06-31T10:00:00Z|/api/test|200|10|client", 1, &entry),
           "June 31st is impossible");
    ASSERT(!parse_line("2026-09-31T10:00:00Z|/api/test|200|10|client", 1, &entry),
           "September 31st is impossible");
    ASSERT(!parse_line("2026-11-31T10:00:00Z|/api/test|200|10|client", 1, &entry),
           "November 31st is impossible");

    /* Month 00 or 13 */
    ASSERT(!parse_line("2026-00-15T10:00:00Z|/api/test|200|10|client", 1, &entry),
           "Month 00 is invalid");
    ASSERT(!parse_line("2026-13-15T10:00:00Z|/api/test|200|10|client", 1, &entry),
           "Month 13 is invalid");

    /* Day 00 or 32 */
    ASSERT(!parse_line("2026-10-00T10:00:00Z|/api/test|200|10|client", 1, &entry),
           "Day 00 is invalid");
    ASSERT(!parse_line("2026-10-32T10:00:00Z|/api/test|200|10|client", 1, &entry),
           "Day 32 is invalid");

    /* Time components: Hour 24, Minute 60, Second 60 */
    ASSERT(!parse_line("2026-10-09T24:00:00Z|/api/test|200|10|client", 1, &entry),
           "Hour 24 is invalid");
    ASSERT(!parse_line("2026-10-09T10:60:00Z|/api/test|200|10|client", 1, &entry),
           "Minute 60 is invalid");
    ASSERT(!parse_line("2026-10-09T10:00:60Z|/api/test|200|10|client", 1, &entry),
           "Second 60 is invalid");

    /* Timezone offset non-Z */
    ASSERT(!parse_line("2026-10-09T10:00:00+00|/api/test|200|10|client", 1, &entry),
           "Non-UTC timestamp must be rejected");
    ASSERT(!parse_line("2026-10-09T10:00:00X|/api/test|200|10|client", 1, &entry),
           "Non-Z suffix must be rejected");
}

/* Check 2: Numeric Safety and Overflow Guarding */
static void test_numeric_safety_and_overflow(void) {
    LogEntry entry;

    /* Status boundary: 99 rejected, 100 accepted, 599 accepted, 600 rejected */
    ASSERT(!parse_line("2026-10-09T10:00:00Z|/api/test|99|10|client", 1, &entry),
           "Status 99 rejected");
    ASSERT(parse_line("2026-10-09T10:00:00Z|/api/test|100|10|client", 1, &entry),
           "Status 100 accepted");
    ASSERT(parse_line("2026-10-09T10:00:00Z|/api/test|599|10|client", 1, &entry),
           "Status 599 accepted");
    ASSERT(!parse_line("2026-10-09T10:00:00Z|/api/test|600|10|client", 1, &entry),
           "Status 600 rejected");

    /* Signs in status code */
    ASSERT(!parse_line("2026-10-09T10:00:00Z|/api/test|+200|10|client", 1, &entry),
           "Status with leading + sign rejected");
    ASSERT(!parse_line("2026-10-09T10:00:00Z|/api/test|-200|10|client", 1, &entry),
           "Status with leading - sign rejected");

    /* Trailing letters in status */
    ASSERT(!parse_line("2026-10-09T10:00:00Z|/api/test|200OK|10|client", 1, &entry),
           "Status with trailing letters rejected");

    /* Response time 0 accepted */
    ASSERT(parse_line("2026-10-09T10:00:00Z|/api/test|200|0|client", 1, &entry),
           "Response time 0 accepted");
    ASSERT_EQ(entry.response_time_ms, 0, "Response time is 0");

    /* Response time INT_MAX (2147483647) accepted */
    ASSERT(parse_line("2026-10-09T10:00:00Z|/api/test|200|2147483647|client", 1, &entry),
           "Response time INT_MAX accepted");
    ASSERT_EQ(entry.response_time_ms, INT_MAX, "Response time matches INT_MAX");

    /* Response time INT_MAX + 1 (2147483648) rejected */
    ASSERT(!parse_line("2026-10-09T10:00:00Z|/api/test|200|2147483648|client", 1, &entry),
           "Response time exceeding INT_MAX rejected without overflow");

    /* Response time very large (9999999999) rejected without overflow */
    ASSERT(!parse_line("2026-10-09T10:00:00Z|/api/test|200|9999999999|client", 1, &entry),
           "Response time 9999999999 rejected safely");

    /* Response time with sign or trailing units */
    ASSERT(!parse_line("2026-10-09T10:00:00Z|/api/test|200|+50|client", 1, &entry),
           "Response time with + sign rejected");
    ASSERT(!parse_line("2026-10-09T10:00:00Z|/api/test|200|50ms|client", 1, &entry),
           "Response time with trailing units rejected");
}

/* Check 3: String Limits and Physical Line Boundaries */
static void test_exact_field_and_physical_limits(void) {
    LogEntry entry;

    /* Route maximum length: buffer is LW_MAX_ROUTE (256), so max string length is 255 */
    char route_255[256];
    route_255[0] = '/';
    memset(route_255 + 1, 'x', 254);
    route_255[255] = '\0'; /* Length 255 */

    char line_buf[1024];
    snprintf(line_buf, sizeof(line_buf), "2026-10-09T10:00:00Z|%s|200|10|client", route_255);
    ASSERT(parse_line(line_buf, 1, &entry), "Route of length 255 accepted");
    ASSERT_EQ(strlen(entry.route), 255, "Route stored with full 255 characters");

    /* Route length 256 (exceeds buffer) */
    char route_256[257];
    route_256[0] = '/';
    memset(route_256 + 1, 'x', 255);
    route_256[256] = '\0'; /* Length 256 */
    snprintf(line_buf, sizeof(line_buf), "2026-10-09T10:00:00Z|%s|200|10|client", route_256);
    ASSERT(!parse_line(line_buf, 1, &entry), "Route of length 256 rejected without truncation");

    /* Client ID maximum length: buffer is LW_MAX_CLIENT_ID (64), so max string length is 63 */
    char client_63[64];
    memset(client_63, 'c', 63);
    client_63[63] = '\0';
    snprintf(line_buf, sizeof(line_buf), "2026-10-09T10:00:00Z|/api/test|200|10|%s", client_63);
    ASSERT(parse_line(line_buf, 1, &entry), "Client ID of length 63 accepted");
    ASSERT_EQ(strlen(entry.client_id), 63, "Client ID stored with full 63 characters");

    /* Client ID length 64 (exceeds buffer) */
    char client_64[65];
    memset(client_64, 'c', 64);
    client_64[64] = '\0';
    snprintf(line_buf, sizeof(line_buf), "2026-10-09T10:00:00Z|/api/test|200|10|%s", client_64);
    ASSERT(!parse_line(line_buf, 1, &entry), "Client ID of length 64 rejected without truncation");

    /* CRLF input verification */
    ASSERT(parse_line("2026-10-09T10:00:00Z|/api/test|200|10|client\r\n", 1, &entry),
           "CRLF line parsed successfully");
    ASSERT_STR_EQ(entry.client_id, "client", "Client ID does not retain trailing carriage return");
}

/* Check 4 & 5: Buffer Capacities, Streaming Early Termination, and Accounting Invariants */
static bool stopping_handler(const LogEntry *entry, void *user_data) {
    (void)entry;
    int *count = (int *)user_data;
    (*count)++;
    if (*count >= 2) {
        return false; /* Request stop after 2 records */
    }
    return true;
}

static void test_api_capacities_and_stream_stop(void) {
    /* 1. Zero-capacity buffer */
    ParseStats stats;
    LogEntry dummy_entries[1];
    size_t out_count = 999;

    bool ok = parse_file_to_buffer("tests/fixtures/synthetic_valid.log", dummy_entries, 0, &out_count, &stats);
    ASSERT(ok, "parse_file_to_buffer succeeds with max_entries == 0");
    ASSERT_EQ(out_count, 0, "Zero records stored when max_entries == 0");
    ASSERT_EQ(stats.valid_records, 5, "All 5 records in file accounted in stats");

    /* 2. Insufficient buffer capacity (max_entries == 2 for a 5-record file) */
    LogEntry small_buffer[2];
    out_count = 0;
    ok = parse_file_to_buffer("tests/fixtures/synthetic_valid.log", small_buffer, 2, &out_count, &stats);
    ASSERT(ok, "parse_file_to_buffer succeeds with insufficient buffer");
    ASSERT_EQ(out_count, 2, "Stored count strictly capped at max_entries == 2");
    ASSERT_EQ(stats.valid_records, 5, "Total valid records in file remains 5");
    ASSERT_STR_EQ(small_buffer[0].route, "/api/snippets", "First record stored correctly");
    ASSERT_STR_EQ(small_buffer[1].route, "/api/snippets", "Second record stored correctly");

    /* 3. Streaming early termination */
    int records_seen = 0;
    ok = parse_file("tests/fixtures/synthetic_valid.log", stopping_handler, &records_seen, &stats);
    ASSERT(ok, "parse_file succeeds when handler requests early stop");
    ASSERT_EQ(records_seen, 2, "Handler processed exactly 2 records before stopping");

    /* 4. Cross-fixture accounting invariant check */
    const char *fixtures[] = {
        "tests/fixtures/synthetic_valid.log",
        "tests/fixtures/synthetic_malformed.log",
        "tests/fixtures/synthetic_blanks.log",
        "tests/fixtures/synthetic_empty.log",
        "tests/fixtures/synthetic_single.log",
        "tests/fixtures/synthetic_duplicates.log"
    };

    for (size_t i = 0; i < 6; i++) {
        ParseStats s;
        ok = parse_file_stats(fixtures[i], &s);
        ASSERT(ok, "Fixture scanned successfully");
        ASSERT_EQ(s.total_lines, s.valid_records + s.malformed_lines + s.blank_lines,
                  "Accounting invariant holds across all standard fixtures");
    }
}

/* Null pointer safety */
static void test_null_pointer_safety(void) {
    LogEntry entry;
    ParseStats stats;
    size_t out_count = 0;

    ASSERT(!parse_line(NULL, 1, &entry), "parse_line with NULL line returns false");
    ASSERT(!parse_line("2026-10-09T10:00:00Z|/api|200|1|c", 1, NULL), "parse_line with NULL out returns false");
    ASSERT(!parse_file(NULL, NULL, NULL, &stats), "parse_file with NULL filepath returns false");
    ASSERT(!parse_file("tests/fixtures/synthetic_valid.log", NULL, NULL, NULL), "parse_file with NULL stats returns false");
    ASSERT(!parse_file_to_buffer(NULL, &entry, 1, &out_count, &stats), "parse_file_to_buffer with NULL path returns false");
    ASSERT(!parse_file_to_buffer("tests/fixtures/synthetic_valid.log", NULL, 1, &out_count, &stats), "parse_file_to_buffer with NULL entries returns false");
    ASSERT(!parse_file_to_buffer("tests/fixtures/synthetic_valid.log", &entry, 1, NULL, &stats), "parse_file_to_buffer with NULL out_count returns false");
    ASSERT(!parse_file_stats(NULL, &stats), "parse_file_stats with NULL path returns false");
    ASSERT(!parse_file_stats("tests/fixtures/synthetic_valid.log", NULL), "parse_file_stats with NULL stats returns false");
    ASSERT(!log_entry_validate(NULL), "log_entry_validate with NULL returns false");
    ASSERT(parse_is_blank_line(NULL), "parse_is_blank_line with NULL returns true");

    parse_stats_init(NULL); /* Safe no-op */
    parse_stats_print(NULL, NULL, PROV_SYNTHETIC); /* Safe no-op */
}

int main(void) {
    TEST_BEGIN("Parser & LogEntry Test Suite");

    RUN_TEST(test_parse_valid_row);
    RUN_TEST(test_fixture_valid);
    RUN_TEST(test_parse_missing_fields);
    RUN_TEST(test_parse_extra_fields);
    RUN_TEST(test_parse_non_numeric_status);
    RUN_TEST(test_parse_status_too_high);
    RUN_TEST(test_parse_status_too_low);
    RUN_TEST(test_parse_negative_response_time);
    RUN_TEST(test_parse_empty_route);
    RUN_TEST(test_parse_route_no_slash);
    RUN_TEST(test_fixture_blanks);
    RUN_TEST(test_fixture_malformed);
    RUN_TEST(test_non_existent_file);
    RUN_TEST(test_fixture_empty);
    RUN_TEST(test_fixture_single);
    RUN_TEST(test_overlong_and_partial_lines);
    RUN_TEST(test_overlong_final_line_no_newline);
    RUN_TEST(test_timestamp_calendar_validation);
    RUN_TEST(test_numeric_safety_and_overflow);
    RUN_TEST(test_exact_field_and_physical_limits);
    RUN_TEST(test_api_capacities_and_stream_stop);
    RUN_TEST(test_null_pointer_safety);

    return TEST_REPORT();
}
