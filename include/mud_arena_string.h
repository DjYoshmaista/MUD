#ifndef MUD_ARENA_STRING_H
#define MUD_ARENA_STRING_H

#include "mud_arena.h"
#include "mud_str.h"
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

//  String Duplication:
/*  @brief Duplicate a string into arena memory.

    @param arena Arena to allocate from
    @param str String to duplicate
    @return New null-terminated string in arena, or NULL on failure

    Returned string is valid until the arena is reset or destroyed
*/
char* mud_arena_strdup(MudArena* arena, const char* str);

/*  @brief Duplicate up to n characters of a string into arena memory.

    @param arena Arena to allocate from
    @param str String to duplicate
    @param n Maximum number of characters to copy
    @return New nul-terminated string in arena, or NULL on failure

    If str is shorter than n, only strlen(str) characters are copied
    The result is always null-terminated
*/
char* mud_arena_strndup(MudArena* arena, const char* str, size_t n);

//  String Formatting:
/*
    @brief Format a string into arena memory (printf-style)

    @param arena Arena to allocate from
    @param fmt Format string
    @param ... Format arguments
    @return Formatted null-terminated string, or NULL on failure
*/
char* mud_arena_sprintf(MudArena* arena, const char* fmt, ...);

/*  @brief Format a string into arena memory (va_list version)

    @param arena Arena to allocate from
    @param fmt Format string
    @param args Format arguments
    @return Formatted null-terminated string, or NULL on failure.
*/
char* mud_arena_vsprintf(MudArena* arena, const char* fmt, va_list args);

//  String Concatenation:
/*  @brief Concatenate multiple strings into arena memory

    @param arena Arena to allocate from
    @param count Number of strings to concatenate
    @param .. String pointers (const char*)
    @return Concatenated null-terminated string, or NULL on failure

    NULL strings in the argument list are treated as empty strings.
*/
char* mud_arena_strcat(MudArena* arena, size_t count, ...);

/*  @brief Join strings with a separator

    @param arena Arena to allocate from
    @param separator String to place between elements
    @param strings Array of strings to join
    @param count Number of strings in array
    @return Joined string, or NULL on failure

    Example: mud_arena_strjoin(arena, ", ", {"a","b","c"}, 3) -> "a, b, c"
*/
char* mud_arena_strjoin(MudArena* arena, const char* separator, const char** strings, size_t count);

//  String View to Owned String
/*  @brief Copy a substring into arena memory

    @param arena Arena to allocate from
    @param str Source string
    @param start Starting offset
    @param len Number of characters to copy
    @return Null-terminated string copy, or NULL on failure
*/
char* mud_arena_strview_to_cstr(MudArena* arena, MudStrView sv);

// String Substring
/*  @brief Copy a substring into arena memory

    @param arena Arena to allocate from
    @param str Source string
    @param start Starting offset
    @param len Number of characters to copy
    @return Null-terminated string copy, or NULL on failure
*/
char* mud_arena_substr(MudArena* arena, const char* str, size_t start, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* MUD_ARENA_STRING_H */
