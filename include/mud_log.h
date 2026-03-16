/*  @file include/mud_log.h
    Levels:
    level       purpose                          #            example
    ---------------------------------------------------------------------------
    TRACE       Detailed Execution Flow          0           "Entering function x"
    DEBUG       Development Diagnostics          1           "Player inventory: [sword, shield]"
    INFO        Normal Operations                2           "Player 'yosh' has logged in"
    WARN        Recoverable Issues               3           "Connection timeout, retrying..."
    ERROR       Failures Needing Attention       4           "Database Write Failed!"
    FATAL       Unrecoverable, shutdown          5           "Out of Memory, terminating..."

    Output Destinations (sinks):
    logs go to:
    - Console (stderr): Development, immediate visibility
    - File: Persistent storage, rotation
    - Syslog: System integration on Unix
    - Custom: Game-specific (admin channel, etc)

    Thread Safety - Logging must:
    - Not corrupt output when multiple threads log
    - Not block threads waiting to log
    - Handle concurrent sink access
*/
#ifndef MUD_LOG_H
#define MUD_LOG_H

#include "mud_log_types.h"
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum MudLogLevel {
    MUD_LOG_TRACE = 0,
    MUD_LOG_DEBUG = 1,
    MUD_LOG_INFO = 2,
    MUD_LOG_WARN = 3,
    MUD_LOG_ERROR = 4,
    MUD_LOG_FATAL = 5,

    MUD_LOG_LEVEL_COUNT
}

// Level Name Conversion
/*  @brief Get the name of a log level

    @param level Log level
    @return String name (e.g., "INFO", "ERROR")
*/
const char* mud_log_level_name(MudLogLevel level);

/*  @brief Parse a level name to enum value

    @param name Level name (case insensitive)
    @param out_level Output level value
    @return true if valid level name, false otherwise
*/
bool mud_log_level_parse(const char* name, MudLogLevel* out_level);

// Global Configuration
/*  @brief Set the minimum log level

    @param level Minimum level to log

    Default is MUD_LOG_INFO for release builds, MUD_LOG_DEBUG for debug
*/
void mud_log_set_level(MudLogLevel level);

/*  @brief Get the current minimum log level

    @return Current minimum level
    Most apps have one log config; per-logger levels add complexity
    Compile-time configurable.  Debug builds are more verbose
*/
MudLogLevel mud_log_get_level(void);

// Core Logging Functions
/*  @brief Log a message at the specified level

    @param level Severity level
    @param file Source file name (__FILE__)
    @param line Source line number (__LINE__)
    @param fmt Format string (printf-style)
    @param ... Format arguments
*/
void mud_log_write(MudLogLevel level, const char* file, int line, const char* fmt, ...);

// @brief Log a message (va_list version)
void mud_log_writev(MudLogLevel level, const char* file, int line, const char* fmt, va_list args);

// Convenience Macros
/*  @brief Log macros that capture source location automatically
    USAGE: MUD_LOG_INFO("Player %s connected", player_name);
*/
#define MUD_LOG_TRACE(...)                                                  \
    mud_log_write(MUD_LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)

#define MUD_LOG_DEBUG(...)                                                  \
    mud_log_write(MUD_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

#define MUD_LOG_INFO(...)                                                   \
    mud_log_write(MUD_LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)

#define MUD_LOG_WARN(...)                                                   \
    mud_log_write(MUD_LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)

#define MUD_LOG_ERROR(...)                                                  \
    mud_log_write(MUD_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

#define MUD_LOG_FATAL(...)                                                  \
    mud_log_write(MUD_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

// Conditional Logging (compile-time elimination)
/*  @brief Compile-time log level threshold

    Logs below this level are completely removed from compiled code
    Define MUD_LOG_COMPILE_LEVEL before including this header to override
*/
#ifndef MUD_LOG_COMPILE_LEVEL
    #ifdef NDEBUG
        #define MUD_LOG_COMPILE_LEVEL MUD_LOG_INFO
    #else
        #define MUD_LOG_COMPILE_LEVEL MUD_LOG_TRACE
    #endif
#endif

/*  @brief Conditional log macros that can be compiled out
    These check at compile time and generate no code if level is too low
*/
#define MUD_LOG_IF(level, ...)                                              \
    do {                                                                    \
        if ((level) >= MUD_LOG_COMPILE_LEVEL) {                             \
            mud_log_write((level), __FILE__, __LINE__, __VA_ARGS__);        \
        }                                                                   \
    } while (0)

// Initialization and Shutdown
/*  @brief Initialize the logging system

    @return ture on success, false on failure

    Must be called before any logging.  Sets up default console sink
*/
bool mud_log_init(void);

/*  @brief Shutdown the logging system

    Flushes all sinks and releases resources
    Safe to call multiple times
*/
void mud_log_shutdown(void);

// Sink Management (Forward Declaration) -- Full definition in mud_log_sink.h
typedef struct MudLogSink MudLogSink;

/*  @brief Add a sink to receive log messages

    @param sink Sink to add (logging system takes ownership)
    @return true on succes
*/
bool mud_log_add_sink(MudLogSink* sink);

/*  @brief Remove all sinks */
void mud_log_clear_sinks(void);

/*  @brief Flush all sinks (write buffered data) */
void mud_log_flush(void);

/*  @brief Write a message to a sink

    @param sink Sink to write to
    @param level Severity level of message
    @param file Source file name (__FILE__)
    @param line Source line number (__LINE__)
    @param fmt Format string (printf-style)
    @param .. Format arguments
    @return true on success, false on failure
*/
void mud_log_write_to_sink(MudLogSink* sink, MudLogLevel level, const char* file, int line, const char* fmt, ...);


#ifdef __cplusplus
}
#endif

#endif /* MUD_LOG_H */
