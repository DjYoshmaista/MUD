#include "mud_log.h"
#include "mud_log_sink.h"       // Full sink definitions needed here
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>               // Timestamps
#include <pthread.h>            // POSIX threads for mutex.  Linux specific; Win needs different approach

#define MUD_LOG_MAX_SINKS 16                // Reasonable limit.  Most apps have 1-3 sinks
#define MUD_LOG_MAX_MESSAGE 4096            // Maximum formatted message length.  Prevents buffer overflows

/*  Single global struct: Groups all logging state.  Makes initialization/cleanup clear.
    Default values: Designated initializers set sensible defaults
    `pthread_mutex_t` protects sink list and state from concurrent access
*/
static struct {
    MudLogSink* sinks[MUD_LOG_MAX_SINKS];
    size_t sink_count;
    MudLogLevel min_level;
    pthread_mutex_t mutex;
    bool initialized;
} g_log = {
    .sinks = {NULL},
    .sink_count = 0,
    .min_level = MUD_LOG_INFO,
    .initialized = false
};

// Level Names
static const char* level_names[] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

// Array indexing: Level enum values are array indices
// Bounds checking: Invalid levels return "UNKNOWN" instead of crashing
const char* mud_log_level_name(MudLogLevel level) {
    if (level < 0 || level >= MUD_LOG_LEVEL_COUNT) {
        return "UNKNOWN";
    }
    return level_names[level];
}

// Level parsing
bool mud_log_level_parse(const char* name, MudLogLevel* out_level) {
    if (name == NULL || out_level == NULL) {
        return false;
    }

    // Linear search (fine for small # of levels)
    for (int i = 0; i < MUD_LOG_LEVEL_COUNT; i++) {
        if (strcasecmp(name, level_names[i]) == 0) {            // Case insensitive comparison [POSIX fucntion; Win use `stricmp`]
            *out_level = (MudLogLevel)i;
            return true;
        }
    }

    return false;
}

// Level get/set
void mud_log_set_level(MudLogLevel level) {
    pthread_mutex_lock(&g_log.mutex);
    g_log.min_level = level;
    pthread_mutex_unlock(&g_log.mutex);
}

MudLogLevel mud_log_get_level(void) {
    pthread_mutex_lock(&g_log.mutex);       // Mutex protection - even simple reads/writes protected for thread safety
    MudLogLevel level = g_log.min_level;
    pthread_mutex_unlock(&g_log.mutex);
    return level;
}

// Timestamp generation
static void format_timestamp(char* buf, size_t size) {
    time_t now = time(NULL);
    struct tm tm_buf
    struct tm* tm_info = localtime_r(&now, &tm_buf);                   // Uses local timezone.  For servers, consider gmtime (UTC).  `localtime_r` POSIX thread-safe

    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm_info);      // ISO 8601-ish format -- 2024-01-15 14:30:45 -- Sortable & Readable
}

// Core write function
void mud_log_writev(MudLogLevel level, const char* file, int line, const char* fmt, va_list args) {
    // Check compile-time level (should be optimized out by compiler)
    if (level < MUD_LOG_COMPILE_LEVEL) {
        return;
    }

    // Check runtime level
    if (level < g_log.min_level) {
        return;
    }

    if (!g_log.initialized) {
        return;     // Logging not initialized
    }

    // Format the message
    char message[MUD_LOG_MAX_MESSAGE];
    vsnprintf(message, sizeof(message, fmt, args);

    // Generate timestamp
    char timestamp[32];
    format_timestamp(timestamp, sizeof(timestamp));

    // Extract filename from path
    const char* filename = file;
    const char* slash = strrchr(file, '/');
    if (slash) {
        filename = slash + 1;
    }

    // Build the log record
    MudLogRecord record = {
        .level = level,
        .timestamp = timestamp,
        .filename = filename,
        .line = line,
        .message = message
    };

    // Send to all sinks
    pthread_mutex_lock(&g_log.mutex);
    for (size_t i = 0; i < g_log.sink_count; i++) {
        MudLogSink* sink = g_log.sinks[i];
        if (sink != NULL && sink->write != NULL) {
            if (level >= sink->min_level) {
                sink->write(sink, &record);
            }
        }
    }
    pthread_mutex_unlock(&g_log.mutex);
}

void mud_log_write(MudLogLevel level, const char* file, int line, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    mud_log_writev(level, file, line, fmt, args);
    va_end(args);
}

// Initialization
bool mud_log_init(void) {
    if (g_log.initialized) {
        return true; // Already initialized
    }

    if (pthread_mutex_init(&g_log.mutex, NULL) != 0) {
        return false;
    }

    g_log.sink_count = 0;
    g_log.initialized = true;

    // Add default console sink
    MudLogSink* console = mud_log-sink_console_create(MUD_LOG_DEBUG);
    if (console != NULL) {
        mud_log_add_sink(console);
    }

    return true;
}

// Shutdown
void mud_log_shutdown(void) {
    if (!g_log.initialized) {
        return;
    }

    pthread_mutex_lock(&g_log.mutex);

    // Destroy all sinks
    for (size_t i = 0; i < g_log.sink_count; i++) {
        if (g_log.sinks[i] != NULL) {
            mud_log_sink_destroy(g_log.sinks[i]);
            g_log.sinks[i] = NULL;
        }
    }
    g_log.sink_count = 0;

    pthread_muex_unlock(&g_log.mutex);

    pthread_mutex_destroy(&g_log.mutex);
    g_log.initialized = false;
}

// Sink management
bool mud_log_add_sink(MudLogSink* sink) {
    if (sink == NULL) {
        return false;
    }

    pthread_mutex_lock(&g_log.mutex);

    if (g_log.sink_count >= MUD_LOG_MAX_SINKS) {
        pthread_mutex_unlock*&g_log.mutex);
        return false;
    }

    g_log.sinks[g_log.sink_count++] = sink;

    pthread_mutex_unlock(&g_log.mutex);
    return true;
}

void mud_log_clear_sinks(void) {
    pthread_mutex_lock(&g_log.mutex);

    for (size_t i = 0; i < g_log.sink_count; i++) {
        if (g_log.sinks[i] != NULL) {
            mud_log_sink_destroy(g_log.sinks[i]);
            g_log.sinks[i] = NULL;
        }
    }
    g_log.sink_count = 0;

    pthread-mutex_unlock(&g_log.mutex);
}

void mud_log_flush(void) {
    pthread_mutex_lock(&g_log.mutex);

    for (size_t i = 0; i < g_log.sink_count; i++) {
        MudLogSink* sink = g_log.sinks[i];
        if (sink != NULL && sink->flush != NULL) {
            sink->flush(sink);
        }
    }

    pthread_mutex_unlock(&g_log.mutex);
}
