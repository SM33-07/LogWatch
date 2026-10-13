#include "cli.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static bool stream_append_handler(const LogEntry *entry, void *user_data) {
    LogArray *arr = (LogArray *)user_data;
    return log_array_append(arr, entry);
}

void cli_init(CliContext *ctx) {
    if (ctx == NULL) return;
    memset(ctx, 0, sizeof(CliContext));
    log_array_init(&ctx->records, 16);
    parse_stats_init(&ctx->stats);
    ctx->provenance = PROV_SYNTHETIC;
}

void cli_free(CliContext *ctx) {
    if (ctx == NULL) return;
    log_array_free(&ctx->records);
    memset(ctx, 0, sizeof(CliContext));
}

bool cli_load_file(CliContext *ctx, const char *filepath, FILE *out) {
    if (ctx == NULL || filepath == NULL || filepath[0] == '\0') {
        if (out) fprintf(out, "[ERROR] Invalid file path provided.\n");
        return false;
    }

    LogArray new_records;
    if (!log_array_init(&new_records, 16)) {
        if (out) fprintf(out, "[ERROR] Memory allocation failure while preparing array.\n");
        return false;
    }

    ParseStats new_stats;
    parse_stats_init(&new_stats);

    bool ok = parse_file(filepath, stream_append_handler, &new_records, &new_stats);
    if (!ok) {
        if (out) fprintf(out, "[ERROR] Failed to open or read file: %s\n", filepath);
        log_array_free(&new_records);
        return false;
    }

    /* Success: safely replace current dataset and stats */
    log_array_free(&ctx->records);
    ctx->records = new_records;
    ctx->stats = new_stats;
    strncpy(ctx->filepath, filepath, sizeof(ctx->filepath) - 1);
    ctx->filepath[sizeof(ctx->filepath) - 1] = '\0';
    ctx->has_file = true;
    ctx->provenance = PROV_SYNTHETIC;

    if (out) {
        fprintf(out, "\n[SUCCESS] Successfully loaded log file: %s [SYNTHETIC]\n", filepath);
        fprintf(out, "  Valid records:   %zu\n", ctx->records.size);
        fprintf(out, "  Malformed lines: %zu\n", ctx->stats.malformed_lines);
        fprintf(out, "  Blank lines:     %zu\n", ctx->stats.blank_lines);
        fprintf(out, "  Total lines:     %zu\n", ctx->stats.total_lines);
    }
    return true;
}

bool cli_reload_file(CliContext *ctx, FILE *out) {
    if (ctx == NULL || !ctx->has_file) {
        if (out) fprintf(out, "[ERROR] No file is currently loaded to reload.\n");
        return false;
    }
    if (out) fprintf(out, "[INFO] Reloading file: %s...\n", ctx->filepath);
    return cli_load_file(ctx, ctx->filepath, out);
}

void cli_display_preview(const CliContext *ctx, size_t max_count, FILE *out) {
    if (out == NULL) return;
    if (ctx == NULL || !ctx->has_file || ctx->records.size == 0) {
        fprintf(out, "\n[INFO] No records to display (empty dataset or no file loaded).\n");
        return;
    }

    size_t count = ctx->records.size < max_count ? ctx->records.size : max_count;
    fprintf(out, "\n========================================================================================\n");
    fprintf(out, "  RECORD PREVIEW (%zu of %zu records) [SYNTHETIC]\n", count, ctx->records.size);
    fprintf(out, "========================================================================================\n");
    fprintf(out, "%-5s | %-20s | %-24s | %-6s | %-8s | %-12s\n",
            "IDX", "TIMESTAMP", "ROUTE", "STATUS", "TIME(ms)", "CLIENT ID");
    fprintf(out, "----------------------------------------------------------------------------------------\n");

    for (size_t i = 0; i < count; i++) {
        const LogEntry *e = log_array_get_const(&ctx->records, i);
        if (e != NULL) {
            fprintf(out, "%-5zu | %-20s | %-24.24s | %-6d | %-8d | %-12.12s\n",
                    i, e->timestamp, e->route, e->status_code, e->response_time_ms, e->client_id);
        }
    }
    fprintf(out, "========================================================================================\n");
}

void cli_display_stats(const CliContext *ctx, FILE *out) {
    if (out == NULL) return;
    if (ctx == NULL || !ctx->has_file) {
        fprintf(out, "\n[INFO] No log file currently loaded.\n");
        return;
    }

    bool invariant = (ctx->stats.total_lines ==
                      ctx->stats.valid_records + ctx->stats.malformed_lines + ctx->stats.blank_lines);

    fprintf(out, "\n==================================================\n");
    fprintf(out, "  DATASET INGESTION STATISTICS [SYNTHETIC]\n");
    fprintf(out, "==================================================\n");
    fprintf(out, "Source file:             %s\n", ctx->filepath);
    fprintf(out, "Valid records loaded:    %zu\n", ctx->stats.valid_records);
    fprintf(out, "Malformed lines skipped: %zu\n", ctx->stats.malformed_lines);
    fprintf(out, "Blank lines skipped:     %zu\n", ctx->stats.blank_lines);
    fprintf(out, "Total physical lines:    %zu\n", ctx->stats.total_lines);
    fprintf(out, "Accounting invariant:    %s\n", invariant ? "VERIFIED (valid+malf+blank==total)" : "VIOLATED");
    fprintf(out, "Array active size:       %zu records\n", ctx->records.size);
    fprintf(out, "Array capacity:          %zu slots\n", ctx->records.capacity);
    fprintf(out, "Data Provenance:         %s\n", provenance_to_string(ctx->provenance));
    fprintf(out, "==================================================\n");
}

void cli_display_metrics(const CliContext *ctx, FILE *out) {
    if (out == NULL) return;
    fprintf(out, "\n====================================================================\n");
    fprintf(out, "  SORTING & SEARCH OPERATION METRICS [SYNTHETIC]\n");
    fprintf(out, "====================================================================\n");

    if (ctx != NULL && ctx->has_sort_metrics) {
        fprintf(out, "Last Sort Execution:\n");
        fprintf(out, "  Algorithm:           %s\n", ctx->last_sort_algo);
        fprintf(out, "  Sort Key:            %s (%s)\n",
                sort_key_to_string(ctx->last_sort_key),
                ctx->last_sort_ascending ? "Ascending" : "Descending");
        fprintf(out, "  Comparisons:         %llu\n", ctx->last_sort_metrics.comparisons);
        fprintf(out, "  Record Swaps:        %llu\n", ctx->last_sort_metrics.record_swaps);
        fprintf(out, "  Record Shifts:       %llu\n", ctx->last_sort_metrics.record_shifts);
        fprintf(out, "  Execution Time:      %.6f seconds\n", ctx->last_sort_metrics.elapsed_sec);
    } else {
        fprintf(out, "Last Sort Execution:   None performed in this session.\n");
    }

    fprintf(out, "--------------------------------------------------------------------\n");
    if (ctx != NULL && ctx->has_search_metrics) {
        fprintf(out, "Last Search Execution:\n");
        fprintf(out, "  Query Description:   %s\n", ctx->last_search_desc);
        fprintf(out, "  Comparisons:         %llu (Examined records)\n", ctx->last_search_metrics.comparisons);
        fprintf(out, "  Matches Found:       %zu\n", ctx->last_search_metrics.matches_found);
        fprintf(out, "  Execution Time:      %.6f seconds\n", ctx->last_search_metrics.elapsed_sec);
    } else {
        fprintf(out, "Last Search Execution: None performed in this session.\n");
    }

    fprintf(out, "--------------------------------------------------------------------\n");
    fprintf(out, "Algorithm Complexity Reference:\n");
    fprintf(out, "  Bubble Sort:     Best: O(n) | Avg: O(n^2) | Worst: O(n^2) | Stable | Early-exit\n");
    fprintf(out, "  Selection Sort:  Best: O(n^2) | Avg: O(n^2) | Worst: O(n^2) | Moves: O(n)\n");
    fprintf(out, "  Insertion Sort:  Best: O(n) | Avg: O(n^2) | Worst: O(n^2) | Stable | Adaptive\n");
    fprintf(out, "  Linear Search:   Best: O(1) | Avg: O(n) | Worst: O(n) | Multi-match: O(n)\n");
    fprintf(out, "====================================================================\n");
}

bool cli_execute_sort(CliContext *ctx, int algo_choice, SortKey key, bool ascending, FILE *out) {
    if (ctx == NULL || !ctx->has_file || ctx->records.size == 0) {
        if (out) fprintf(out, "[ERROR] No records loaded to sort.\n");
        return false;
    }

    SortMetrics m;
    bool ok = false;
    const char *name = "Unknown";

    switch (algo_choice) {
        case 1:
            name = "Bubble Sort";
            ok = sort_bubble(&ctx->records, key, ascending, &m);
            break;
        case 2:
            name = "Selection Sort";
            ok = sort_selection(&ctx->records, key, ascending, &m);
            break;
        case 3:
            name = "Insertion Sort";
            ok = sort_insertion(&ctx->records, key, ascending, &m);
            break;
        default:
            if (out) fprintf(out, "[ERROR] Invalid algorithm choice: %d\n", algo_choice);
            return false;
    }

    if (ok) {
        ctx->last_sort_metrics = m;
        ctx->has_sort_metrics = true;
        ctx->last_sort_key = key;
        ctx->last_sort_ascending = ascending;
        strncpy(ctx->last_sort_algo, name, sizeof(ctx->last_sort_algo) - 1);

        if (out) {
            fprintf(out, "\n==================================================\n");
            fprintf(out, "  SORT EXECUTION REPORT [SYNTHETIC]\n");
            fprintf(out, "==================================================\n");
            fprintf(out, "Algorithm:       %s\n", name);
            fprintf(out, "Key:             %s (%s)\n", sort_key_to_string(key), ascending ? "Ascending" : "Descending");
            fprintf(out, "Tie-Breaker:     Ascending source_line [STABLE]\n");
            fprintf(out, "Comparisons:     %llu\n", m.comparisons);
            fprintf(out, "Record Swaps:    %llu\n", m.record_swaps);
            fprintf(out, "Record Shifts:   %llu\n", m.record_shifts);
            fprintf(out, "Elapsed Time:    %.6f seconds\n", m.elapsed_sec);
            fprintf(out, "==================================================\n");
        }
    } else {
        if (out) fprintf(out, "[ERROR] Sorting operation failed.\n");
    }
    return ok;
}

size_t cli_execute_search(CliContext *ctx, int search_type, const void *param, FILE *out) {
    if (ctx == NULL || !ctx->has_file || ctx->records.size == 0 || param == NULL) {
        if (out) fprintf(out, "[ERROR] No records loaded or invalid search parameter.\n");
        return 0;
    }

    size_t *indices = (size_t *)malloc(ctx->records.size * sizeof(size_t));
    if (indices == NULL) {
        if (out) fprintf(out, "[ERROR] Memory allocation failure for search indices.\n");
        return 0;
    }

    SearchMetrics m;
    size_t count = 0;
    char desc[64] = "Search";

    switch (search_type) {
        case 1: { /* Exact response time */
            int target = *(const int *)param;
            snprintf(desc, sizeof(desc), "Exact response time = %d ms", target);
            count = search_linear_by_response_time(&ctx->records, target, indices, ctx->records.size, &m);
            break;
        }
        case 2: { /* Threshold */
            int thresh = *(const int *)param;
            snprintf(desc, sizeof(desc), "Response time >= %d ms", thresh);
            count = search_linear_by_threshold(&ctx->records, thresh, indices, ctx->records.size, &m);
            break;
        }
        case 3: { /* Status code */
            int status = *(const int *)param;
            snprintf(desc, sizeof(desc), "Status code = %d", status);
            count = search_linear_by_status(&ctx->records, status, indices, ctx->records.size, &m);
            break;
        }
        case 4: { /* Route substring */
            const char *sub = (const char *)param;
            snprintf(desc, sizeof(desc), "Route substring '%s'", sub);
            count = search_linear_by_route(&ctx->records, sub, indices, ctx->records.size, &m);
            break;
        }
        case 5: { /* Route prefix */
            const char *prefix = (const char *)param;
            snprintf(desc, sizeof(desc), "Route prefix '%s'", prefix);
            count = search_linear_by_route_prefix(&ctx->records, prefix, indices, ctx->records.size, &m);
            break;
        }
        default:
            if (out) fprintf(out, "[ERROR] Unknown search query type: %d\n", search_type);
            free(indices);
            return 0;
    }

    ctx->last_search_metrics = m;
    ctx->has_search_metrics = true;
    strncpy(ctx->last_search_desc, desc, sizeof(ctx->last_search_desc) - 1);

    if (out) {
        fprintf(out, "\n========================================================================================\n");
        fprintf(out, "  SEARCH RESULTS: %s [SYNTHETIC]\n", desc);
        fprintf(out, "========================================================================================\n");
        fprintf(out, "Total Matches: %zu | Records Examined: %llu | Search Time: %.6f s\n",
                count, m.comparisons, m.elapsed_sec);
        fprintf(out, "----------------------------------------------------------------------------------------\n");

        if (count == 0) {
            fprintf(out, "  (No matching records found)\n");
        } else {
            fprintf(out, "%-5s | %-5s | %-20s | %-24s | %-6s | %-8s | %-12s\n",
                    "MATCH", "INDEX", "TIMESTAMP", "ROUTE", "STATUS", "TIME(ms)", "CLIENT ID");
            fprintf(out, "----------------------------------------------------------------------------------------\n");
            for (size_t i = 0; i < count; i++) {
                size_t idx = indices[i];
                const LogEntry *e = log_array_get_const(&ctx->records, idx);
                if (e != NULL) {
                    fprintf(out, "%-5zu | %-5zu | %-20s | %-24.24s | %-6d | %-8d | %-12.12s\n",
                            i + 1, idx, e->timestamp, e->route, e->status_code, e->response_time_ms, e->client_id);
                }
            }
        }
        fprintf(out, "========================================================================================\n");
    }

    free(indices);
    return count;
}

/*
 * Reads a single line safely from stream into buffer.
 * Strips trailing newline and consumes overlong inputs to avoid line desync.
 * Returns true on success, false on EOF or error.
 */
static bool cli_read_line(FILE *in, char *buffer, size_t buffer_size) {
    if (in == NULL || buffer == NULL || buffer_size == 0) return false;
    if (fgets(buffer, (int)buffer_size, in) == NULL) {
        return false;
    }

    size_t len = strlen(buffer);
    if (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
        while (len > 0 && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r')) {
            buffer[len - 1] = '\0';
            len--;
        }
    } else if (len == buffer_size - 1) {
        /* Line was truncated by buffer; consume remaining characters until newline or EOF */
        int ch;
        while ((ch = fgetc(in)) != '\n' && ch != EOF) {
            /* discard */
        }
    }
    return true;
}

/*
 * Reads an integer within [min_val, max_val] with safe validation.
 */
static bool cli_read_int(FILE *in, int *out, int min_val, int max_val) {
    if (in == NULL || out == NULL) return false;
    char line[64];
    if (!cli_read_line(in, line, sizeof(line))) {
        return false;
    }

    /* Skip leading whitespace */
    char *start = line;
    while (*start == ' ' || *start == '\t') start++;
    if (*start == '\0') {
        return false;
    }

    char *endptr = NULL;
    errno = 0;
    long val = strtol(start, &endptr, 10);
    if (errno != 0 || endptr == start) {
        return false;
    }
    while (*endptr == ' ' || *endptr == '\t') endptr++;
    if (*endptr != '\0') {
        return false;
    }
    if (val < min_val || val > max_val) {
        return false;
    }

    *out = (int)val;
    return true;
}

static void print_main_menu(FILE *out, const CliContext *ctx) {
    if (out == NULL) return;
    fprintf(out, "\n==================================================\n");
    fprintf(out, "       LogWatch — Access Log Analyzer [SYNTHETIC]\n");
    fprintf(out, "==================================================\n");
    if (ctx != NULL && ctx->has_file) {
        fprintf(out, "Loaded: %s (%zu records)\n", ctx->filepath, ctx->records.size);
    } else {
        fprintf(out, "Loaded: (No file loaded)\n");
    }
    fprintf(out, "--------------------------------------------------\n");
    fprintf(out, " 1. Load log file\n");
    fprintf(out, " 2. Preview records\n");
    fprintf(out, " 3. Sort records\n");
    fprintf(out, " 4. Search records\n");
    fprintf(out, " 5. Display algorithm metrics\n");
    fprintf(out, " 6. Show dataset statistics\n");
    fprintf(out, " 7. Reload current file\n");
    fprintf(out, " 8. Exit\n");
    fprintf(out, "==================================================\n");
    fprintf(out, "Enter choice (1-8): ");
    fflush(out);
}

int cli_run_stream(CliContext *ctx, FILE *in, FILE *out) {
    if (ctx == NULL || in == NULL) return 1;

    while (1) {
        print_main_menu(out, ctx);
        int choice = 0;
        if (!cli_read_int(in, &choice, 1, 8)) {
            if (feof(in)) {
                if (out) fprintf(out, "\n[INFO] End of input reached. Exiting cleanly.\n");
                break;
            }
            if (out) fprintf(out, "[ERROR] Invalid selection. Please enter a number between 1 and 8.\n");
            continue;
        }

        if (choice == 8) {
            if (out) fprintf(out, "\n[INFO] Exiting LogWatch. Goodbye!\n");
            break;
        }

        switch (choice) {
            case 1: { /* Load file */
                if (out) {
                    fprintf(out, "Enter file path to load: ");
                    fflush(out);
                }
                char path[CLI_MAX_PATH];
                if (!cli_read_line(in, path, sizeof(path))) {
                    if (out) fprintf(out, "\n[INFO] Input stream closed.\n");
                    return 0;
                }
                if (path[0] == '\0') {
                    if (out) fprintf(out, "[ERROR] File path cannot be empty.\n");
                } else {
                    cli_load_file(ctx, path, out);
                }
                break;
            }

            case 2: { /* Preview */
                if (!ctx->has_file) {
                    if (out) fprintf(out, "[INFO] Please load a file first (Option 1).\n");
                    break;
                }
                if (out) {
                    fprintf(out, "Enter maximum records to display (1 - 500, default 10): ");
                    fflush(out);
                }
                int count = 10;
                char line[32];
                if (cli_read_line(in, line, sizeof(line)) && line[0] != '\0') {
                    int val = atoi(line);
                    if (val >= 1 && val <= 500) count = val;
                }
                cli_display_preview(ctx, (size_t)count, out);
                break;
            }

            case 3: { /* Sort */
                if (!ctx->has_file || ctx->records.size == 0) {
                    if (out) fprintf(out, "[INFO] Please load a non-empty log file first.\n");
                    break;
                }

                if (out) {
                    fprintf(out, "\nSelect Field to Sort:\n");
                    fprintf(out, "  1. Response Time (response_time_ms)\n");
                    fprintf(out, "  2. Status Code (status_code)\n");
                    fprintf(out, "  3. Timestamp (chronological)\n");
                    fprintf(out, "  4. Route (alphabetical)\n");
                    fprintf(out, "  5. Client ID (alphabetical)\n");
                    fprintf(out, "Choice (1-5): ");
                    fflush(out);
                }
                int field_choice = 0;
                if (!cli_read_int(in, &field_choice, 1, 5)) {
                    if (out) fprintf(out, "[ERROR] Invalid field choice.\n");
                    break;
                }
                SortKey key = SORT_KEY_RESPONSE_TIME;
                if (field_choice == 1) key = SORT_KEY_RESPONSE_TIME;
                else if (field_choice == 2) key = SORT_KEY_STATUS_CODE;
                else if (field_choice == 3) key = SORT_KEY_TIMESTAMP;
                else if (field_choice == 4) key = SORT_KEY_ROUTE;
                else if (field_choice == 5) key = SORT_KEY_CLIENT_ID;

                if (out) {
                    fprintf(out, "Select Direction:\n  1. Ascending\n  2. Descending\nChoice (1-2): ");
                    fflush(out);
                }
                int dir_choice = 0;
                if (!cli_read_int(in, &dir_choice, 1, 2)) {
                    if (out) fprintf(out, "[ERROR] Invalid direction choice.\n");
                    break;
                }
                bool ascending = (dir_choice == 1);

                if (out) {
                    fprintf(out, "Select Algorithm:\n");
                    fprintf(out, "  1. Bubble Sort (adaptive early exit)\n");
                    fprintf(out, "  2. Selection Sort (O(n) swaps)\n");
                    fprintf(out, "  3. Insertion Sort (adaptive shifts)\n");
                    fprintf(out, "Choice (1-3): ");
                    fflush(out);
                }
                int algo_choice = 0;
                if (!cli_read_int(in, &algo_choice, 1, 3)) {
                    if (out) fprintf(out, "[ERROR] Invalid algorithm choice.\n");
                    break;
                }

                cli_execute_sort(ctx, algo_choice, key, ascending, out);
                break;
            }

            case 4: { /* Search */
                if (!ctx->has_file || ctx->records.size == 0) {
                    if (out) fprintf(out, "[INFO] Please load a non-empty log file first.\n");
                    break;
                }

                if (out) {
                    fprintf(out, "\nSelect Search Query:\n");
                    fprintf(out, "  1. Exact response time (ms)\n");
                    fprintf(out, "  2. Response-time threshold (>= threshold ms)\n");
                    fprintf(out, "  3. HTTP status code\n");
                    fprintf(out, "  4. Route substring\n");
                    fprintf(out, "  5. Route prefix\n");
                    fprintf(out, "Choice (1-5): ");
                    fflush(out);
                }
                int search_type = 0;
                if (!cli_read_int(in, &search_type, 1, 5)) {
                    if (out) fprintf(out, "[ERROR] Invalid search query choice.\n");
                    break;
                }

                if (search_type == 1 || search_type == 2) {
                    if (out) {
                        fprintf(out, "Enter response time in ms (>= 0): ");
                        fflush(out);
                    }
                    int ms = 0;
                    if (!cli_read_int(in, &ms, 0, 10000000)) {
                        if (out) fprintf(out, "[ERROR] Invalid response time value.\n");
                        break;
                    }
                    cli_execute_search(ctx, search_type, &ms, out);
                } else if (search_type == 3) {
                    if (out) {
                        fprintf(out, "Enter HTTP status code (100 - 599): ");
                        fflush(out);
                    }
                    int status = 0;
                    if (!cli_read_int(in, &status, 100, 599)) {
                        if (out) fprintf(out, "[ERROR] Invalid status code (must be 100 - 599).\n");
                        break;
                    }
                    cli_execute_search(ctx, search_type, &status, out);
                } else {
                    if (out) {
                        fprintf(out, "Enter route text to query: ");
                        fflush(out);
                    }
                    char query[LW_MAX_ROUTE];
                    if (!cli_read_line(in, query, sizeof(query)) || query[0] == '\0') {
                        if (out) fprintf(out, "[ERROR] Route query cannot be empty.\n");
                        break;
                    }
                    cli_execute_search(ctx, search_type, query, out);
                }
                break;
            }

            case 5: /* Metrics */
                cli_display_metrics(ctx, out);
                break;

            case 6: /* Stats */
                cli_display_stats(ctx, out);
                break;

            case 7: /* Reload */
                cli_reload_file(ctx, out);
                break;

            default:
                break;
        }
    }

    return 0;
}

int cli_run(CliContext *ctx) {
    return cli_run_stream(ctx, stdin, stdout);
}
