#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include "log_entry.h"

/*
 * ParseStats tracks detailed ingestion line accounting.
 * Invariant: total_lines == valid_records + malformed_lines + blank_lines
 */
typedef struct {
    size_t total_lines;
    size_t valid_records;
    size_t malformed_lines;
    size_t blank_lines;
} ParseStats;

/*
 * Callback invoked for each valid LogEntry parsed during streaming file read.
 * Returning false from the handler requests early termination of parsing.
 */
typedef bool (*LogRecordHandler)(const LogEntry *entry, void *user_data);

/*
 * Initialize all counters in ParseStats to zero.
 */
void parse_stats_init(ParseStats *stats);

/*
 * Print formatted ingestion summary with explicit provenance.
 */
void parse_stats_print(const ParseStats *stats, const char *filepath, Provenance prov);

/*
 * Returns true if line is NULL, empty, or consists exclusively of whitespace characters.
 */
bool parse_is_blank_line(const char *line);

/*
 * Parses a single pipe-delimited record string into out.
 * Assigns line_number to out->source_line.
 * Returns true on success, false if malformed or blank.
 */
bool parse_line(const char *line, int line_number, LogEntry *out);

/*
 * Parses an entire log file line by line.
 * Calls handler for every valid LogEntry.
 * Correctly increments ParseStats and handles overlong physical lines.
 * Returns true on successful read completion, false on file open or I/O failure.
 */
bool parse_file(const char *filepath, LogRecordHandler handler, void *user_data, ParseStats *stats);

/*
 * Parses a log file and stores up to max_entries records in the entries array.
 * Populates *out_count with the number of records actually stored.
 * Returns true on success, false on I/O error.
 */
bool parse_file_to_buffer(const char *filepath, LogEntry *entries, size_t max_entries,
                          size_t *out_count, ParseStats *stats);

/*
 * Scans a log file and gathers ParseStats without storing records.
 * Returns true on success, false on I/O error.
 */
bool parse_file_stats(const char *filepath, ParseStats *stats);

#endif /* PARSER_H */
