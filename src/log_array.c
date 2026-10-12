#include "log_array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

bool log_array_init(LogArray *arr, size_t initial_capacity) {
    if (arr == NULL) {
        return false;
    }

    size_t cap = (initial_capacity == 0) ? LW_INITIAL_CAPACITY : initial_capacity;

    /* Overflow check on capacity * sizeof(LogEntry) */
    if (cap > SIZE_MAX / sizeof(LogEntry)) {
        return false;
    }

    arr->entries = (LogEntry *)malloc(cap * sizeof(LogEntry));
    if (arr->entries == NULL) {
        arr->size = 0;
        arr->capacity = 0;
        return false;
    }

    arr->size = 0;
    arr->capacity = cap;
    arr->provenance = PROV_SYNTHETIC;
    return true;
}

bool log_array_append(LogArray *arr, const LogEntry *entry) {
    if (arr == NULL || entry == NULL) {
        return false;
    }

    /*
     * Safety buffer: Copy entry to local storage first.
     * This protects against self-referential aliasing (e.g., if entry points
     * to an element inside arr->entries and a realloc moves the memory block).
     */
    LogEntry to_append = *entry;

    if (arr->entries == NULL || arr->capacity == 0) {
        if (!log_array_init(arr, LW_INITIAL_CAPACITY)) {
            return false;
        }
    }

    if (arr->size >= arr->capacity) {
        /* Check multiplication overflow for doubling */
        if (arr->capacity > (SIZE_MAX / 2) / sizeof(LogEntry)) {
            return false;
        }

        size_t new_cap = arr->capacity * 2;
        if (new_cap > SIZE_MAX / sizeof(LogEntry)) {
            return false;
        }

        LogEntry *new_entries = (LogEntry *)realloc(arr->entries, new_cap * sizeof(LogEntry));
        if (new_entries == NULL) {
            /* On allocation failure, existing storage and elements remain intact */
            return false;
        }

        arr->entries = new_entries;
        arr->capacity = new_cap;
    }

    arr->entries[arr->size] = to_append;
    arr->size++;
    return true;
}

LogEntry *log_array_get(LogArray *arr, size_t index) {
    if (arr == NULL || arr->entries == NULL || index >= arr->size) {
        return NULL;
    }
    return &arr->entries[index];
}

const LogEntry *log_array_get_const(const LogArray *arr, size_t index) {
    if (arr == NULL || arr->entries == NULL || index >= arr->size) {
        return NULL;
    }
    return &arr->entries[index];
}

bool log_array_get_at(const LogArray *arr, size_t index, LogEntry *out) {
    if (arr == NULL || arr->entries == NULL || out == NULL || index >= arr->size) {
        return false;
    }
    *out = arr->entries[index];
    return true;
}

size_t log_array_size(const LogArray *arr) {
    return (arr != NULL) ? arr->size : 0;
}

size_t log_array_capacity(const LogArray *arr) {
    return (arr != NULL) ? arr->capacity : 0;
}

bool log_array_is_empty(const LogArray *arr) {
    return (arr == NULL || arr->size == 0);
}

void log_array_clear(LogArray *arr) {
    if (arr != NULL) {
        arr->size = 0;
    }
}

void log_array_free(LogArray *arr) {
    if (arr == NULL) {
        return;
    }

    if (arr->entries != NULL) {
        free(arr->entries);
        arr->entries = NULL;
    }
    arr->size = 0;
    arr->capacity = 0;
}

bool log_array_clone(const LogArray *src, LogArray *dest) {
    if (src == NULL || dest == NULL) {
        return false;
    }

    size_t cap = (src->capacity > 0) ? src->capacity : 
                 ((src->size > 0) ? src->size : LW_INITIAL_CAPACITY);

    if (!log_array_init(dest, cap)) {
        return false;
    }

    if (src->size > 0 && src->entries != NULL) {
        memcpy(dest->entries, src->entries, src->size * sizeof(LogEntry));
    }

    dest->size = src->size;
    dest->provenance = src->provenance;
    return true;
}

void log_array_display(const LogArray *arr, size_t max_count) {
    if (arr == NULL || arr->entries == NULL || arr->size == 0) {
        printf("(empty log array) %s\n", provenance_to_string(arr ? arr->provenance : PROV_SYNTHETIC));
        return;
    }

    printf("\n=== LOG RECORD DISPLAY (%s) [Total: %zu] ===\n",
           provenance_to_string(arr->provenance), arr->size);
    log_entry_print_header();

    size_t limit = (max_count > 0 && max_count < arr->size) ? max_count : arr->size;
    for (size_t i = 0; i < limit; i++) {
        log_entry_print(&arr->entries[i]);
    }

    if (limit < arr->size) {
        printf("... (%zu additional records omitted) ...\n", arr->size - limit);
    }
    printf("====================================================\n\n");
}

Provenance log_array_get_provenance(const LogArray *arr) {
    return (arr != NULL) ? arr->provenance : PROV_SYNTHETIC;
}

void log_array_set_provenance(LogArray *arr, Provenance prov) {
    if (arr != NULL) {
        arr->provenance = prov;
    }
}
