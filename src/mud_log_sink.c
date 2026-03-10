#include "mud_log_sink.h"
#include "mud_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>  // for isatty() to detect if stderr is a terminal (for colors)

#define MUD_LOG_FILE_SINK "logs/mud.log"

// Custom errno values (local to this file)
#ifndef EFPINVAL
    #define EFPINVAL 1000
#endif
#ifndef EFINVAL
    #define EFINVAL 1001
#endif
#ifndef EFHINVAL
    #define EFHINVAL 1002
#endif

// Generic Destroy
void mud_log_sink_destroy(MudLogSink* sink) {
    if (sink != NULL && sink->destroy != NULL) {
        sink->destroy(sink);
    }
}

// ANSI Color Codes
#define ANSI_RESET      "\x1b[0m"
#define ANSI_GRAY       "\x1b[90m"
#define ANSI_CYAN       "\x1b[36m"
#define ANSI_GREEN      "\x1b[32m"
#define ANSI_YELLOW     "\x1b[33m"
#define ANSI_RED        "\x1b[31m"
#define ANSI_MAGENTA    "\x1b[35m"

static const char* level_colors[] = {
    ANSI_GRAY,      // TRACE
    ANSI_CYAN,      // DEBUG
    ANSI_GREEN,      // INFO
    ANSI_YELLOW,    // WARN
    ANSI_RED,       // ERROR
    ANSI_MAGENTA,   // FATAL
};

// Console sink structure
typedef struct {
    MudLogSink base;
    FILE* stream;               // Actual output stream (stderr, stdout, etc)
    bool use_colors;            // Pre-computed: Whether to use ANSI color codes
    bool is_interactive;        // Pre-computed: Whether stream is an interactive terminal
    bool colors_forced;         // User override via env/config
} ConsoleSink;

// Callback Sink structure
typedef struct {
    MudLogSink base;
    MudLogCallbackFn callback;
    void* user_data;
    MudLogLevel min_level;
} CallbackSink;

// Console Sink: Write
static void console_write(MudLogSink* sink, const MudLogRecord* record) {
    ConsoleSink* console = (ConsoleSink*)sink;
    FILE* out = console->stream;  // Use the stored stream

    if (console->use_colors) {
        fprintf(out, "%s[%s]%s %s%s%s %s:%d: %s\n",
                level_colors[record->level],
                mud_log_level_name(record->level),
                ANSI_RESET,
                ANSI_GRAY,
                record->timestamp,
                ANSI_RESET,
                record->file,
                record->line,
                record->message);
    } else {
        fprintf(out, "[%s] %s %s:%d: %s\n",
                mud_log_level_name(record->level),
                record->timestamp,
                record->file,
                record->line,
                record->message);
    }
}

// Console Sink: Flush
static void console_flush(MudLogSink* sink) {
    ConsoleSink* console = (ConsoleSink*)sink;
    // stream is not owned by this sink -- do not fclose() it; no other heap allocations to free
    fflush(console->stream);  // Force buffered output to be written
}

// Console Sink: Destroy
static void console_destroy(MudLogSink* sink) {
    ConsoleSink* console = (ConsoleSink*)sink;
    // stream is not owned by this sink -- do not fclose() it; no other heap allocations to free
    free(console); // Frees the ConsoleSink allocation
}

// Convenience function to create a sink that writes to stderr (most common)
MudLogSink* mud_log_sink_stderr_create(MudLogLevel min_level) {
    MudLogSink* stderr_sink = mud_log_sink_console_create(stderr, min_level);
    return stderr_sink;
}

// Convenience function to create a sink that writes to stdout
MudLogSink* mud_log_sink_stdout_create(MudLogLevel min_level) {
    MudLogSink* stdout_sink = mud_log_sink_console_create(stdout, min_level);
    return stdout_sink;
}

// Console Sink: Create
MudLogSink* mud_log_sink_console_create(FILE* stream, MudLogLevel min_level) {
    if (stream == NULL) {
        errno = EFPINVAL;
        return NULL;
    }

    ConsoleSink* sink = malloc(sizeof(ConsoleSink));
    if (sink == NULL) {
        return NULL;
    }

    // Initialize base functionality
    sink->base.write = console_write;
    sink->base.flush = console_flush;
    sink->base.destroy = console_destroy;
    sink->base.min_level = min_level;

    // Store the stream
    sink->stream = stream;

    // Pre-compute capabilities
    sink->is_interactive = (mud_stream_is_tty(stream) == 1);
    sink->use_colors = mud_stream_supports_color(stream);

    // Allow runtime override via environment
    if (getenv("MUD_LOG_FORCE_COLOR") != NULL) {
        sink->use_colors = true;
        sink->colors_forced = true;
    }
    if (getenv("MUD_LOG_NO_COLOR") != NULL) {
        sink->use_colors = false;
        sink->colors_forced = true;
    }
    
    MUD_LOG_DEBUG("Console Sink created: stream=%p, colors=%s, interactive=%s\n",
                  (void*)stream,
                  sink->use_colors ? "enabled" : "disabled",
                  sink->is_interactive ? "yes" : "no");

    return &sink->base;
}

// File sink structure
typedef struct {
    MudLogSink base;
    FILE* file;
    char* path;
} FileSink;

// File Sink: Write
static void file_write(MudLogSink* sink, const MudLogRecord* record) {
    FileSink* file_sink = (FileSink*)sink;

    if (file_sink->file == NULL) {
        return;
    }

    fprintf(file_sink->file, "[%s] %s %s:%d: %s\n",
            mud_log_level_name(record->level),
            record->timestamp,
            record->file,
            record->line,
            record->message);
}

// File Sink: Flush
static void file_flush(MudLogSink* sink) {
    FileSink* file_sink = (FileSink*)sink;
    if (file_sink->file != NULL) {
        fflush(file_sink->file);
    }
}

// File Sink: Destroy
static void file_destroy(MudLogSink* sink) {
    FileSink* file_sink = (FileSink*)sink;

    if (file_sink->file != NULL) {
        fflush(file_sink->file);
        fclose(file_sink->file);
    }

    free(file_sink->path);
    free(file_sink);
}

// File Sink: Create
MudLogSink* mud_log_sink_file_create(const char* path, MudLogLevel min_level, bool append) {
    if (path == NULL) {
        return NULL;
    }

    FileSink* sink = malloc(sizeof(FileSink));
    if (sink == NULL) {
        return NULL;
    }

    sink->path = mud_strdup(path);
    if (sink->path == NULL) {
        free(sink);
        return NULL;
    }

    sink->file = fopen(path, append ? "a" : "w");
    if (sink->file == NULL) {
        free(sink->path);
        free(sink);
        return NULL;
    }

    sink->base.write = file_write;
    sink->base.flush = file_flush;
    sink->base.destroy = file_destroy;
    sink->base.min_level = min_level;

    return &sink->base;
}

// Callback Sink Structure -- Stores callback and context
// Write
static void callback_write(MudLogSink* sink, const MudLogRecord* record) {
    CallbackSink* cb_sink = (CallbackSink*)sink;

    if (cb_sink->callback != NULL) {
        cb_sink->callback(record, cb_sink->user_data);
    }
}

// Flush/Destroy
static void callback_flush(MudLogSink* sink) {
    (void)sink; // Callbacks are unbuffered
}

static void callback_destroy(MudLogSink* sink) {
    // Don't ever free user_data (caller owns it)
    free(sink);
}

// Create
MudLogSink* mud_log_sink_callback_create(MudLogCallbackFn callback, void* user_data, MudLogLevel min_level) {
    if (callback == NULL) {
        return NULL;
    }

    CallbackSink* sink = malloc(sizeof(CallbackSink));
    if (sink == NULL) {
        return NULL;
    }

    sink->callback = callback;
    sink->user_data = user_data;

    sink->base.write = callback_write;
    sink->base.flush = callback_flush;
    sink->base.destroy = callback_destroy;
    sink->base.min_level = min_level;

    return &sink->base;
}
