#include "search.h"
#include <string.h>
#include <ctype.h>
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

void search_metrics_reset(SearchMetrics *metrics) {
    if (metrics == NULL) {
        return;
    }
    metrics->comparisons = 0;
    metrics->matches_found = 0;
    metrics->elapsed_sec = 0.0;
}

/*
 * Checks whether 'str' starts with 'prefix'.
 */
static bool route_matches_prefix(const char *str, const char *prefix, bool case_sensitive) {
    if (str == NULL || prefix == NULL) {
        return false;
    }
    size_t prefix_len = strlen(prefix);
    size_t str_len = strlen(str);
    if (str_len < prefix_len) {
        return false;
    }

    if (case_sensitive) {
        return strncmp(str, prefix, prefix_len) == 0;
    }

    for (size_t i = 0; i < prefix_len; i++) {
        if (tolower((unsigned char)str[i]) != tolower((unsigned char)prefix[i])) {
            return false;
        }
    }
    return true;
}

/*
 * Checks whether 'str' contains 'sub' anywhere.
 */
static bool route_matches_substring(const char *str, const char *sub, bool case_sensitive) {
    if (str == NULL || sub == NULL) {
        return false;
    }
    size_t sub_len = strlen(sub);
    if (sub_len == 0) {
        return true;
    }
    size_t str_len = strlen(str);
    if (str_len < sub_len) {
        return false;
    }

    if (case_sensitive) {
        return strstr(str, sub) != NULL;
    }

    for (size_t i = 0; i <= str_len - sub_len; i++) {
        bool match = true;
        for (size_t j = 0; j < sub_len; j++) {
            if (tolower((unsigned char)str[i + j]) != tolower((unsigned char)sub[j])) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }
    return false;
}

size_t search_linear_by_response_time(const LogArray *arr, int target_ms,
                                      size_t *out_indices, size_t max_indices,
                                      SearchMetrics *metrics) {
    if (metrics != NULL) {
        search_metrics_reset(metrics);
    }
    if (arr == NULL || arr->entries == NULL || arr->size == 0) {
        return 0;
    }

    double start_time = get_time_sec();
    size_t stored_count = 0;
    size_t total_matches = 0;

    for (size_t i = 0; i < arr->size; i++) {
        if (metrics != NULL) {
            metrics->comparisons++;
        }
        if (arr->entries[i].response_time_ms == target_ms) {
            if (out_indices != NULL && stored_count < max_indices) {
                out_indices[stored_count++] = i;
            }
            total_matches++;
        }
    }

    if (metrics != NULL) {
        metrics->matches_found = total_matches;
        metrics->elapsed_sec = get_time_sec() - start_time;
    }
    return total_matches;
}

size_t search_linear_by_threshold(const LogArray *arr, int threshold_ms,
                                  size_t *out_indices, size_t max_indices,
                                  SearchMetrics *metrics) {
    if (metrics != NULL) {
        search_metrics_reset(metrics);
    }
    if (arr == NULL || arr->entries == NULL || arr->size == 0) {
        return 0;
    }

    double start_time = get_time_sec();
    size_t stored_count = 0;
    size_t total_matches = 0;

    for (size_t i = 0; i < arr->size; i++) {
        if (metrics != NULL) {
            metrics->comparisons++;
        }
        if (arr->entries[i].response_time_ms >= threshold_ms) {
            if (out_indices != NULL && stored_count < max_indices) {
                out_indices[stored_count++] = i;
            }
            total_matches++;
        }
    }

    if (metrics != NULL) {
        metrics->matches_found = total_matches;
        metrics->elapsed_sec = get_time_sec() - start_time;
    }
    return total_matches;
}

size_t search_linear_by_status(const LogArray *arr, int status_code,
                               size_t *out_indices, size_t max_indices,
                               SearchMetrics *metrics) {
    if (metrics != NULL) {
        search_metrics_reset(metrics);
    }
    if (arr == NULL || arr->entries == NULL || arr->size == 0) {
        return 0;
    }

    double start_time = get_time_sec();
    size_t stored_count = 0;
    size_t total_matches = 0;

    for (size_t i = 0; i < arr->size; i++) {
        if (metrics != NULL) {
            metrics->comparisons++;
        }
        if (arr->entries[i].status_code == status_code) {
            if (out_indices != NULL && stored_count < max_indices) {
                out_indices[stored_count++] = i;
            }
            total_matches++;
        }
    }

    if (metrics != NULL) {
        metrics->matches_found = total_matches;
        metrics->elapsed_sec = get_time_sec() - start_time;
    }
    return total_matches;
}

size_t search_linear_by_route_ext(const LogArray *arr, const char *route_query,
                                  bool prefix_only, bool case_sensitive,
                                  size_t *out_indices, size_t max_indices,
                                  SearchMetrics *metrics) {
    if (metrics != NULL) {
        search_metrics_reset(metrics);
    }
    if (arr == NULL || arr->entries == NULL || arr->size == 0 || route_query == NULL) {
        return 0;
    }

    double start_time = get_time_sec();
    size_t stored_count = 0;
    size_t total_matches = 0;

    for (size_t i = 0; i < arr->size; i++) {
        if (metrics != NULL) {
            metrics->comparisons++;
        }

        bool matched = prefix_only ?
            route_matches_prefix(arr->entries[i].route, route_query, case_sensitive) :
            route_matches_substring(arr->entries[i].route, route_query, case_sensitive);

        if (matched) {
            if (out_indices != NULL && stored_count < max_indices) {
                out_indices[stored_count++] = i;
            }
            total_matches++;
        }
    }

    if (metrics != NULL) {
        metrics->matches_found = total_matches;
        metrics->elapsed_sec = get_time_sec() - start_time;
    }
    return total_matches;
}

size_t search_linear_by_route(const LogArray *arr, const char *route_sub,
                              size_t *out_indices, size_t max_indices,
                              SearchMetrics *metrics) {
    return search_linear_by_route_ext(arr, route_sub, false, true, out_indices, max_indices, metrics);
}

size_t search_linear_by_route_prefix(const LogArray *arr, const char *route_prefix,
                                     size_t *out_indices, size_t max_indices,
                                     SearchMetrics *metrics) {
    return search_linear_by_route_ext(arr, route_prefix, true, true, out_indices, max_indices, metrics);
}

bool search_populate_results(const LogArray *src, const size_t *indices, size_t count, LogArray *dest) {
    if (src == NULL || dest == NULL) {
        return false;
    }
    if (count > 0 && indices == NULL) {
        return false;
    }
    if (count == 0) {
        return true;
    }

    /* Pre-validate all indices to ensure atomic failure (no partial mutations on error) */
    for (size_t i = 0; i < count; i++) {
        if (indices[i] >= src->size) {
            return false;
        }
    }

    /* Handle source/destination aliasing safely */
    if (src == dest) {
        LogArray temp;
        if (!log_array_init(&temp, count)) {
            return false;
        }
        for (size_t i = 0; i < count; i++) {
            const LogEntry *e = log_array_get_const(src, indices[i]);
            if (e == NULL || !log_array_append(&temp, e)) {
                log_array_free(&temp);
                return false;
            }
        }
        for (size_t i = 0; i < temp.size; i++) {
            if (!log_array_append(dest, &temp.entries[i])) {
                log_array_free(&temp);
                return false;
            }
        }
        log_array_free(&temp);
        return true;
    }

    for (size_t i = 0; i < count; i++) {
        const LogEntry *e = log_array_get_const(src, indices[i]);
        if (e == NULL || !log_array_append(dest, e)) {
            return false;
        }
    }
    return true;
}
