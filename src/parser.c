#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

void parse_stats_init(ParseStats *stats) {
    if (stats == NULL) {
        return;
    }
    stats->total_lines = 0;
    stats->valid_records = 0;
    stats->malformed_lines = 0;
    stats->blank_lines = 0;
}

void parse_stats_print(const ParseStats *stats, const char *filepath, Provenance prov) {
    printf("\n==================================================\n");
    printf("  LOGWATCH INGESTION SUMMARY %s\n", provenance_to_string(prov));
    printf("==================================================\n");
    if (filepath != NULL) {
        printf("Source File:      %s\n", filepath);
    }
    if (stats != NULL) {
        printf("Total Lines:      %zu\n", stats->total_lines);
        printf("Valid Records:    %zu\n", stats->valid_records);
        printf("Malformed Lines:  %zu\n", stats->malformed_lines);
        printf("Blank Lines:      %zu\n", stats->blank_lines);
    }
    printf("==================================================\n\n");
}

bool parse_is_blank_line(const char *line) {
    if (line == NULL) {
        return true;
    }
    while (*line != '\0') {
        if (!isspace((unsigned char)*line)) {
            return false;
        }
        line++;
    }
    return true;
}

bool parse_line(const char *line, int line_number, LogEntry *out) {
    if (line == NULL || out == NULL) {
        return false;
    }

    if (parse_is_blank_line(line)) {
        return false;
    }

    /* Delimiter verification: exactly 4 pipes required for 5 fields */
    int pipe_count = 0;
    for (const char *p = line; *p != '\0'; p++) {
        if (*p == '|') {
            pipe_count++;
        }
    }
    if (pipe_count != 4) {
        return false;
    }

    /* Locate the 4 pipe delimiters */
    const char *p1 = strchr(line, '|');
    if (p1 == NULL) return false;

    const char *p2 = strchr(p1 + 1, '|');
    if (p2 == NULL) return false;

    const char *p3 = strchr(p2 + 1, '|');
    if (p3 == NULL) return false;

    const char *p4 = strchr(p3 + 1, '|');
    if (p4 == NULL) return false;

    /* Field lengths */
    size_t len_ts     = (size_t)(p1 - line);
    size_t len_route  = (size_t)(p2 - (p1 + 1));
    size_t len_status = (size_t)(p3 - (p2 + 1));
    size_t len_rt     = (size_t)(p4 - (p3 + 1));
    size_t len_client = strlen(p4 + 1);

    /* Strip trailing newline/carriage return from client field */
    while (len_client > 0 && 
           (p4[len_client] == '\r' || p4[len_client] == '\n')) {
        len_client--;
    }

    /* Check length boundaries before copying to prevent buffer overflow or truncation */
    if (len_ts == 0 || len_ts >= LW_MAX_TIMESTAMP) return false;
    if (len_route == 0 || len_route >= LW_MAX_ROUTE) return false;
    if (len_status == 0 || len_status > 10) return false;
    if (len_rt == 0 || len_rt > 10) return false;
    if (len_client == 0 || len_client >= LW_MAX_CLIENT_ID) return false;

    /* Field 1: Timestamp */
    memcpy(out->timestamp, line, len_ts);
    out->timestamp[len_ts] = '\0';

    /* Field 2: Route */
    memcpy(out->route, p1 + 1, len_route);
    out->route[len_route] = '\0';

    /* Field 3: Status code (must be all digits, 100-599) */
    long status = 0;
    for (const char *c = p2 + 1; c < p3; c++) {
        if (!isdigit((unsigned char)*c)) {
            return false;
        }
        status = status * 10 + (*c - '0');
        if (status > 599) {
            return false;
        }
    }
    if (status < 100 || status > 599) {
        return false;
    }
    out->status_code = (int)status;

    /* Field 4: Response time in ms (must be all digits, >= 0, <= INT_MAX) */
    unsigned long long rt = 0;
    for (const char *c = p3 + 1; c < p4; c++) {
        if (!isdigit((unsigned char)*c)) {
            return false;
        }
        rt = rt * 10 + (unsigned long long)(*c - '0');
        if (rt > (unsigned long long)INT_MAX) {
            return false;
        }
    }
    out->response_time_ms = (int)rt;

    /* Field 5: Client ID */
    memcpy(out->client_id, p4 + 1, len_client);
    out->client_id[len_client] = '\0';

    out->source_line = line_number;

    /* Final validation of all fields and format constraints */
    return log_entry_validate(out);
}

bool parse_file(const char *filepath, LogRecordHandler handler, void *user_data, ParseStats *stats) {
    if (filepath == NULL || stats == NULL) {
        return false;
    }

    parse_stats_init(stats);

    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        fprintf(stderr, "[ERROR] Unable to open log file: %s\n", filepath);
        return false;
    }

    char line_buf[LW_MAX_LINE + 2];
    int current_line = 0;

    while (fgets(line_buf, sizeof(line_buf), fp) != NULL) {
        size_t len = strlen(line_buf);
        char *nl = strchr(line_buf, '\n');

        if (nl != NULL) {
            /* Standard line ending within buffer limits */
            current_line++;
            stats->total_lines++;

            *nl = '\0';
            if (nl > line_buf && *(nl - 1) == '\r') {
                *(nl - 1) = '\0';
            }

            if (parse_is_blank_line(line_buf)) {
                stats->blank_lines++;
            } else {
                LogEntry entry;
                if (parse_line(line_buf, current_line, &entry)) {
                    stats->valid_records++;
                    if (handler != NULL) {
                        if (!handler(&entry, user_data)) {
                            break;
                        }
                    }
                } else {
                    stats->malformed_lines++;
                }
            }
        } else {
            /* Buffer has no newline: either final line without newline or overlong line */
            if (feof(fp)) {
                /* Final line without newline */
                current_line++;
                stats->total_lines++;

                if (len > 0 && line_buf[len - 1] == '\r') {
                    line_buf[--len] = '\0';
                }

                if (parse_is_blank_line(line_buf)) {
                    stats->blank_lines++;
                } else {
                    LogEntry entry;
                    if (parse_line(line_buf, current_line, &entry)) {
                        stats->valid_records++;
                        if (handler != NULL) {
                            handler(&entry, user_data);
                        }
                    } else {
                        stats->malformed_lines++;
                    }
                }
            } else {
                /* Overlong physical line: consume remainder and reject once */
                int ch;
                while ((ch = fgetc(fp)) != '\n' && ch != EOF) {
                    /* Consume remainder of line */
                }

                current_line++;
                stats->total_lines++;
                stats->malformed_lines++;
            }
        }
    }

    fclose(fp);
    return true;
}

typedef struct {
    LogEntry *entries;
    size_t    max_entries;
    size_t    count;
} BufferCollectContext;

static bool buffer_collect_handler(const LogEntry *entry, void *user_data) {
    BufferCollectContext *ctx = (BufferCollectContext *)user_data;
    if (ctx->count < ctx->max_entries) {
        ctx->entries[ctx->count] = *entry;
        ctx->count++;
    }
    return true;
}

bool parse_file_to_buffer(const char *filepath, LogEntry *entries, size_t max_entries,
                          size_t *out_count, ParseStats *stats) {
    if (entries == NULL || out_count == NULL) {
        return false;
    }
    *out_count = 0;
    BufferCollectContext ctx = { entries, max_entries, 0 };
    bool ok = parse_file(filepath, buffer_collect_handler, &ctx, stats);
    if (ok) {
        *out_count = ctx.count;
    }
    return ok;
}

bool parse_file_stats(const char *filepath, ParseStats *stats) {
    return parse_file(filepath, NULL, NULL, stats);
}
