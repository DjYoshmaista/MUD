#ifndef MUD_STRDUP_H
#define MUD_STRDUP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*  strdup replacement for C source, header and other files
    mud_strdup
    Portable replacement for POSIX strdup

    Behavior:
        - Allocates a duplicate copy of a string
        - Caller owns returned memory
        Must be freed using free()

    Returns:
        - Newly allocated string copy, or NULL on failure
*/

char* mud_strdup(const char* str);

#ifdef __cplusplus
}
#endif

#endif /* MUD_UTILS_H */
