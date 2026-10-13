#ifndef SORT_H
#define SORT_H

#include "log_array.h"
#include <stdbool.h>

/*
 * Sortable fields documented in PRD/SRS §3.2 and Milestone 4 requirements.
 */
typedef enum {
    SORT_KEY_TIMESTAMP     = 0,
    SORT_KEY_ROUTE         = 1,
    SORT_KEY_STATUS_CODE   = 2,
    SORT_KEY_RESPONSE_TIME = 3,
    SORT_KEY_CLIENT_ID     = 4
} SortKey;

/*
 * SortMetrics tracks exact empirical operations performed during sorting.
 *
 * Counter definitions:
 * - comparisons:   Pairwise element-to-element ordering comparisons evaluated.
 *                  Loop termination condition checks are NOT counted.
 * - record_swaps:  Number of complete two-record exchanges (LogEntry temp = a; a = b; b = temp).
 *                  Used by Bubble Sort and Selection Sort.
 * - record_shifts: Number of single-record shifts during displacement (arr[j] = arr[j-1]).
 *                  Used by Insertion Sort.
 * - elapsed_sec:   Total sorting execution duration in seconds.
 *                  Measured via C11 timespec_get() with fallback to clock().
 */
typedef struct {
    unsigned long long comparisons;
    unsigned long long record_swaps;
    unsigned long long record_shifts;
    double             elapsed_sec;
} SortMetrics;

/*
 * Resets all fields in metrics to zero. Safe on NULL.
 */
void sort_metrics_reset(SortMetrics *metrics);

/*
 * Converts SortKey enum value to human-readable string.
 */
const char *sort_key_to_string(SortKey key);

/*
 * Deterministic comparison function between two LogEntry records.
 *
 * Rules:
 * 1. Primary ordering: Compares the field specified by 'key'.
 *    - If ascending is true, smaller values precede larger values.
 *    - If ascending is false, larger values precede smaller values.
 * 2. Tie-breaking rule:
 *    - If primary key values are equal, ties are ALWAYS broken by
 *      ascending source_line, regardless of the primary 'ascending' direction.
 *
 * Returns:
 *   < 0 if 'a' precedes 'b'
 *   > 0 if 'a' succeeds 'b'
 *     0 if 'a' and 'b' are identical in both primary key and source_line
 */
int log_entry_compare(const LogEntry *a, const LogEntry *b, SortKey key, bool ascending);

/*
 * Bubble Sort:
 * Repeatedly sweeps the array comparing adjacent pairs and swapping complete
 * LogEntry records when out of order.
 * Employs early exit optimization: stops immediately if a full pass makes 0 swaps.
 *
 * Returns true on success, false on NULL array, NULL entries, or invalid key.
 * Arrays with size < 2 are trivially sorted and return true with 0 operations.
 */
bool sort_bubble(LogArray *arr, SortKey key, bool ascending, SortMetrics *metrics);

/*
 * Selection Sort:
 * In pass i, scans the unsorted subarray to find the minimum/maximum element,
 * then exchanges it with arr[i].
 * Makes at most one complete LogEntry record swap per pass, avoiding
 * redundant self-swaps when the target element is already at index i.
 *
 * Returns true on success, false on NULL array, NULL entries, or invalid key.
 * Arrays with size < 2 are trivially sorted and return true with 0 operations.
 */
bool sort_selection(LogArray *arr, SortKey key, bool ascending, SortMetrics *metrics);

/*
 * Insertion Sort:
 * Builds the sorted sequence incrementally by shifting preceding elements rightward
 * to open a slot for the inserted record.
 * Moves complete LogEntry records during every shift and preserves all fields of
 * the inserted record.
 *
 * Returns true on success, false on NULL array, NULL entries, or invalid key.
 * Arrays with size < 2 are trivially sorted and return true with 0 operations.
 */
bool sort_insertion(LogArray *arr, SortKey key, bool ascending, SortMetrics *metrics);

#endif /* SORT_H */
