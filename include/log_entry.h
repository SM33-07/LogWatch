#ifndef LOG_ENTRY_H
#define LOG_ENTRY_H

#include <stddef.h>
#include <stdbool.h>

#define LW_MAX_TIMESTAMP    32
#define LW_MAX_ROUTE       256
#define LW_MAX_CLIENT_ID    64
#define LW_MAX_LINE       1024

/*
 * Data provenance tagging
 * Default is PROV_SYNTHETIC. Synthetic data must never be claimed
 * as verified production traffic.
 */
typedef enum {
    PROV_SYNTHETIC = 0,
    PROV_REAL      = 1
} Provenance;

/*
 * LogEntry represents a single HTTP request log record.
 * Complete struct size is ~360 bytes.
 * Relational integrity invariant: All fields remain permanently
 * paired with this record during sorting and search operations.
 */
typedef struct {
    char timestamp[LW_MAX_TIMESTAMP];
    char route[LW_MAX_ROUTE];
    int  status_code;       /* Valid HTTP status: 100 - 599 */
    int  response_time_ms;  /* Duration in milliseconds: >= 0 */
    char client_id[LW_MAX_CLIENT_ID];
    int  source_line;       /* 1-based line number in source file */
} LogEntry;

/*
 * Prints a single LogEntry as a formatted tabular row.
 */
void log_entry_print(const LogEntry *entry);

/*
 * Prints the column header for tabular LogEntry output.
 */
void log_entry_print_header(void);

/*
 * Validates that all fields in entry satisfy format, boundaries, and limits.
 * Returns true if valid, false otherwise.
 */
bool log_entry_validate(const LogEntry *entry);

/*
 * Returns display badge string for provenance (e.g., "[SYNTHETIC]").
 */
const char *provenance_to_string(Provenance prov);

#endif /* LOG_ENTRY_H */
