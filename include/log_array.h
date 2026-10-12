#ifndef LOG_ARRAY_H
#define LOG_ARRAY_H

#include <stddef.h>
#include <stdbool.h>
#include "log_entry.h"

#define LW_INITIAL_CAPACITY 16

/*
 * LogArray encapsulates a dynamically resizable array of complete LogEntry structs.
 * Memory ownership contract:
 * - LogArray owns the dynamically allocated contiguous array pointed to by entries.
 * - Appending copies the complete LogEntry struct into owned memory.
 * - Calling log_array_free releases all allocated memory and resets the array to empty.
 */
typedef struct {
    LogEntry   *entries;
    size_t      size;
    size_t      capacity;
    Provenance  provenance;
} LogArray;

/*
 * Initialize an empty LogArray with the given initial capacity.
 * If initial_capacity is 0, LW_INITIAL_CAPACITY (16) is used.
 * Sets size = 0 and provenance = PROV_SYNTHETIC.
 * Returns true on success, false on allocation failure or null pointer.
 */
bool log_array_init(LogArray *arr, size_t initial_capacity);

/*
 * Appends a copy of entry to the array.
 * Automatically doubles capacity when full.
 * Supports self-referential / aliased input pointers (e.g. entry pointing inside arr->entries)
 * by buffering the struct before any potential reallocation.
 * Returns true on success, false on allocation failure or null pointer.
 */
bool log_array_append(LogArray *arr, const LogEntry *entry);

/*
 * Returns a pointer to the LogEntry at index, or NULL if index >= size or arr is NULL.
 */
LogEntry *log_array_get(LogArray *arr, size_t index);

/*
 * Returns a const pointer to the LogEntry at index, or NULL if index >= size or arr is NULL.
 */
const LogEntry *log_array_get_const(const LogArray *arr, size_t index);

/*
 * Safely copies the LogEntry at index into out.
 * Returns true if index is valid and out is non-null; false otherwise.
 */
bool log_array_get_at(const LogArray *arr, size_t index, LogEntry *out);

/*
 * Returns current number of valid elements stored.
 */
size_t log_array_size(const LogArray *arr);

/*
 * Returns current allocated capacity.
 */
size_t log_array_capacity(const LogArray *arr);

/*
 * Returns true if size is 0 or arr is NULL.
 */
bool log_array_is_empty(const LogArray *arr);

/*
 * Clears stored records by setting size = 0 while retaining allocated capacity.
 */
void log_array_clear(LogArray *arr);

/*
 * Releases all allocated heap memory, sets entries = NULL, size = 0, capacity = 0.
 * Safe to call repeatedly on already-freed or empty arrays.
 */
void log_array_free(LogArray *arr);

/*
 * Clones src into dest. Dest is initialized with an independent heap allocation
 * containing identical LogEntry copies.
 * Returns true on success, false on failure or null pointer.
 */
bool log_array_clone(const LogArray *src, LogArray *dest);

/*
 * Prints up to max_count records in tabular format.
 */
void log_array_display(const LogArray *arr, size_t max_count);

/*
 * Provenance getters and setters.
 */
Provenance log_array_get_provenance(const LogArray *arr);
void log_array_set_provenance(LogArray *arr, Provenance prov);

#endif /* LOG_ARRAY_H */
