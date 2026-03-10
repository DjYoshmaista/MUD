#include "mud_arena_string.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

// String Duplication
char* mud_arena_strdup(MudArena* arena, const char* str) {
    if (arena == NULL || str == NULL) {
        return NULL;
    }

    size_t len = strlen(str);
    char* copy = mud_arena_alloc(arena, len + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, str, len + 1);  // Include null terminator
    return copy;
}

char* mud_arena_strndup(MudArena* arena, const char* str, size_t n) {
    if (arena == NULL || str == NULL) {
        return NULL;
    }

    // Find actual length (may be less than n)
    size_t len = 0;
    while (len < n && str[len] != '\0') {
        len++;
    }

    char* copy = mud_arena_alloc(arena, len + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, str, len);
    copy[len] = '\0';  // Null terminate
    return copy;
}

// String Formatting
char* mud_arena_vsprintf(MudArena* arena, const char* fmt, va_list args) {
    if (arena == NULL || fmt == NULL) {
        return NULL;
    }

    // First pass: deetermine required size
    va_list args_copy;
    va_copy(args_copy, args);
    int required = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    if (required < 0) {
        return NULL; // Format error
    }

    // Allocate and format
    char* result = mud_arena_alloc(arena, (size_t)required + 1);
    if (result == NULL) {
        return NULL;
    }

    vsnprintf(result, (size_t)required + 1, fmt, args);
    return result;
}

char* mud_arena_sprintf(MudArena* arena, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char* result = mud_arena_vsprintf(arena, fmt, args);
    va_end(args);
    return result;
}

// String Concatenation
char* mud_arena_strcat(MudArena* arena, size_t count, ...) {
    if (arena == NULL || count == 0) {
        return NULL;
    }

    va_list args;

    // First pass: calculate total length
    va_start(args, count);
    size_t total_len = 0;
    for (size_t i = 0; i < count; i++) {
        const char* s = va_arg(args, const char*);
        if (s != NULL) {
            total_len += strlen(s);
        }
    }
    va_end(args);

    // Allocate
    char* result = mud_arena_alloc(arena, total_len + 1);
    if (result == NULL) {
        return NULL;
    }

    // Second pass: copy strings
    va_start(args, count);
    char* dest = result;
    for (size_t i = 0; i < count; i++) {
        const char* s = va_arg(args, const char*);
        if (s != NULL) {
            size_t len = strlen(s);
            memcpy(dest, s, len);
            dest += len;
        }
    }
    va_end(args);

    *dest = '\0';   // Null terminate
    return result;
}

// String Join
char* mud_arena_strjoin(MudArena* arena, const char* separator, const char** strings, size_t count) {
    if (arena == NULL) {
        return NULL;
    }

    if (count == 0 || strings == NULL) {
        return mud_arena_strdup(arena, "");
    }

    size_t sep_len = separator ? strlen(separator) : 0;

    // Calculate total length
    size_t total_len = 0;
    for (size_t i = 0; i < count; i++) {
        if (strings[i] != NULL) {
            total_len += strlen(strings[i]);
        }
        if (i < count - 1) {
            total_len += sep_len;
        }
    }

    // Allocate
    char* result = mud_arena_alloc(arena, total_len + 1);
    if (result == NULL) {
        return NULL;
    }

    // Build result
    char* dest = result;
    for (size_t i = 0; i < count; i++) {
        if (strings[i] != NULL) {
            size_t len = strlen(strings[i]);
            memcpy(dest, strings[i], len);
            dest += len;
        }
        if (i < count - 1 && sep_len > 0) {
            memcpy(dest, separator, sep_len);
            dest += sep_len;
        }
    }

    *dest = '\0';   // Null terminate
    return result;
}

// Substring Extraction
char* mud_arena_substr(MudArena* arena, const char* str, size_t start, size_t len) {
    if (arena == NULL || str == NULL) {
        return NULL;
    }

    size_t str_len = strlen(str);

    // Clamp start
    if (start >= str_len) {
        return mud_arena_strdup(arena, "");
    }

    // Clamp length
    size_t available = str_len - start;
    if (len > available) {
        len = available;
    }

    char* result = mud_arena_alloc(arena, len + 1);
    if (result == NULL) {
        return NULL;
    }

    memcpy(result, str + start, len);
    result[len] = '\0';
    return result;
}
