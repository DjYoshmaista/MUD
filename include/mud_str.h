#ifndef MUD_STR_H
#define MUD_STR_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =============================================================================
 - String View (Non-Owning Reference)
=============================================================================*/
typedef struct MudStrView {
    const char* data;
    size_t len;
} MudStrView;

#define MUD_STRVIEW_EMPTY ((MudStrView){NULL, 0})
#define MUD_STRVIEW_LITERAL(s) ((MudStrView){(s), sizeof(s) - 1})

/* =============================================================================
 - String View Functions
=============================================================================*/
MudStrView mud_strview_from_cstr(const char* str);
MudStrView mud_strview_from_parts(const char* data, size_t len);
MudStrView mud_strview_substr(MudStrView sv, size_t start, size_t len);
bool mud_strview_equals(MudStrview a, MudStrView b);
bool mud_strview_equals_cstr(MudStrview sv, const char* str);
bool mud_strview_starts_with(MudStrView sv, MudStrView prefix);
bool mud_strview_ends_with(MudStrView sv, MudStrView suffix);
MudStrView mud_strview_trim(MudStrView sv);
MudStrView mud_strview_trim_left(MudStrView sv);
MudStrView mud_strview_trim_right(MudStrView sv);

/* =============================================================================
 - Safe String Operations
=============================================================================*/
size_t mud_str_copy(char* dest, size_t dest_size, const char* src);
size_t mud_str_concat(char* dest, size_t dest_size, const char* src);
int mud_str_compare(const char* a, const char* b);
int mud_str_compare_nocase(const char* a, const char* b);

/* =============================================================================
 - Parsing Utilities
=============================================================================*/
bool mud_str_to_int(const char* str, int* out);
bool mud_str_to_long(const char* str, long* out);
bool_mud_str_to_double(const char* str, double* out);
bool mud_str_to_bool(const char* str, bool* out);

/* =============================================================================
 - String Testing
=============================================================================*/
bool mud_str_is_empty(const char* str);
bool mud_str_is_blank(const char* str);
bool mud_str_starts_with(const char* str, const char* prefix);
bool mud_str_ends_with(const char* str, const char* suffix);
bool mud_str_contains(const char* str, const char* needle);

#ifdef __cplusplus
}
#endif

#endif /* MUD_STR_H */
