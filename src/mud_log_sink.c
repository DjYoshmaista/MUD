#include "mud_log_sink.h"
#include "mud_utils.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  // for isatty() to detect if stderr is a terminal (for colors)

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
    ANSI_GREN,      // INFO
    ANSI_YELLOW,    // WARN
    ANSI_RED,       // ERROR
    ANSI_MAGENTA,   // FATAL
};

// Console sink structure
typedef struct {
    MudLogSink base;
    bool use_colors;
} ConsoleSink;

// Console Sink: Write
static void console_write(MudLogSink* sink, const MudLogRecord* record) {
    ConsoleSink* console = (ConsoleSink*)sink;

    if (console->use_colors) {
        fprintf(stderr, "%s[%s]%s %s%s%s %s:%d: %s\n",
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
        fprintf(stderr, "[%s] %s %s:%d: %s\n",
                mud_log_level_name(record->level),
                record->timestamp,
                record->file,
                record->line,
                record->message);
    }
}

// Console Sink: Flush
static void console_flush(MudLogSink* sink) {
    (void)sink; // unused
    fflush(stderr);  // Force buffered output to be written
}

// Console Sink: Destroy
static void console_destroy(MudLogSink* sink) {
    free(sink);
}

// Console Sink: Create
MudLogSink* mud_log_sink_console_create(MudLogLevel min_level) {
    ConsoleSink* sink = malloc(sizeof(ConsoleSink));
    if (sink == NULL) {
        return NULL;
    }

    sink->base.write = console_write;
    sink->base.flush = console_flush;
    sink->base.destroy = console_destroy;
    sink->base.min_level = min_level;

    // Check if stderr is a terminal
    sink->use_colors = isatty(fileno(stderr));

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
