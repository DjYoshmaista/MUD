#ifndef MUD_LOG_SINK_H
#define MUD_LOG_SINK_H

#include "mud_log.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// Log Record Structure
/*  @brief A single log record passed to sinks */
typedef struct MudLogRecord {
    MudLogLevel level;          // Severity
    const char* timestamp;      // Fomratted time string
    const char* file;           // Source filename
    int line;                   // Source line number
    const char* message;        // Formatted message text
} MudLogRecord;

// Sink interface
/*  @brief Function pointer type for writing a log record */
typedef void (*MudLogSinkWriteFn)(struct MudLogSink* sink, const MudLogRecord* record);

/*  @brief Function pointer type for flushing a sink */
typedef void (*MudLogSinkFlushFn)(struct MudLogSink* sink);

/*  @brief Function pointer type for destroying a sink */
typedef void (*MudLogSinkDestroyFn)(struct MudLogSink* sink);

/*  @brief Base sink structure.  All sink types embed this */
typedef struct MudLogSink {
    MudLogSinkWriteFn write;        // Called for each log message
    MudLogSinkFlushFn flush;        // Force buffered data out.  May be NULL if sink is unbuffered
    MudLogSinkDestroyFn destroy;    // Clean up sink resources.  Called by mud_log_sink_destroy
    MudLogLevel min_level;          // Per-sink filtering.  Sink only receives messages at or above this level
} MudLogSink;

// Generic sink operations
/*  @brief Destoroy a sink and free its resources

    @param sink Sink to destroy (NULL safe)
*/
void mud_log_sink_destroy(MudLogSink* sink); // Wrapper function: calls the sink's `destroy` function pointer

// Console sink
/*  @brief Create a sink that writes to stderr with colors.

    @param min_level Minimum level for this sink
    @return New sink, or NULL on failure
*/
MudLogSink* mud_log_sink_console_create(MudLogLevel min_level);  // Output: colored, human-readable -- Colors: ANSI escape codes.  Makes lvl visually distinct

// File Sink
/*  @brief Create a sink that writes to a file

    @param path File path to write to
    @param min_level Minimum level for this sink
    @param append true to append, false to overwrite
    @return New sink, or NULL on failure
*/
MudLogSink* mud_log_sink_file_create(const char* path, MudLogLevel min_level, bool append); // Append mode - usually true for servers; don't lose previous logs on restart

// Callback Sink
/*  @brief User callback type for custom log handling */
typedef void (*MudLogCallbackFn)(const MudLogRecord* record, void* user_data);

/*  @brief Create a sink that calls a user function

    @param callback Function to call for each log message
    @param user_data Opaque pointer passed to callback
    @param min_level Minimum level for this sink
    @return New sink, or NULL on failure
*/
MudLogSink* mud_log_sink_callback_create(MudLogCallbackFn callback, void* user_data, MudLogLevel min_level); // Integration w/ other systems

#ifdef __cplusplus
}
#endif

#endif /* MUD_LOG_SINK_H */
