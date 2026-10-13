#include "test_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define popen_cmd _popen
#define pclose_cmd _pclose
#else
#define popen_cmd popen
#define pclose_cmd pclose
#endif

/*
 * Helper to run bin/logwatch.exe as an external process, feed input,
 * and capture the process exit code.
 */
static int run_logwatch_process(const char *args, const char *input_script) {
    char cmd[512];
    if (args != NULL && args[0] != '\0') {
        snprintf(cmd, sizeof(cmd), "bin\\logwatch.exe %s", args);
    } else {
        snprintf(cmd, sizeof(cmd), "bin\\logwatch.exe");
    }

    FILE *pipe = popen_cmd(cmd, "w");
    if (!pipe) {
        return -1;
    }

    if (input_script != NULL && input_script[0] != '\0') {
        fputs(input_script, pipe);
        fflush(pipe);
    }

    return pclose_cmd(pipe);
}

/*
 * 1. Process startup and clean exit
 */
static void test_exec_startup_and_exit(void) {
    int code = run_logwatch_process(NULL, "8\n");
    ASSERT_EQ(code, 0, "logwatch.exe starts and exits cleanly with code 0 on choice 8");
}

/*
 * 2. Process startup with file argument
 */
static void test_exec_file_argument_load(void) {
    int code = run_logwatch_process("tests\\fixtures\\synthetic_valid.log", "8\n");
    ASSERT_EQ(code, 0, "logwatch.exe loads initial file argument and exits with code 0");
}

/*
 * 3. Complete interactive workflow via process stdin
 */
static void test_exec_interactive_workflow(void) {
    const char *script =
        "1\n"
        "tests\\fixtures\\synthetic_valid.log\n"
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

    int code = run_logwatch_process(NULL, script);
    ASSERT_EQ(code, 0, "Interactive workflow through logwatch.exe completes with code 0");
}

/*
 * 4. Error recovery and invalid inputs via process stdin
 */
static void test_exec_error_recovery(void) {
    const char *script =
        "99\n"
        "-1\n"
        "invalid_text\n"
        "\n"
        "1\n"
        "\n"
        "1\n"
        "missing_file.log\n"
        "1\n"
        "tests\\fixtures\\synthetic_valid.log\n"
        "7\n"
        "8\n";

    int code = run_logwatch_process(NULL, script);
    ASSERT_EQ(code, 0, "Process handles invalid choices and missing files without crashing (code 0)");
}

/*
 * 5. Immediate EOF on process stdin
 */
static void test_exec_immediate_eof(void) {
    int code = run_logwatch_process(NULL, "");
    ASSERT_EQ(code, 0, "Process exits cleanly with code 0 on immediate EOF");
}

/*
 * 6. EOF during sub-prompts
 */
static void test_exec_eof_during_prompt(void) {
    int code = run_logwatch_process(NULL, "1\n");
    ASSERT_EQ(code, 0, "Process exits cleanly with code 0 on EOF during sub-prompt");
}

/*
 * 7. Verification across all fixture files as process arguments
 */
static void test_exec_all_fixtures_as_args(void) {
    const char *fixtures[] = {
        "tests\\fixtures\\synthetic_empty.log",
        "tests\\fixtures\\synthetic_single.log",
        "tests\\fixtures\\synthetic_malformed.log",
        "tests\\fixtures\\synthetic_duplicates.log",
        "tests\\fixtures\\synthetic_blanks.log",
        "samples\\synthetic_access.log"
    };

    for (size_t i = 0; i < sizeof(fixtures) / sizeof(fixtures[0]); i++) {
        int code = run_logwatch_process(fixtures[i], "6\n8\n");
        ASSERT_EQ(code, 0, "Process runs stats and exits cleanly for fixture argument");
    }
}

int main(void) {
    TEST_BEGIN("LogWatch Executable-Level Acceptance Tests");

    RUN_TEST(test_exec_startup_and_exit);
    RUN_TEST(test_exec_file_argument_load);
    RUN_TEST(test_exec_interactive_workflow);
    RUN_TEST(test_exec_error_recovery);
    RUN_TEST(test_exec_immediate_eof);
    RUN_TEST(test_exec_eof_during_prompt);
    RUN_TEST(test_exec_all_fixtures_as_args);

    return TEST_REPORT();
}
