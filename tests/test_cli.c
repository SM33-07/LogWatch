#include "test_utils.h"
#include "cli.h"

/*
 * Helper to create a temporary file with text content
 */
static void write_temp_script(const char *filename, const char *content) {
    FILE *f = fopen(filename, "w");
    if (f != NULL) {
        fputs(content, f);
        fclose(f);
    }
}

/*
 * 1. CLI Lifecycle and Empty-State Invariants
 */
static void test_cli_lifecycle_and_empty_state(void) {
    CliContext ctx;
    cli_init(&ctx);

    ASSERT(!ctx.has_file, "New context has no file");
    ASSERT_EQ(ctx.records.size, 0, "Initial record count is 0");
    ASSERT_EQ(ctx.records.capacity, 16, "Initial capacity is 16");
    ASSERT_EQ(ctx.stats.total_lines, 0, "Initial stats lines is 0");
    ASSERT(!ctx.has_sort_metrics, "No initial sort metrics");
    ASSERT(!ctx.has_search_metrics, "No initial search metrics");

    /* Empty state operations should safely report no-ops or false without crash */
    cli_display_preview(&ctx, 10, NULL);
    cli_display_stats(&ctx, NULL);
    cli_display_metrics(&ctx, NULL);

    ASSERT(!cli_reload_file(&ctx, NULL), "Reload on empty context returns false");
    ASSERT(!cli_execute_sort(&ctx, 1, SORT_KEY_RESPONSE_TIME, true, NULL), "Sort on empty context returns false");

    int target = 42;
    ASSERT_EQ(cli_execute_search(&ctx, 1, &target, NULL), 0, "Search on empty context returns 0");

    cli_free(&ctx);
    ASSERT_EQ(ctx.records.size, 0, "Records size is 0 after free");
}

/*
 * 2. File Loading, Provenance, and Preservation on Failed Replacement
 */
static void test_cli_loading_and_preservation(void) {
    CliContext ctx;
    cli_init(&ctx);

    /* Invalid path */
    ASSERT(!cli_load_file(&ctx, NULL, NULL), "Load rejects NULL path");
    ASSERT(!cli_load_file(&ctx, "", NULL), "Load rejects empty path");
    ASSERT(!cli_load_file(&ctx, "non_existent_file.log", NULL), "Load rejects non-existent file");
    ASSERT(!ctx.has_file, "has_file remains false after failed load");

    /* Load valid fixture */
    ASSERT(cli_load_file(&ctx, "tests/fixtures/synthetic_valid.log", NULL), "Load synthetic_valid.log");
    ASSERT(ctx.has_file, "has_file is true after valid load");
    ASSERT_EQ(ctx.records.size, 5, "Loaded 5 records");
    ASSERT_EQ(ctx.stats.valid_records, 5, "5 valid records in stats");
    ASSERT_EQ(ctx.stats.total_lines, 5, "5 total lines in stats");
    ASSERT_EQ(ctx.provenance, PROV_SYNTHETIC, "Provenance is strictly PROV_SYNTHETIC");

    /* Reload successful */
    ASSERT(cli_reload_file(&ctx, NULL), "Reload succeeds");
    ASSERT_EQ(ctx.records.size, 5, "Still 5 records after reload");

    /* Failed reload / replacement: MUST preserve existing dataset! */
    ASSERT(!cli_load_file(&ctx, "non_existent_subsequent_file.log", NULL), "Replacement file load fails");
    ASSERT(ctx.has_file, "has_file remains true after failed replacement");
    ASSERT_EQ(ctx.records.size, 5, "Existing 5 records preserved intact after failed replacement");
    ASSERT_STR_EQ(ctx.filepath, "tests/fixtures/synthetic_valid.log", "Original filepath preserved");

    /* Empty file */
    ASSERT(cli_load_file(&ctx, "tests/fixtures/synthetic_empty.log", NULL), "Load synthetic_empty.log");
    ASSERT_EQ(ctx.records.size, 0, "Empty file has 0 records");
    ASSERT_EQ(ctx.stats.total_lines, 0, "0 total lines in empty file");

    /* Single record */
    ASSERT(cli_load_file(&ctx, "tests/fixtures/synthetic_single.log", NULL), "Load synthetic_single.log");
    ASSERT_EQ(ctx.records.size, 1, "Single file has 1 record");
    ASSERT_EQ(ctx.stats.valid_records, 1, "1 valid record in stats");

    /* Malformed mixed fixture with accounting invariant */
    ASSERT(cli_load_file(&ctx, "tests/fixtures/synthetic_malformed.log", NULL), "Load synthetic_malformed.log");
    ASSERT_EQ(ctx.records.size, 3, "3 valid records loaded from malformed fixture");
    ASSERT_EQ(ctx.stats.malformed_lines, 6, "6 malformed lines counted");
    ASSERT_EQ(ctx.stats.blank_lines, 0, "0 blank lines");
    ASSERT_EQ(ctx.stats.total_lines, 9, "9 total lines");
    ASSERT_EQ(ctx.stats.total_lines,
              ctx.stats.valid_records + ctx.stats.malformed_lines + ctx.stats.blank_lines,
              "Accounting invariant strictly holds: total == valid + malformed + blank");

    cli_free(&ctx);
}

/*
 * 3. Sorting Execution Across All Algorithms & Keys
 */
static void test_cli_sorting_integration(void) {
    CliContext ctx;
    cli_init(&ctx);
    ASSERT(cli_load_file(&ctx, "tests/fixtures/synthetic_valid.log", NULL), "Load valid log");

    /* Bubble sort (algo 1) by response time ascending */
    ASSERT(cli_execute_sort(&ctx, 1, SORT_KEY_RESPONSE_TIME, true, NULL), "Bubble sort response time asc");
    ASSERT(ctx.has_sort_metrics, "Sort metrics updated");
    ASSERT_STR_EQ(ctx.last_sort_algo, "Bubble Sort", "Algo name is Bubble Sort");
    ASSERT_EQ(ctx.records.entries[0].response_time_ms, 18, "Smallest element is 18ms at index 0");
    ASSERT_EQ(ctx.records.entries[4].response_time_ms, 1200, "Largest element is 1200ms at index 4");

    /* Selection sort (algo 2) by status code descending */
    ASSERT(cli_execute_sort(&ctx, 2, SORT_KEY_STATUS_CODE, false, NULL), "Selection sort status desc");
    ASSERT_STR_EQ(ctx.last_sort_algo, "Selection Sort", "Algo name is Selection Sort");
    ASSERT_EQ(ctx.records.entries[0].status_code, 503, "Largest status 503 at index 0");
    ASSERT_EQ(ctx.records.entries[4].status_code, 200, "Smallest status 200 at index 4");

    /* Insertion sort (algo 3) by timestamp ascending */
    ASSERT(cli_execute_sort(&ctx, 3, SORT_KEY_TIMESTAMP, true, NULL), "Insertion sort timestamp asc");
    ASSERT_STR_EQ(ctx.last_sort_algo, "Insertion Sort", "Algo name is Insertion Sort");
    ASSERT_STR_EQ(ctx.records.entries[0].timestamp, "2026-10-09T10:00:00Z", "First timestamp chronologically");

    /* Invalid algorithm */
    ASSERT(!cli_execute_sort(&ctx, 99, SORT_KEY_RESPONSE_TIME, true, NULL), "Invalid algorithm choice returns false");

    cli_free(&ctx);
}

/*
 * 4. Searching Execution Across All Supported Query Types
 */
static void test_cli_search_integration(void) {
    CliContext ctx;
    cli_init(&ctx);
    ASSERT(cli_load_file(&ctx, "tests/fixtures/synthetic_valid.log", NULL), "Load valid log");

    /* 1. Exact response time (18ms) */
    int target_18 = 18;
    size_t count = cli_execute_search(&ctx, 1, &target_18, NULL);
    ASSERT_EQ(count, 1, "Exact response time 18ms found 1 match");
    ASSERT(ctx.has_search_metrics, "Search metrics updated");

    /* 2. Threshold search (>= 100ms) */
    int thresh_100 = 100;
    count = cli_execute_search(&ctx, 2, &thresh_100, NULL);
    ASSERT_EQ(count, 2, "Threshold >= 100ms found 2 matches (310 and 1200)");

    /* 3. Status code (200) */
    int status_200 = 200;
    count = cli_execute_search(&ctx, 3, &status_200, NULL);
    ASSERT_EQ(count, 2, "Status code 200 found 2 matches");

    /* 4. Route substring ("/snippets") */
    count = cli_execute_search(&ctx, 4, "/snippets", NULL);
    ASSERT_EQ(count, 3, "Route substring /snippets found 3 matches");

    /* 5. Route prefix ("/api") */
    count = cli_execute_search(&ctx, 5, "/api", NULL);
    ASSERT_EQ(count, 5, "Route prefix /api found 5 matches");

    /* Absent value */
    int target_absent = 99999;
    count = cli_execute_search(&ctx, 1, &target_absent, NULL);
    ASSERT_EQ(count, 0, "Absent target found 0 matches");

    /* Invalid type */
    count = cli_execute_search(&ctx, 99, &target_18, NULL);
    ASSERT_EQ(count, 0, "Invalid query type returns 0");

    cli_free(&ctx);
}

/*
 * 5. Full Scripted CLI Stream Execution: Comprehensive Happy Path
 */
static void test_cli_scripted_happy_path(void) {
    const char *script_file = "bin/temp_script_happy.txt";
    /*
     * Script steps:
     * 1 (load file) -> tests/fixtures/synthetic_valid.log
     * 2 (preview) -> 5
     * 3 (sort) -> 1 (response time) -> 1 (ascending) -> 1 (bubble sort)
     * 4 (search) -> 1 (exact time) -> 18
     * 5 (metrics)
     * 6 (stats)
     * 7 (reload)
     * 8 (exit)
     */
    const char *input_text =
        "1\n"
        "tests/fixtures/synthetic_valid.log\n"
        "2\n"
        "5\n"
        "3\n"
        "1\n"
        "1\n"
        "1\n"
        "4\n"
        "1\n"
        "18\n"
        "5\n"
        "6\n"
        "7\n"
        "8\n";

    write_temp_script(script_file, input_text);

    FILE *in = fopen(script_file, "r");
    ASSERT(in != NULL, "Open script file for reading");

    CliContext ctx;
    cli_init(&ctx);

    int res = cli_run_stream(&ctx, in, NULL);
    fclose(in);
    remove(script_file);

    ASSERT_EQ(res, 0, "CLI stream execution exited with code 0");
    ASSERT(ctx.has_file, "Context retained loaded file");
    ASSERT_EQ(ctx.records.size, 5, "5 records remain loaded");
    ASSERT(ctx.has_sort_metrics, "Sort metrics were recorded");
    ASSERT(ctx.has_search_metrics, "Search metrics were recorded");

    cli_free(&ctx);
}

/*
 * 6. Scripted Error Recovery: Invalid Input, Negative Values, Excessive Chars
 */
static void test_cli_scripted_error_recovery(void) {
    const char *script_file = "bin/temp_script_errors.txt";
    /*
     * Script steps with errors:
     * 99 (invalid choice)
     * -1 (negative choice)
     * xyz (non-numeric)
     * <blank line>
     * 1 (load) -> <empty path> -> non_existent.log -> tests/fixtures/synthetic_valid.log
     * 3 (sort) -> 99 (invalid field) -> 1 (valid field) -> -1 (invalid dir) -> 1 (asc) -> 5 (invalid algo) -> 1 (bubble)
     * 4 (search) -> 99 (invalid type) -> 3 (status) -> -5 (invalid status) -> 700 (invalid status) -> 200 (valid status)
     * 8 (exit)
     */
    const char *input_text =
        "99\n"
        "-1\n"
        "xyz\n"
        "\n"
        "1\n"
        "\n"
        "1\n"
        "non_existent.log\n"
        "1\n"
        "tests/fixtures/synthetic_valid.log\n"
        "3\n"
        "99\n"
        "1\n"
        "-1\n"
        "1\n"
        "5\n"
        "1\n"
        "4\n"
        "99\n"
        "3\n"
        "-5\n"
        "700\n"
        "200\n"
        "8\n";

    write_temp_script(script_file, input_text);

    FILE *in = fopen(script_file, "r");
    ASSERT(in != NULL, "Open error script file for reading");

    CliContext ctx;
    cli_init(&ctx);

    int res = cli_run_stream(&ctx, in, NULL);
    fclose(in);
    remove(script_file);

    ASSERT_EQ(res, 0, "CLI recovered cleanly from all invalid inputs and exited with code 0");
    ASSERT_EQ(ctx.records.size, 5, "Dataset intact after recovery");

    cli_free(&ctx);
}

/*
 * 7. Scripted EOF Handling: Clean exit when stream ends prematurely
 */
static void test_cli_scripted_eof(void) {
    const char *script_file = "bin/temp_script_eof.txt";
    /* Immediate EOF without choice 8 */
    write_temp_script(script_file, "");

    FILE *in = fopen(script_file, "r");
    ASSERT(in != NULL, "Open empty EOF script file");

    CliContext ctx;
    cli_init(&ctx);

    int res = cli_run_stream(&ctx, in, NULL);
    fclose(in);
    remove(script_file);

    ASSERT_EQ(res, 0, "CLI exited cleanly with code 0 upon immediate EOF");
    cli_free(&ctx);

    /* EOF during prompt (e.g. choice 1 entered, then stream closes before path) */
    write_temp_script(script_file, "1\n");
    in = fopen(script_file, "r");
    ASSERT(in != NULL, "Open partial script file");

    cli_init(&ctx);
    res = cli_run_stream(&ctx, in, NULL);
    fclose(in);
    remove(script_file);

    ASSERT_EQ(res, 0, "CLI exited cleanly with code 0 upon EOF during sub-prompt");
    cli_free(&ctx);
}

int main(void) {
    TEST_BEGIN("LogWatch Interactive CLI Test Suite");

    RUN_TEST(test_cli_lifecycle_and_empty_state);
    RUN_TEST(test_cli_loading_and_preservation);
    RUN_TEST(test_cli_sorting_integration);
    RUN_TEST(test_cli_search_integration);
    RUN_TEST(test_cli_scripted_happy_path);
    RUN_TEST(test_cli_scripted_error_recovery);
    RUN_TEST(test_cli_scripted_eof);

    return TEST_REPORT();
}
