#include "sort.h"
#include <string.h>
#include <time.h>

/*
 * Portable high-resolution timer.
 * Uses C11 timespec_get(ts, TIME_UTC) when available; falls back to clock().
 */
static double get_time_sec(void) {
#if defined(TIME_UTC)
    struct timespec ts;
    if (timespec_get(&ts, TIME_UTC)) {
        return (double)ts.tv_sec + ((double)ts.tv_nsec / 1e9);
    }
#endif
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

void sort_metrics_reset(SortMetrics *metrics) {
    if (metrics == NULL) {
        return;
    }
    metrics->comparisons = 0;
    metrics->record_swaps = 0;
    metrics->record_shifts = 0;
    metrics->elapsed_sec = 0.0;
}

const char *sort_key_to_string(SortKey key) {
    switch (key) {
        case SORT_KEY_TIMESTAMP:     return "timestamp";
        case SORT_KEY_ROUTE:         return "route";
        case SORT_KEY_STATUS_CODE:   return "status_code";
        case SORT_KEY_RESPONSE_TIME: return "response_time_ms";
        case SORT_KEY_CLIENT_ID:     return "client_id";
        default:                     return "unknown";
    }
}

int log_entry_compare(const LogEntry *a, const LogEntry *b, SortKey key, bool ascending) {
    if (a == NULL && b == NULL) return 0;
    if (a == NULL) return 1;
    if (b == NULL) return -1;

    int diff = 0;
    switch (key) {
        case SORT_KEY_TIMESTAMP: {
            int res = strcmp(a->timestamp, b->timestamp);
            diff = (res > 0) - (res < 0);
            break;
        }
        case SORT_KEY_ROUTE: {
            int res = strcmp(a->route, b->route);
            diff = (res > 0) - (res < 0);
            break;
        }
        case SORT_KEY_STATUS_CODE:
            diff = (a->status_code > b->status_code) - (a->status_code < b->status_code);
            break;
        case SORT_KEY_RESPONSE_TIME:
            diff = (a->response_time_ms > b->response_time_ms) - (a->response_time_ms < b->response_time_ms);
            break;
        case SORT_KEY_CLIENT_ID: {
            int res = strcmp(a->client_id, b->client_id);
            diff = (res > 0) - (res < 0);
            break;
        }
        default:
            diff = 0;
            break;
    }

    if (diff != 0) {
        return ascending ? diff : -diff;
    }

    /*
     * Tie-breaking rule: ALWAYS ascending source_line regardless of primary
     * sort direction, preserving deterministic stability.
     */
    if (a->source_line < b->source_line) {
        return -1;
    }
    if (a->source_line > b->source_line) {
        return 1;
    }
    return 0;
}

bool sort_bubble(LogArray *arr, SortKey key, bool ascending, SortMetrics *metrics) {
    if (metrics != NULL) {
        sort_metrics_reset(metrics);
    }
    if (arr == NULL) {
        return false;
    }
    if (key < SORT_KEY_TIMESTAMP || key > SORT_KEY_CLIENT_ID) {
        return false;
    }
    if (arr->size < 2) {
        return true;
    }
    if (arr->entries == NULL) {
        return false;
    }

    double start_time = get_time_sec();
    size_t n = arr->size;

    for (size_t i = 0; i < n - 1; i++) {
        bool swapped = false;
        for (size_t j = 0; j < n - 1 - i; j++) {
            if (metrics != NULL) {
                metrics->comparisons++;
            }
            if (log_entry_compare(&arr->entries[j], &arr->entries[j + 1], key, ascending) > 0) {
                /* Complete LogEntry struct swap to preserve field integrity */
                LogEntry temp = arr->entries[j];
                arr->entries[j] = arr->entries[j + 1];
                arr->entries[j + 1] = temp;
                if (metrics != NULL) {
                    metrics->record_swaps++;
                }
                swapped = true;
            }
        }
        if (!swapped) {
            break; /* Early termination */
        }
    }

    if (metrics != NULL) {
        metrics->elapsed_sec = get_time_sec() - start_time;
    }
    return true;
}

bool sort_selection(LogArray *arr, SortKey key, bool ascending, SortMetrics *metrics) {
    if (metrics != NULL) {
        sort_metrics_reset(metrics);
    }
    if (arr == NULL) {
        return false;
    }
    if (key < SORT_KEY_TIMESTAMP || key > SORT_KEY_CLIENT_ID) {
        return false;
    }
    if (arr->size < 2) {
        return true;
    }
    if (arr->entries == NULL) {
        return false;
    }

    double start_time = get_time_sec();
    size_t n = arr->size;

    for (size_t i = 0; i < n - 1; i++) {
        size_t target_idx = i;
        for (size_t j = i + 1; j < n; j++) {
            if (metrics != NULL) {
                metrics->comparisons++;
            }
            if (log_entry_compare(&arr->entries[j], &arr->entries[target_idx], key, ascending) < 0) {
                target_idx = j;
            }
        }
        /* Avoiding unnecessary self-swaps: exchange only if target differs from i */
        if (target_idx != i) {
            LogEntry temp = arr->entries[i];
            arr->entries[i] = arr->entries[target_idx];
            arr->entries[target_idx] = temp;
            if (metrics != NULL) {
                metrics->record_swaps++;
            }
        }
    }

    if (metrics != NULL) {
        metrics->elapsed_sec = get_time_sec() - start_time;
    }
    return true;
}

bool sort_insertion(LogArray *arr, SortKey key, bool ascending, SortMetrics *metrics) {
    if (metrics != NULL) {
        sort_metrics_reset(metrics);
    }
    if (arr == NULL) {
        return false;
    }
    if (key < SORT_KEY_TIMESTAMP || key > SORT_KEY_CLIENT_ID) {
        return false;
    }
    if (arr->size < 2) {
        return true;
    }
    if (arr->entries == NULL) {
        return false;
    }

    double start_time = get_time_sec();
    size_t n = arr->size;

    for (size_t i = 1; i < n; i++) {
        LogEntry key_entry = arr->entries[i];
        size_t j = i;
        while (j > 0) {
            if (metrics != NULL) {
                metrics->comparisons++;
            }
            if (log_entry_compare(&key_entry, &arr->entries[j - 1], key, ascending) < 0) {
                /* Complete LogEntry record shift */
                arr->entries[j] = arr->entries[j - 1];
                if (metrics != NULL) {
                    metrics->record_shifts++;
                }
                j--;
            } else {
                break;
            }
        }
        if (j != i) {
            arr->entries[j] = key_entry;
        }
    }

    if (metrics != NULL) {
        metrics->elapsed_sec = get_time_sec() - start_time;
    }
    return true;
}
