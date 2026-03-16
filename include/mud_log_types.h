#ifndef MUD_LOG_TYPES_H
#define MUD_LOG_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*  A single log record
    @param level Severity level
    @param timestamp Timestamp (ISO 8601-ish)
    @param file Source file name (__FILE__)
    @param line Source line number (__LINE__)
    @param message Formatted message (printf-style)
*/
// Log severity levels, from least to most severe
typedef enum MudLogLevel {
    MUD_LOG_TRACE = 0,
    MUD_LOG_DEBUG = 1,
    MUD_LOG_INFO = 2,
    MUD_LOG_WARN = 3,
    MUD_LOG_ERROR = 4,
    MUD_LOG_FATAL = 5,

    MUD_LOG_LEVEL_COUNT
} MudLogLevel;

typedef struct MudLogRecord {
    MudLogLevel level;
    const char* timestamp;
    const char* file;
    int line;
    const char* message;
} MudLogRecord;

typedef struct MudLogSink MudLogSink;

#ifdef __cplusplus
}
#endif

#endif /* MUD_LOG_TYPES_H */
