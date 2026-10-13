#ifndef SEARCH_H
#define SEARCH_H

#include "log_array.h"
#include <stdbool.h>

/*
 * SearchMetrics tracks empirical linear search operations.
 *
 * Counter definitions:
 * - comparisons:   Total number of records examined in the array.
 *                  For multi-match linear scans across the entire array,
 *                  each record is examined exactly once (comparisons == arr->size).
 * - matches_found: Total number of matching records satisfying the query predicate.
 * - elapsed_sec:   Informational execution duration in seconds.
 *                  Measured via C11 timespec_get() with fallback to clock().
 */
typedef struct {
    unsigned long long comparisons;
    size_t             matches_found;
    double             elapsed_sec;
} SearchMetrics;

/*
 * Resets all fields in metrics to zero. Safe on NULL.
 */
void search_metrics_reset(SearchMetrics *metrics);

/*
 * 1. Exact Response-Time Search
 *
 * Scans arr for records where response_time_ms == target_ms.
 * Writes up to max_indices matching indices into out_indices.
 *
 * Returns: Total number of matching records found in arr (regardless of max_indices).
 * If out_indices is NULL or max_indices is 0, counts matches without writing.
 * If max_indices < total matches, writes the first max_indices matches safely.
 */
size_t search_linear_by_response_time(const LogArray *arr, int target_ms,
                                      size_t *out_indices, size_t max_indices,
                                      SearchMetrics *metrics);

/*
 * 2. Response-Time Threshold Search
 *
 * Scans arr for records where response_time_ms >= threshold_ms.
 * Writes up to max_indices matching indices into out_indices.
 *
 * Returns: Total number of matching records found in arr.
 */
size_t search_linear_by_threshold(const LogArray *arr, int threshold_ms,
                                  size_t *out_indices, size_t max_indices,
                                  SearchMetrics *metrics);

/*
 * 3. Exact HTTP Status-Code Search
 *
 * Scans arr for records where status_code == status_code.
 * Writes up to max_indices matching indices into out_indices.
 *
 * Returns: Total number of matching records found in arr.
 */
size_t search_linear_by_status(const LogArray *arr, int status_code,
                               size_t *out_indices, size_t max_indices,
                               SearchMetrics *metrics);

/*
 * 4. Route Substring Search (Default Case-Sensitive)
 *
 * Scans arr for records where route contains route_sub as a substring.
 * Writes up to max_indices matching indices into out_indices.
 *
 * Returns: Total number of matching records found in arr.
 */
size_t search_linear_by_route(const LogArray *arr, const char *route_sub,
                              size_t *out_indices, size_t max_indices,
                              SearchMetrics *metrics);

/*
 * Route Prefix Search (Default Case-Sensitive)
 *
 * Scans arr for records where route starts with route_prefix.
 * Writes up to max_indices matching indices into out_indices.
 *
 * Returns: Total number of matching records found in arr.
 */
size_t search_linear_by_route_prefix(const LogArray *arr, const char *route_prefix,
                                     size_t *out_indices, size_t max_indices,
                                     SearchMetrics *metrics);

/*
 * Configurable Route Search
 *
 * Allows configuring prefix matching (prefix_only) and case sensitivity (case_sensitive).
 * If prefix_only is true, matches from the beginning of route; otherwise matches anywhere.
 *
 * Returns: Total number of matching records found in arr.
 */
size_t search_linear_by_route_ext(const LogArray *arr, const char *route_query,
                                  bool prefix_only, bool case_sensitive,
                                  size_t *out_indices, size_t max_indices,
                                  SearchMetrics *metrics);

/*
 * Copies complete LogEntry records corresponding to indices from src into dest LogArray.
 * Preserves complete record integrity across every field.
 * Returns true on success, false on NULL pointers or index out of range.
 */
bool search_populate_results(const LogArray *src, const size_t *indices, size_t count, LogArray *dest);

#endif /* SEARCH_H */
