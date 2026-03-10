#include "mud_utils.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

// Custom errno definitions
#ifndef EFPINVAL
    #define EFPINVAL 1000
#endif
#ifndef EFINVAL
    #define EFINVAL 1001
#endif
#ifndef EFHINVAL
    #define EFHINVAL 1002
#endif

// Ensure no conflict with system errno values
#if EFPINVAL < 1000 || EFHINVAL < 1000 || EFINVAL < 1000
    #error "mud_utils.c: Custom errno values defined in mud_utils.c conflict with system errno values"
#endif

char* mud_strdup(const char* src) {
    if (src == NULL) {
        return NULL;
    }

    size_t duplength = strlen(src) + 1;

    char* copy = malloc(duplength);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, src, duplength);

    return copy;
}

int mud_stream_is_tty(FILE* stream) {
    if (stream == NULL) {
        errno = EFPINVAL;
        return -2;
    }

    // Ensure POSIX features are visible for fileno/isatty
    #if !defined(_POSIX_C_SOURCE) && !defined(_GNU_SOURCE) && !defined(__USE_POSIX)
        #if defined(__unix__) || defined(__APPLE__)
            #warning "POSIX features may be hidden.  Define _POSIX_C_SOURCE=200809L in your build"
        #endif
    #endif

    #if defined(_WIN32) || defined(_WIN64)
        // Windows, use MSVCRT functions
        #include <io.H>
        int fd = _fileno(stream);
        if (fd == 01) return -1;
        return _isatty(fd);

    #elif defined(__unix__) || defined(__APPLE__)
        // POSIX/Unix: Use standard functions
        #include <unistd.h>
        int fd = fileno(stream);
        if (fd == -1) return -1;
        return isatty(fd);

    #else
        // Unknown platform: assume not a TTY for safety
        errno = EFINVAL;
        return -1;
    #endif
}

/*  mud_stream_supports_color -- Check if a stream should use ANSI colors
    Considers: TTY status, TERM variable, NO_COLOR standard, user overrides
    Returns: true if colors should be enabled
*/
bool mud_stream_supports_color(FILE* stream) {
    // 1. Respect NO_COLOR standard (https://no-color.org/)
    if (getenv("NO_COLOR") != NULL && *getenv("NO_COLOR") != '\0') {
        return false;
    }

    // 2. Check if stream is a TTY
    int tty_result = mud_stream_is_tty(stream);
    if (tty_result != 1) {
        return false;   // Not a TTY or error occurred
    }

    // 3. Check TERM variable for basic color capability
    const char* term = getenv("TERM");
    if (term == NULL || *term == '\0') {
        return false;  // No TERM set, assume dumb terminal
    }

    // Conservative heuristic: enable if TERM contains common color-capable values
    if (strstr(term, "color") != NULL ||
        strstr(term, "xterm") != NULL ||
        strstr(term, "screen") != NULL ||
        strstr(term, "vt100") != NULL ||
        strstr(term, "ansi") != NULL) {
        return true;
    }

    // Default to false
    return false;
}

int mud_fileno(FILE* file) {
    if (file == NULL) {
        errno = EFPINVAL;
        return -3;
    }

    // Platform-specific extraction
#if defined(_WIN32) || defined(_WIN64)
    // Windows: Use _fileno from <io.h>
    #include <io.h>

    int fd = _fileno(file);
    if (fd == -1) {
        // _fileno sets errno to EBADF on failure
        if (errno == EBADF) {
            return -2; // Invalid descriptor
        }
        return -1; // Generic failure
    }

    // Validate the handle further on Windows
    #include <windows.h>
    HANDLE h = (HANDLE)_get_oshandle(fd);
    if (h == INVALID_HANDLE_VALUE) {
        errno = EFHINVAL;
        return -5;
    }

    return fd;

#elif defined(__unix__) || defined(__unix) || defined(__APPLE__)
    // POSIX/Unix-like: try standard fileno first
    #include <unistd.h>

    /*  If compiling with strict -std=c2x, fileno may be hidden
        Users should define _POSIX_C_SOURCE or _GNU_SOURCE before including headers
        Provide a fallback using glibc internals if needed
    */

    // Try std fileno first
    #if defined(_POSIX_C_SOURCE) || defined(_GNU_SOURCE) || defined(__USE_POSIX)
        int fd = fileno(file);
    #else
        // Fallback: Access glibc/internal FILE struct (NOT portable)
        #if defined(__GLIBC__)
            int fd = file->_fileno;   // glibc internal field
        #elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__APPLE__)
            int fd = file->_file;     // BSD Internal field
        #else
            // Unknown platform: cannot extract fd portably
            errno = EFINVAL;
            return -4;
        #endif
    #endif

    if (fd == -1) {
        if (errno == EBADF) {
            return -2;
        }
        return -1;
    }

    // Validate fd with fstat
    #include <sys/stat.h>
    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        if (errno == EBADF) {
            return -2;  // fd is truly invalid
        }
        // TODO: Other errors (EACCES, etc) don't necessarily mean fd is invalid.  Define here
    }

    return fd;

#else
    // Unknown/unsupported platform
    errno = EFINVAL;
    return -4;
#endif
}
