#include "log_entry.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static bool is_digit_char(char c) {
    return c >= '0' && c <= '9';
}

static bool validate_iso8601_utc(const char *ts) {
    if (ts == NULL) {
        return false;
    }
    if (strlen(ts) != 20) {
        return false;
    }

    /* Format: YYYY-MM-DDTHH:MM:SSZ */
    for (int i = 0; i < 4; i++) {
        if (!is_digit_char(ts[i])) return false;
    }
    if (ts[4] != '-') return false;
    if (!is_digit_char(ts[5]) || !is_digit_char(ts[6])) return false;
    if (ts[7] != '-') return false;
    if (!is_digit_char(ts[8]) || !is_digit_char(ts[9])) return false;
    if (ts[10] != 'T') return false;
    if (!is_digit_char(ts[11]) || !is_digit_char(ts[12])) return false;
    if (ts[13] != ':') return false;
    if (!is_digit_char(ts[14]) || !is_digit_char(ts[15])) return false;
    if (ts[16] != ':') return false;
    if (!is_digit_char(ts[17]) || !is_digit_char(ts[18])) return false;
    if (ts[19] != 'Z') return false;

    /* Basic numerical boundary checks */
    int year = (ts[0] - '0') * 1000 + (ts[1] - '0') * 100 + (ts[2] - '0') * 10 + (ts[3] - '0');
    int month = (ts[5] - '0') * 10 + (ts[6] - '0');
    if (month < 1 || month > 12) return false;

    int day = (ts[8] - '0') * 10 + (ts[9] - '0');
    if (day < 1) return false;

    /* Calendar days-in-month validation with leap year handling */
    int max_days = 31;
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        max_days = 30;
    } else if (month == 2) {
        bool is_leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        max_days = is_leap ? 29 : 28;
    }
    if (day > max_days) return false;

    int hour = (ts[11] - '0') * 10 + (ts[12] - '0');
    if (hour < 0 || hour > 23) return false;

    int min = (ts[14] - '0') * 10 + (ts[15] - '0');
    if (min < 0 || min > 59) return false;

    int sec = (ts[17] - '0') * 10 + (ts[18] - '0');
    if (sec < 0 || sec > 59) return false;

    return true;
}

bool log_entry_validate(const LogEntry *entry) {
    if (entry == NULL) {
        return false;
    }

    /* Timestamp check */
    if (!validate_iso8601_utc(entry->timestamp)) {
        return false;
    }

    /* Route check: must start with '/', non-empty, fit in bounds, no pipe */
    size_t route_len = strlen(entry->route);
    if (route_len == 0 || route_len >= LW_MAX_ROUTE) {
        return false;
    }
    if (entry->route[0] != '/') {
        return false;
    }
    if (strchr(entry->route, '|') != NULL) {
        return false;
    }

    /* HTTP status code: 100 - 599 */
    if (entry->status_code < 100 || entry->status_code > 599) {
        return false;
    }

    /* Response time: non-negative */
    if (entry->response_time_ms < 0) {
        return false;
    }

    /* Client ID: non-empty, fit in bounds, no pipe */
    size_t client_len = strlen(entry->client_id);
    if (client_len == 0 || client_len >= LW_MAX_CLIENT_ID) {
        return false;
    }
    if (strchr(entry->client_id, '|') != NULL) {
        return false;
    }

    return true;
}

void log_entry_print_header(void) {
    printf("%-6s | %-20s | %-32s | %-6s | %-8s | %-16s\n",
           "LINE", "TIMESTAMP (UTC)", "ROUTE", "STATUS", "TIME(ms)", "CLIENT ID");
    printf("-------+----------------------+----------------------------------+--------+----------+-----------------\n");
}

void log_entry_print(const LogEntry *entry) {
    if (entry == NULL) {
        printf("(null record)\n");
        return;
    }
    printf("%-6d | %-20s | %-32s | %-6d | %-8d | %-16s\n",
           entry->source_line,
           entry->timestamp,
           entry->route,
           entry->status_code,
           entry->response_time_ms,
           entry->client_id);
}

const char *provenance_to_string(Provenance prov) {
    switch (prov) {
        case PROV_REAL:
            return "[REAL]";
        case PROV_SYNTHETIC:
        default:
            return "[SYNTHETIC]";
    }
}
