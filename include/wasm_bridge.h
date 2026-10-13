#ifndef WASM_BRIDGE_H
#define WASM_BRIDGE_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#define EMSCRIPTEN_KEEPALIVE_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define EMSCRIPTEN_KEEPALIVE_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 1. Lifecycle and Dataset Ingestion
 */

/*
 * Initializes the global bridge context and underlying LogArray.
 * Safe to call repeatedly. Returns true on success.
 */
EMSCRIPTEN_KEEPALIVE_EXPORT bool bridge_init(void);

/*
 * Loads a synthetic log file from the virtual filesystem (MEMFS).
 * If loading fails (missing file or I/O failure), the previously loaded
 * dataset is preserved intact without corruption or data loss.
 * Returns true on success, false on failure.
 */
EMSCRIPTEN_KEEPALIVE_EXPORT bool bridge_load_from_vfs(const char *filepath);

/*
 * Alias for bridge_load_from_vfs for interoperability.
 */
EMSCRIPTEN_KEEPALIVE_EXPORT bool bridge_load_file(const char *filepath);

/*
 * Clears active records while retaining capacity, resets stats.
 */
EMSCRIPTEN_KEEPALIVE_EXPORT void bridge_clear(void);

/*
 * Releases all heap memory allocated by the bridge and resets state.
 * Safe to call repeatedly.
 */
EMSCRIPTEN_KEEPALIVE_EXPORT void bridge_free(void);

/*
 * 2. Metadata and Ingestion Statistics
 */

/*
 * Returns the number of valid records loaded.
 */
EMSCRIPTEN_KEEPALIVE_EXPORT size_t bridge_get_size(void);

/*
 * Returns the current capacity of the internal LogArray.
 */
EMSCRIPTEN_KEEPALIVE_EXPORT size_t bridge_get_capacity(void);

/*
 * Returns a dynamically allocated JSON string with ingestion statistics
 * and the accounting invariant check (total == valid + malformed + blank).
 * Caller must free the returned string via bridge_free_string().
 */
EMSCRIPTEN_KEEPALIVE_EXPORT char *bridge_get_stats_json(void);

/*
 * 3. Paginated Record Retrieval
 */

/*
 * Returns a dynamically allocated JSON string containing up to 'limit'
 * records starting at index 'offset'.
 * Serializes all fields with proper RFC 8259 escaping.
 * Caller must free the returned string via bridge_free_string().
 */
EMSCRIPTEN_KEEPALIVE_EXPORT char *bridge_get_records_json(size_t offset, size_t limit);

/*
 * 4. Authoritative Sorting Execution
 */

/*
 * Executes sorting directly on the active LogArray using the C11 engine.
 * algorithm_id: 0 = Bubble Sort (early exit), 1 = Selection Sort, 2 = Insertion Sort
 * sort_key:     0 = Timestamp, 1 = Route, 2 = Status Code, 3 = Response Time, 4 = Client ID
 * ascending:    true = ascending, false = descending
 * Ties are broken deterministically by ascending source_line.
 * Returns a dynamically allocated JSON string with exact SortMetrics.
 * Caller must free the returned string via bridge_free_string().
 */
EMSCRIPTEN_KEEPALIVE_EXPORT char *bridge_sort(int algorithm_id, int sort_key, bool ascending);

/*
 * 5. Non-Destructive Algorithm Comparison Benchmark
 */

/*
 * Clones the active LogArray into three independent copies, executes
 * Bubble Sort, Selection Sort, and Insertion Sort on identical starting inputs,
 * records exact operation metrics for each, and frees the clones.
 * The active dataset is completely untouched.
 * Returns a dynamically allocated JSON string containing comparative metrics.
 * Caller must free the returned string via bridge_free_string().
 */
EMSCRIPTEN_KEEPALIVE_EXPORT char *bridge_compare_all_sorts(int sort_key, bool ascending);

/*
 * 6. Authoritative Linear Search Execution
 */

/*
 * Searches records where response_time_ms == target_ms.
 * Returns a dynamically allocated JSON string with SearchMetrics and matched records.
 * Caller must free the returned string via bridge_free_string().
 */
EMSCRIPTEN_KEEPALIVE_EXPORT char *bridge_search_response_time(int target_ms);

/*
 * Searches records where response_time_ms >= threshold_ms.
 * Returns a dynamically allocated JSON string with SearchMetrics and matched records.
 * Caller must free the returned string via bridge_free_string().
 */
EMSCRIPTEN_KEEPALIVE_EXPORT char *bridge_search_threshold(int threshold_ms);

/*
 * Searches records where status_code == status_code.
 * Returns a dynamically allocated JSON string with SearchMetrics and matched records.
 * Caller must free the returned string via bridge_free_string().
 */
EMSCRIPTEN_KEEPALIVE_EXPORT char *bridge_search_status(int status_code);

/*
 * Searches records matching route_query.
 * prefix_only: true = matches prefix, false = substring match
 * case_sensitive: true = case-sensitive, false = case-insensitive
 * Returns a dynamically allocated JSON string with SearchMetrics and matched records.
 * Caller must free the returned string via bridge_free_string().
 */
EMSCRIPTEN_KEEPALIVE_EXPORT char *bridge_search_route(const char *route_query, bool prefix_only, bool case_sensitive);

/*
 * 7. Memory Management Helper
 */

/*
 * Explicitly frees a JSON response string dynamically allocated by any bridge function.
 * Safe to call with NULL.
 */
EMSCRIPTEN_KEEPALIVE_EXPORT void bridge_free_string(char *str);

#ifdef __cplusplus
}
#endif

#endif /* WASM_BRIDGE_H */
