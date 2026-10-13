#include "wasm_bridge.h"
#include "log_array.h"
#include "parser.h"
#include "sort.h"
#include "search.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Maximum path length stored in bridge context
 */
#define BRIDGE_MAX_PATH 512

/*
 * Bridge Context holding the authoritative C runtime dataset state
 */
typedef struct {
    LogArray   records;
    ParseStats stats;
    char       filepath[BRIDGE_MAX_PATH];
    bool       is_initialized;
    bool       has_file;
} BridgeContext;

static BridgeContext g_bridge_ctx = {
    .records = { .entries = NULL, .size = 0, .capacity = 0, .provenance = PROV_SYNTHETIC },
    .stats = { 0, 0, 0, 0 },
    .filepath = { 0 },
    .is_initialized = false,
    .has_file = false
};

/*
 * ---------------------------------------------------------------------------
 * Safe Dynamic String Builder (StrBuf) with RFC 8259 JSON Escaping
 * ---------------------------------------------------------------------------
 */
typedef struct {
    char  *data;
    size_t len;
    size_t cap;
    bool   oom;
} StrBuf;

static bool strbuf_init(StrBuf *sb, size_t initial_cap) {
    if (!sb) return false;
    size_t cap = initial_cap ? initial_cap : 256;
    sb->data = (char *)malloc(cap);
    if (!sb->data) {
        sb->len = 0;
        sb->cap = 0;
        sb->oom = true;
        return false;
    }
    sb->data[0] = '\0';
    sb->len = 0;
    sb->cap = cap;
    sb->oom = false;
    return true;
}

static bool strbuf_ensure(StrBuf *sb, size_t extra) {
    if (!sb || sb->oom) return false;
    if (sb->len + extra + 1 <= sb->cap) return true;

    /* Check arithmetic overflow */
    if (sb->len > SIZE_MAX - extra - 1) {
        sb->oom = true;
        return false;
    }

    size_t required = sb->len + extra + 1;
    size_t new_cap = sb->cap ? sb->cap * 2 : 256;
    if (new_cap < required) {
        new_cap = required;
    }

    char *new_data = (char *)realloc(sb->data, new_cap);
    if (!new_data) {
        sb->oom = true;
        return false;
    }

    sb->data = new_data;
    sb->cap = new_cap;
    return true;
}

static void strbuf_append_str(StrBuf *sb, const char *s) {
    if (!sb || !s || sb->oom) return;
    size_t slen = strlen(s);
    if (!strbuf_ensure(sb, slen)) return;
    memcpy(sb->data + sb->len, s, slen);
    sb->len += slen;
    sb->data[sb->len] = '\0';
}

static void strbuf_append_escaped(StrBuf *sb, const char *s) {
    if (!sb || !s || sb->oom) return;
    for (size_t i = 0; s[i] != '\0'; i++) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
            case '"':  strbuf_append_str(sb, "\\\""); break;
            case '\\': strbuf_append_str(sb, "\\\\"); break;
            case '\b': strbuf_append_str(sb, "\\b"); break;
            case '\f': strbuf_append_str(sb, "\\f"); break;
            case '\n': strbuf_append_str(sb, "\\n"); break;
            case '\r': strbuf_append_str(sb, "\\r"); break;
            case '\t': strbuf_append_str(sb, "\\t"); break;
            default:
                if (c < 0x20) {
                    char hex[8];
                    snprintf(hex, sizeof(hex), "\\u%04x", (unsigned int)c);
                    strbuf_append_str(sb, hex);
                } else {
                    char ch[2] = { (char)c, '\0' };
                    strbuf_append_str(sb, ch);
                }
                break;
        }
    }
}

static void strbuf_append_int(StrBuf *sb, int val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", val);
    strbuf_append_str(sb, buf);
}

static void strbuf_append_size(StrBuf *sb, size_t val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%zu", val);
    strbuf_append_str(sb, buf);
}

static void strbuf_append_ull(StrBuf *sb, unsigned long long val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%llu", val);
    strbuf_append_str(sb, buf);
}

static void strbuf_append_double(StrBuf *sb, double val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f", val);
    strbuf_append_str(sb, buf);
}

static void strbuf_append_bool(StrBuf *sb, bool val) {
    strbuf_append_str(sb, val ? "true" : "false");
}

static char *strbuf_detach(StrBuf *sb) {
    if (!sb) return NULL;
    if (sb->oom) {
        if (sb->data) free(sb->data);
        sb->data = NULL;
        return NULL;
    }
    char *res = sb->data;
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
    return res;
}


/*
 * Appends a JSON representation of a single LogEntry record
 */
static void append_record_json(StrBuf *sb, const LogEntry *entry) {
    strbuf_append_str(sb, "{\"line\":");
    strbuf_append_int(sb, entry->source_line);
    strbuf_append_str(sb, ",\"timestamp\":\"");
    strbuf_append_escaped(sb, entry->timestamp);
    strbuf_append_str(sb, "\",\"route\":\"");
    strbuf_append_escaped(sb, entry->route);
    strbuf_append_str(sb, "\",\"status\":");
    strbuf_append_int(sb, entry->status_code);
    strbuf_append_str(sb, ",\"response_time_ms\":");
    strbuf_append_int(sb, entry->response_time_ms);
    strbuf_append_str(sb, ",\"client_id\":\"");
    strbuf_append_escaped(sb, entry->client_id);
    strbuf_append_str(sb, "\"}");
}

/*
 * Returns a dynamically allocated error JSON string
 */
static char *create_error_json(const char *message) {
    StrBuf sb;
    if (!strbuf_init(&sb, 128)) return NULL;
    strbuf_append_str(&sb, "{\"provenance\":\"[SYNTHETIC]\",\"success\":false,\"error\":\"");
    strbuf_append_escaped(&sb, message ? message : "Unknown error");
    strbuf_append_str(&sb, "\"}");
    return strbuf_detach(&sb);
}

/*
 * Callback handler for streaming parser during bridge ingestion
 */
static bool bridge_append_handler(const LogEntry *entry, void *user_data) {
    LogArray *arr = (LogArray *)user_data;
    return log_array_append(arr, entry);
}

/*
 * ---------------------------------------------------------------------------
 * Public Bridge Implementations
 * ---------------------------------------------------------------------------
 */

bool bridge_init(void) {
    if (g_bridge_ctx.is_initialized) {
        return true;
    }
    if (!log_array_init(&g_bridge_ctx.records, LW_INITIAL_CAPACITY)) {
        return false;
    }
    parse_stats_init(&g_bridge_ctx.stats);
    g_bridge_ctx.filepath[0] = '\0';
    g_bridge_ctx.has_file = false;
    g_bridge_ctx.is_initialized = true;
    return true;
}

bool bridge_load_from_vfs(const char *filepath) {
    if (!filepath || filepath[0] == '\0') {
        return false;
    }
    if (!g_bridge_ctx.is_initialized) {
        if (!bridge_init()) {
            return false;
        }
    }

    /*
     * Dataset Preservation Invariant:
     * Ingest into an isolated temporary array. If reading or parsing fails,
     * the existing loaded dataset is preserved intact without corruption.
     */
    LogArray temp_arr;
    if (!log_array_init(&temp_arr, LW_INITIAL_CAPACITY)) {
        return false;
    }

    ParseStats temp_stats;
    parse_stats_init(&temp_stats);

    bool ok = parse_file(filepath, bridge_append_handler, &temp_arr, &temp_stats);
    if (!ok) {
        log_array_free(&temp_arr);
        return false;
    }

    /* Successful load: release old array and swap to new */
    log_array_free(&g_bridge_ctx.records);
    g_bridge_ctx.records = temp_arr;
    g_bridge_ctx.stats = temp_stats;

    strncpy(g_bridge_ctx.filepath, filepath, sizeof(g_bridge_ctx.filepath) - 1);
    g_bridge_ctx.filepath[sizeof(g_bridge_ctx.filepath) - 1] = '\0';
    g_bridge_ctx.has_file = true;

    return true;
}

bool bridge_load_file(const char *filepath) {
    return bridge_load_from_vfs(filepath);
}

void bridge_clear(void) {
    if (!g_bridge_ctx.is_initialized) return;
    log_array_clear(&g_bridge_ctx.records);
    parse_stats_init(&g_bridge_ctx.stats);
    g_bridge_ctx.filepath[0] = '\0';
    g_bridge_ctx.has_file = false;
}

void bridge_free(void) {
    if (!g_bridge_ctx.is_initialized) return;
    log_array_free(&g_bridge_ctx.records);
    parse_stats_init(&g_bridge_ctx.stats);
    g_bridge_ctx.filepath[0] = '\0';
    g_bridge_ctx.has_file = false;
    g_bridge_ctx.is_initialized = false;
}

size_t bridge_get_size(void) {
    return g_bridge_ctx.is_initialized ? log_array_size(&g_bridge_ctx.records) : 0;
}

size_t bridge_get_capacity(void) {
    return g_bridge_ctx.is_initialized ? log_array_capacity(&g_bridge_ctx.records) : 0;
}

char *bridge_get_stats_json(void) {
    StrBuf sb;
    if (!strbuf_init(&sb, 256)) return NULL;

    size_t total = g_bridge_ctx.is_initialized ? g_bridge_ctx.stats.total_lines : 0;
    size_t valid = g_bridge_ctx.is_initialized ? g_bridge_ctx.stats.valid_records : 0;
    size_t malformed = g_bridge_ctx.is_initialized ? g_bridge_ctx.stats.malformed_lines : 0;
    size_t blank = g_bridge_ctx.is_initialized ? g_bridge_ctx.stats.blank_lines : 0;
    size_t size = bridge_get_size();
    size_t cap = bridge_get_capacity();
    bool invariant_ok = (total == valid + malformed + blank);

    strbuf_append_str(&sb, "{\"provenance\":\"[SYNTHETIC]\"");
    strbuf_append_str(&sb, ",\"filepath\":\"");
    strbuf_append_escaped(&sb, g_bridge_ctx.is_initialized ? g_bridge_ctx.filepath : "");
    strbuf_append_str(&sb, "\",\"has_file\":");
    strbuf_append_bool(&sb, g_bridge_ctx.has_file);
    strbuf_append_str(&sb, ",\"size\":");
    strbuf_append_size(&sb, size);
    strbuf_append_str(&sb, ",\"capacity\":");
    strbuf_append_size(&sb, cap);
    strbuf_append_str(&sb, ",\"total_lines\":");
    strbuf_append_size(&sb, total);
    strbuf_append_str(&sb, ",\"valid_records\":");
    strbuf_append_size(&sb, valid);
    strbuf_append_str(&sb, ",\"malformed_lines\":");
    strbuf_append_size(&sb, malformed);
    strbuf_append_str(&sb, ",\"blank_lines\":");
    strbuf_append_size(&sb, blank);
    strbuf_append_str(&sb, ",\"invariant_verified\":");
    strbuf_append_bool(&sb, invariant_ok);
    strbuf_append_str(&sb, "}");

    return strbuf_detach(&sb);
}

char *bridge_get_records_json(size_t offset, size_t limit) {
    StrBuf sb;
    if (!strbuf_init(&sb, 1024)) return NULL;

    size_t total = bridge_get_size();
    size_t count = 0;
    if (offset < total && limit > 0) {
        size_t available = total - offset;
        count = (limit < available) ? limit : available;
    }

    strbuf_append_str(&sb, "{\"provenance\":\"[SYNTHETIC]\"");
    strbuf_append_str(&sb, ",\"total_count\":");
    strbuf_append_size(&sb, total);
    strbuf_append_str(&sb, ",\"offset\":");
    strbuf_append_size(&sb, offset);
    strbuf_append_str(&sb, ",\"limit\":");
    strbuf_append_size(&sb, limit);
    strbuf_append_str(&sb, ",\"count\":");
    strbuf_append_size(&sb, count);
    strbuf_append_str(&sb, ",\"records\":[");

    for (size_t i = 0; i < count; i++) {
        if (i > 0) {
            strbuf_append_str(&sb, ",");
        }
        const LogEntry *entry = log_array_get_const(&g_bridge_ctx.records, offset + i);
        if (entry) {
            append_record_json(&sb, entry);
        }
    }

    strbuf_append_str(&sb, "]}");
    return strbuf_detach(&sb);
}

char *bridge_sort(int algorithm_id, int sort_key, bool ascending) {
    if (!g_bridge_ctx.is_initialized || g_bridge_ctx.records.size == 0) {
        return create_error_json("No records loaded to sort");
    }
    if (sort_key < 0 || sort_key > 4) {
        return create_error_json("Invalid sort_key (expected 0-4)");
    }
    if (algorithm_id < 0 || algorithm_id > 2) {
        return create_error_json("Invalid algorithm_id (expected 0=Bubble, 1=Selection, 2=Insertion)");
    }

    SortMetrics metrics;
    sort_metrics_reset(&metrics);
    bool ok = false;
    const char *algo_name = "";

    switch (algorithm_id) {
        case 0:
            algo_name = "Bubble Sort";
            ok = sort_bubble(&g_bridge_ctx.records, (SortKey)sort_key, ascending, &metrics);
            break;
        case 1:
            algo_name = "Selection Sort";
            ok = sort_selection(&g_bridge_ctx.records, (SortKey)sort_key, ascending, &metrics);
            break;
        case 2:
            algo_name = "Insertion Sort";
            ok = sort_insertion(&g_bridge_ctx.records, (SortKey)sort_key, ascending, &metrics);
            break;
        default:
            break;
    }

    if (!ok) {
        return create_error_json("Sorting operation failed");
    }

    StrBuf sb;
    if (!strbuf_init(&sb, 256)) return NULL;

    strbuf_append_str(&sb, "{\"provenance\":\"[SYNTHETIC]\"");
    strbuf_append_str(&sb, ",\"success\":true");
    strbuf_append_str(&sb, ",\"algorithm\":\"");
    strbuf_append_escaped(&sb, algo_name);
    strbuf_append_str(&sb, "\",\"sort_key\":\"");
    strbuf_append_escaped(&sb, sort_key_to_string((SortKey)sort_key));
    strbuf_append_str(&sb, "\",\"ascending\":");
    strbuf_append_bool(&sb, ascending);
    strbuf_append_str(&sb, ",\"comparisons\":");
    strbuf_append_ull(&sb, metrics.comparisons);
    strbuf_append_str(&sb, ",\"record_swaps\":");
    strbuf_append_ull(&sb, metrics.record_swaps);
    strbuf_append_str(&sb, ",\"record_shifts\":");
    strbuf_append_ull(&sb, metrics.record_shifts);
    strbuf_append_str(&sb, ",\"elapsed_sec\":");
    strbuf_append_double(&sb, metrics.elapsed_sec);
    strbuf_append_str(&sb, "}");

    return strbuf_detach(&sb);
}

char *bridge_compare_all_sorts(int sort_key, bool ascending) {
    if (!g_bridge_ctx.is_initialized || g_bridge_ctx.records.size == 0) {
        return create_error_json("No records loaded to benchmark");
    }
    if (sort_key < 0 || sort_key > 4) {
        return create_error_json("Invalid sort_key (expected 0-4)");
    }

    /*
     * Non-destructive comparison:
     * Clone the active LogArray into three independent copies.
     * All algorithms evaluate against identical starting raw data.
     */
    LogArray clone_bubble;
    LogArray clone_select;
    LogArray clone_insert;

    if (!log_array_clone(&g_bridge_ctx.records, &clone_bubble) ||
        !log_array_clone(&g_bridge_ctx.records, &clone_select) ||
        !log_array_clone(&g_bridge_ctx.records, &clone_insert)) {
        log_array_free(&clone_bubble);
        log_array_free(&clone_select);
        log_array_free(&clone_insert);
        return create_error_json("Failed to clone arrays for comparison");
    }

    SortMetrics m_bubble, m_select, m_insert;
    sort_metrics_reset(&m_bubble);
    sort_metrics_reset(&m_select);
    sort_metrics_reset(&m_insert);

    sort_bubble(&clone_bubble, (SortKey)sort_key, ascending, &m_bubble);
    sort_selection(&clone_select, (SortKey)sort_key, ascending, &m_select);
    sort_insertion(&clone_insert, (SortKey)sort_key, ascending, &m_insert);

    log_array_free(&clone_bubble);
    log_array_free(&clone_select);
    log_array_free(&clone_insert);

    StrBuf sb;
    if (!strbuf_init(&sb, 512)) return NULL;

    strbuf_append_str(&sb, "{\"provenance\":\"[SYNTHETIC]\"");
    strbuf_append_str(&sb, ",\"success\":true");
    strbuf_append_str(&sb, ",\"record_count\":");
    strbuf_append_size(&sb, g_bridge_ctx.records.size);
    strbuf_append_str(&sb, ",\"sort_key\":\"");
    strbuf_append_escaped(&sb, sort_key_to_string((SortKey)sort_key));
    strbuf_append_str(&sb, "\",\"ascending\":");
    strbuf_append_bool(&sb, ascending);
    strbuf_append_str(&sb, ",\"algorithms\":[");

    /* Bubble Sort */
    strbuf_append_str(&sb, "{\"name\":\"Bubble Sort\",\"comparisons\":");
    strbuf_append_ull(&sb, m_bubble.comparisons);
    strbuf_append_str(&sb, ",\"record_swaps\":");
    strbuf_append_ull(&sb, m_bubble.record_swaps);
    strbuf_append_str(&sb, ",\"record_shifts\":");
    strbuf_append_ull(&sb, m_bubble.record_shifts);
    strbuf_append_str(&sb, ",\"elapsed_sec\":");
    strbuf_append_double(&sb, m_bubble.elapsed_sec);
    strbuf_append_str(&sb, "},");

    /* Selection Sort */
    strbuf_append_str(&sb, "{\"name\":\"Selection Sort\",\"comparisons\":");
    strbuf_append_ull(&sb, m_select.comparisons);
    strbuf_append_str(&sb, ",\"record_swaps\":");
    strbuf_append_ull(&sb, m_select.record_swaps);
    strbuf_append_str(&sb, ",\"record_shifts\":");
    strbuf_append_ull(&sb, m_select.record_shifts);
    strbuf_append_str(&sb, ",\"elapsed_sec\":");
    strbuf_append_double(&sb, m_select.elapsed_sec);
    strbuf_append_str(&sb, "},");

    /* Insertion Sort */
    strbuf_append_str(&sb, "{\"name\":\"Insertion Sort\",\"comparisons\":");
    strbuf_append_ull(&sb, m_insert.comparisons);
    strbuf_append_str(&sb, ",\"record_swaps\":");
    strbuf_append_ull(&sb, m_insert.record_swaps);
    strbuf_append_str(&sb, ",\"record_shifts\":");
    strbuf_append_ull(&sb, m_insert.record_shifts);
    strbuf_append_str(&sb, ",\"elapsed_sec\":");
    strbuf_append_double(&sb, m_insert.elapsed_sec);
    strbuf_append_str(&sb, "}");

    strbuf_append_str(&sb, "]}");
    return strbuf_detach(&sb);
}

/*
 * Helper to serialize search results with indices resolved into records
 */
static char *format_search_response(const char *query_type,
                                    const size_t *indices,
                                    size_t total_matches,
                                    const SearchMetrics *metrics) {
    StrBuf sb;
    if (!strbuf_init(&sb, 1024)) return NULL;

    strbuf_append_str(&sb, "{\"provenance\":\"[SYNTHETIC]\"");
    strbuf_append_str(&sb, ",\"success\":true");
    strbuf_append_str(&sb, ",\"query_type\":\"");
    strbuf_append_escaped(&sb, query_type);
    strbuf_append_str(&sb, "\",\"matches_found\":");
    strbuf_append_size(&sb, total_matches);
    strbuf_append_str(&sb, ",\"comparisons\":");
    strbuf_append_ull(&sb, metrics ? metrics->comparisons : 0);
    strbuf_append_str(&sb, ",\"elapsed_sec\":");
    strbuf_append_double(&sb, metrics ? metrics->elapsed_sec : 0.0);
    strbuf_append_str(&sb, ",\"records\":[");

    for (size_t i = 0; i < total_matches; i++) {
        if (i > 0) {
            strbuf_append_str(&sb, ",");
        }
        size_t idx = indices[i];
        const LogEntry *entry = log_array_get_const(&g_bridge_ctx.records, idx);
        if (entry) {
            append_record_json(&sb, entry);
        }
    }

    strbuf_append_str(&sb, "]}");
    return strbuf_detach(&sb);
}

char *bridge_search_response_time(int target_ms) {
    if (!g_bridge_ctx.is_initialized || g_bridge_ctx.records.size == 0) {
        SearchMetrics empty_m = { 0, 0, 0.0 };
        return format_search_response("response_time", NULL, 0, &empty_m);
    }

    size_t size = g_bridge_ctx.records.size;
    size_t *indices = (size_t *)malloc(size * sizeof(size_t));
    if (!indices) return create_error_json("Memory allocation failed for search");

    SearchMetrics metrics;
    search_metrics_reset(&metrics);

    size_t matches = search_linear_by_response_time(&g_bridge_ctx.records, target_ms,
                                                   indices, size, &metrics);

    char *res = format_search_response("response_time", indices, matches, &metrics);
    free(indices);
    return res;
}

char *bridge_search_threshold(int threshold_ms) {
    if (!g_bridge_ctx.is_initialized || g_bridge_ctx.records.size == 0) {
        SearchMetrics empty_m = { 0, 0, 0.0 };
        return format_search_response("threshold", NULL, 0, &empty_m);
    }

    size_t size = g_bridge_ctx.records.size;
    size_t *indices = (size_t *)malloc(size * sizeof(size_t));
    if (!indices) return create_error_json("Memory allocation failed for search");

    SearchMetrics metrics;
    search_metrics_reset(&metrics);

    size_t matches = search_linear_by_threshold(&g_bridge_ctx.records, threshold_ms,
                                                indices, size, &metrics);

    char *res = format_search_response("threshold", indices, matches, &metrics);
    free(indices);
    return res;
}

char *bridge_search_status(int status_code) {
    if (!g_bridge_ctx.is_initialized || g_bridge_ctx.records.size == 0) {
        SearchMetrics empty_m = { 0, 0, 0.0 };
        return format_search_response("status_code", NULL, 0, &empty_m);
    }

    size_t size = g_bridge_ctx.records.size;
    size_t *indices = (size_t *)malloc(size * sizeof(size_t));
    if (!indices) return create_error_json("Memory allocation failed for search");

    SearchMetrics metrics;
    search_metrics_reset(&metrics);

    size_t matches = search_linear_by_status(&g_bridge_ctx.records, status_code,
                                             indices, size, &metrics);

    char *res = format_search_response("status_code", indices, matches, &metrics);
    free(indices);
    return res;
}

char *bridge_search_route(const char *route_query, bool prefix_only, bool case_sensitive) {
    if (!g_bridge_ctx.is_initialized || g_bridge_ctx.records.size == 0 || !route_query) {
        SearchMetrics empty_m = { 0, 0, 0.0 };
        return format_search_response("route", NULL, 0, &empty_m);
    }

    size_t size = g_bridge_ctx.records.size;
    size_t *indices = (size_t *)malloc(size * sizeof(size_t));
    if (!indices) return create_error_json("Memory allocation failed for search");

    SearchMetrics metrics;
    search_metrics_reset(&metrics);

    size_t matches = search_linear_by_route_ext(&g_bridge_ctx.records, route_query,
                                                prefix_only, case_sensitive,
                                                indices, size, &metrics);

    char *res = format_search_response("route", indices, matches, &metrics);
    free(indices);
    return res;
}

void bridge_free_string(char *str) {
    if (str != NULL) {
        free(str);
    }
}
