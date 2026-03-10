#ifndef MUD_STRDUP_H
#define MUD_STRDUP_H

#include <stddef.h>
#include <stdio.h>

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

/*  fileno replacement for C source, header and other files
    mud_fileno
    Portable replacement for POSIX fileno

    WARNING: Extracting file descriptors from FILE* is inherently non-portable.  This function uses platform-specific internals and may fail on some system.s  For maximum portability, consider:
        - Using POSIX fileno() directly w/ _POSIX_C_SOURCE defined
        - Designing API to work with FILE* throughput
        - Passing file descriptors explicitly where needed

    Behavior:
        - Returns the file descriptor of a file
        - Returns -1 on failure
        - Returns -2 if underlying fd is invalid (errno=EBADF)
        - Returns -3 if FILE* pointer is NULL/invalid (errno=EFPINVAL)
        - Returns -4 if FILE object state is invalid (errno=EFINVAL)
        - Returns -5 if underlying OS handle is invalid (errno=EFHINVAL)

    Returns:
        - File descriptor (>=0) on success
        - Negative error code on failure (see above)
        - errno is set to provide additional error information
*/
int mud_fileno(FILE* file);

/*  Check if a stream is a TTY (terminal)
    mud_stream_is_tty
    Portable replacement for POSIX isatty

    Behavior:
        Returns a value dependent on what the stream is.
    Returns:
        - Returns 1 if stream is a TTY
        - Returns 0 if stream is not a TTY
        - Returns -1 on failure (errno=EFPINVAL)
        - Returns -2 if stream is invalid (errno=EFHINVAL)
*/
int mud_stream_is_tty(FILE* stream);

/*  Check if a stream should use ANSI colors
    mud_stream_supports_color
    Portable replacement for POSIX isatty

    Behavior:
        - Returns true if stream should use ANSI colors
        - Returns false if stream should not use ANSI colors

    Returns:
        - Returns true if stream should use ANSI colors
        - Returns false if stream should not use ANSI colors
*/
bool mud_stream_supports_color(FILE* stream);

#ifdef __cplusplus
}
#endif

#endif /* MUD_UTILS_H */
