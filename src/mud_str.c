#include "mud_str.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

static const char* skip_space(const char* str) {
    while (isspace((unsigned char)*str)) {
        str++;
    }
    return str;
}

static bool is_at_end_after_space(const char* str) {
    str = skip_space(str);
    return *str == '\0';
}

static bool equals_trimmed_nocase(const char* str, const char* word) {
    if (str == NULL || word == NULL) {
        return false;
    }

    str = skip_space(str);

    while (*str != '\0' && *word != '\0') {
        if (tolower((unsigned char)*str) != tolower((unsigned char)*word)) {
            return false;
        }
        str++;
        word++;
    }

    return *word == '\0' && is_at_end_after_space(str);
}

/* =============================================================================
 - String View Implementation
=============================================================================*/
MudStrView mud_strview_from_cstr(const char* str) {
    if (str == NULL) {
        return MUD_STRVIEW_EMPTY;
    }
    return (MudStrView){.data = str, .len = strlen(str)};
}

MudStrView mud_strview_from_parts(const char* data, size_t len) {
    if (data == NULL) {
        return MUD_STRVIEW_EMPTY;
    }
    return (MudStrView){.data = data, .len = len};
}

MudStrView mud_strview_substr(MudStrView sv, size_t start, size_t len) {
    if (sv.data == NULL || start >= sv.len) {
        return MUD_STRVIEW_EMPTY;
    }

    size_t max_len = sv.len - start;
    if (len > max_len) {
        len = max_len;
    }

    return (MudStrView){.data = sv.data + start, .len = len};
}

/* =============================================================================
 - String View Comparison
=============================================================================*/
bool mud_strview_equals(MudStrView a, MudStrView b) {
    if (a.len != b.len) {
        return false;
    }
    if (a.len == 0) {
        return true;  // Both empty
    }
    if (a.data == b.data) {
        return true;  // Same pointer
    }
    return memcmp(a.data, b.data, a.len) == 0;
}

bool mud_strview_equals_cstr(MudStrView sv, const char* str) {
    if (str == NULL) {
        return sv.len == 0;
    }
    size_t str_len = strlen(str);
    if (sv.len != str_len) {
        return false;
    }
    return memcmp(sv.data, str, sv.len) == 0;
}

/* =============================================================================
 - Prefix/Suffix Checking
=============================================================================*/
bool mud_strview_starts_with(MudStrView sv, MudStrView prefix) {
    if (prefix.len > sv.len) {
        return false;
    }
    return memcmp(sv.data, prefix.data, prefix.len) == 0;
}

bool mud_strview_ends_with(MudStrView sv, MudStrView suffix) {
    if (suffix.len > sv.len) {
        return false;
    }
    return memcmp(sv.data + sv.len - suffix.len, suffix.data, suffix.len) == 0;
}

/* =============================================================================
 - Trimming
=============================================================================*/
MudStrView mud_strview_trim_left(MudStrView sv) {
    while (sv.len > 0 && isspace((unsigned char)sv.data[0])) {
        sv.data++;
        sv.len--;
    }
    return sv;
}

MudStrView mud_strview_trim_right(MudStrView sv) {
    while (sv.len > 0 && isspace((unsigned char)sv.data[sv.len - 1])) {
        sv.len--;
    }
    return sv;
}

MudStrView mud_strview_trim(MudStrView sv) {
    return mud_strview_trim_right(mud_strview_trim_left(sv));
}

/* =============================================================================
 - Safe String Copy
=============================================================================*/
size_t mud_str_copy(char* dest, size_t dest_size, const char* src) {
    if (dest == NULL || dest_size == 0) {
        return 0;
    }

    if (src == NULL) {
        dest[0] = '\0';
        return 0;
    }

    size_t src_len = strlen(src);
    size_t copy_len = (src_len < dest_size - 1) ? src_len : (dest_size - 1);

    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';

    return copy_len;
}

/* =============================================================================
 - Safe Concatenation
=============================================================================*/
size_t mud_str_concat(char* dest, size_t dest_size, const char* src) {
    if (dest == NULL || dest_size == 0) {
        return 0;
    }

    size_t dest_len = strlen(dest);
    if (dest_len >= dest_size - 1) {
        return 0;
    }

    size_t remaining = dest_size - dest_len - 1;
    return mud_str_copy(dest + dest_len, remaining + 1, src);
}

/* =============================================================================
 - Comparison Functions
=============================================================================*/
int mud_str_compare(const char* a, const char* b) {
    if (a == b) return 0;
    if (a == NULL) return -1;
    if (b == NULL) return 1;
    return strcmp(a, b);
}

int mud_str_compare_nocase(const char* a, const char* b) {
    if (a == b) return 0;
    if (a == NULL) return -1;
    if (b == NULL) return 1;

    while (*a && *b) {
        int diff = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (diff != 0) return diff;
        a++;
        b++;
    }

    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

/* =============================================================================
 - Parsing Functions
=============================================================================*/
bool mud_str_to_int(const char* str, int* out) {
    if (str == NULL || out == NULL) {
        return false;
    }

    str = skip_space(str);
    if (*str == '\0') {
        return false;
    }

    char* end = NULL;
    errno = 0;
    long val = strtol(str, &end, 10);
    if (errno == ERANGE || val < INT_MIN || val > INT_MAX) {
        return false;  // Overflow
    }

    if (end == str || !is_at_end_after_space(end)) {
        return false;  // No conversion or trailing garbage
    }

    *out = (int)val;
    return true;
}

bool mud_str_to_long(const char* str, long* out) {
    if (str == NULL || out == NULL) {
        return false;
    }

    str = skip_space(str);
    if (*str == '\0') {
        return false;
    }

    char* end = NULL;
    errno = 0;
    long val = strtol(str, &end, 10);

    if (errno == ERANGE || end == str || !is_at_end_after_space(end)) {
        return false;
    }

    *out = val;
    return true;
}

bool mud_str_to_double(const char* str, double* out) {
    if (str == NULL || out == NULL) return false;
    str = skip_space(str);
    if (*str == '\0') return false;

    char* end = NULL;
    errno = 0;
    double val = strtod(str, &end);
    if (end == str || errno == ERANGE) return false;

    if (!is_at_end_after_space(end)) return false;

    *out = val;
    return true;
}

bool mud_str_to_bool(const char* str, bool* out) {
    if (str == NULL || out == NULL) {
        return false;
    }

    if (equals_trimmed_nocase(str, "true") ||
        equals_trimmed_nocase(str, "yes") ||
        equals_trimmed_nocase(str, "on") ||
        equals_trimmed_nocase(str, "1")) {
        *out = true;
        return true;
    }

    if (equals_trimmed_nocase(str, "false") ||
        equals_trimmed_nocase(str, "no") ||
        equals_trimmed_nocase(str, "off") ||
        equals_trimmed_nocase(str, "0")) {
        *out = false;
        return true;
    }

    return false;
}

/* =============================================================================
 - String Testing
=============================================================================*/
bool mud_str_is_empty(const char* str) {
    return str == NULL || str[0] == '\0';
}

bool mud_str_is_blank(const char* str) {
    if (str == NULL) {
        return true;
    }

    while (*str) {
        if (!isspace((unsigned char)*str)) {
            return false;
        }
        str++;
    }

    return true;
}

bool mud_str_starts_with(const char* str, const char* prefix) {
    if (str == NULL || prefix == NULL) {
        return false;
    }
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

bool mud_str_ends_with(const char* str, const char* suffix) {
    if (str == NULL || suffix == NULL) {
        return false;
    }

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (suffix_len > str_len) {
        return false;
    }

    return strcmp(str + str_len - suffix_len, suffix) == 0;
}

bool mud_str_contains(const char* str, const char* needle) {
    if (str == NULL || needle == NULL) {
        return false;
    }
    return strstr(str, needle) != NULL;
}
