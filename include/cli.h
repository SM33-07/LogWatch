#ifndef CLI_H
#define CLI_H

#include "log_array.h"
#include "parser.h"
#include "sort.h"
#include "search.h"
#include <stdio.h>
#include <stdbool.h>

/*
 * Maximum path length for file loading
 */
#define CLI_MAX_PATH 512

/*
 * CliContext maintains the runtime state of the LogWatch CLI session.
 */
typedef struct {
    LogArray      records;              /* Currently loaded records */
    ParseStats    stats;                /* Parse statistics for loaded file */
    char          filepath[CLI_MAX_PATH]; /* Path to current loaded file */
    bool          has_file;             /* true if a file is currently loaded */
    Provenance    provenance;           /* Provenance indicator (PROV_SYNTHETIC) */

    /* Metrics from the most recent sort operation */
    SortMetrics   last_sort_metrics;
    bool          has_sort_metrics;
    SortKey       last_sort_key;
    bool          last_sort_ascending;
    char          last_sort_algo[32];

    /* Metrics from the most recent search operation */
    SearchMetrics last_search_metrics;
    bool          has_search_metrics;
    char          last_search_desc[64];
} CliContext;

/*
 * Initializes an empty CLI context.
 */
void cli_init(CliContext *ctx);

/*
 * Releases all resources owned by the CLI context and resets state.
 */
void cli_free(CliContext *ctx);

/*
 * Loads a log file into the context.
 * If a file is already loaded and loading the new file fails,
 * the existing dataset is preserved intact.
 * Returns true on success, false on failure.
 */
bool cli_load_file(CliContext *ctx, const char *filepath, FILE *out);

/*
 * Reloads the currently loaded log file.
 * Returns true on success, false if no file loaded or read failed.
 */
bool cli_reload_file(CliContext *ctx, FILE *out);

/*
 * Displays a bounded preview of the loaded records.
 */
void cli_display_preview(const CliContext *ctx, size_t max_count, FILE *out);

/*
 * Displays ingestion and dataset statistics with accounting invariant verification.
 */
void cli_display_stats(const CliContext *ctx, FILE *out);

/*
 * Displays sorting and search metrics alongside theoretical complexity.
 */
void cli_display_metrics(const CliContext *ctx, FILE *out);

/*
 * Executes sorting on the loaded records.
 */
bool cli_execute_sort(CliContext *ctx, int algo_choice, SortKey key, bool ascending, FILE *out);

/*
 * Executes search on the loaded records and displays matching records.
 */
size_t cli_execute_search(CliContext *ctx, int search_type, const void *param, FILE *out);

/*
 * Runs the interactive CLI menu loop over stream inputs.
 * Accepts input from 'in' (typically stdin) and writes output to 'out' (typically stdout).
 * Handles EOF and invalid input cleanly. Returns 0 on clean exit.
 */
int cli_run_stream(CliContext *ctx, FILE *in, FILE *out);

/*
 * Standard interactive entry point reading stdin and writing stdout.
 */
int cli_run(CliContext *ctx);

#endif /* CLI_H */
