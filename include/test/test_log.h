#ifndef MUD_TEST_LOG_H
#define MUD_TEST_LOG_H

#include "mud_log.h"

/*  @brief Log macros for use within TEST functions.

    These include the test name in the log message for easier debugging
    The 'ctx' variable must be in scope (which it always is in TEST fn's)
*/
#define TEST_LOG_TRACE(fmt, ...)                                  \
    MUD_LOG_TRACE("[%s] " fmt, ctx->current_test_name, ##__VA_ARGS__)

#define TEST_LOG_DEBUG(fmt, ...)                                  \
    MUD_LOG_DEBUG("[%s] " fmt, ctx->current_test_name, ##__VA_ARGS__)

#define TEST_LOG_INFO(fmt, ...)                                   \
    MUD_LOG_INFO("[%s] " fmt, ctx->current_test_name, ##__VA_ARGS__)

#define TEST_LOG_WARN(fmt, ...)                                   \
    MUD_LOG_WARN("[%s] " fmt, ctx->current_test_name, ##__VA_ARGS__)

#define TEST_LOG_ERROR(fmt, ...)                                  \
    MUD_LOG_ERROR("[%s] " fmt, ctx->current_test_name, ##__VA_ARGS__)

#define TEST_LOG_FATAL(fmt, ...)                                  \
    MUD_LOG_FATAL("[%s] " fmt, ctx->current_test_name, ##__VA_ARGS__)

/*  @brief Log an action being performed in a test */
#define TEST_LOG_ACTION(action)                                   \
    MUD_LOG_DEBUG("[%s] Action: %s", ctx->current_test_name, action)

/*  @brief Log a checkpoint in a test */
#define TEST_LOG_CHECKPOINT(name)                                 \
    MUD_LOG_TRACE("[%s] Checkpoint: %s", ctx->current_test_name, name)

/* Context aware macro layer for tests */
#define TEST_LOG_TO(sink, level, fmt, ...)                        \
    mud_log_write_to_sink((sink), (level), __FILE__, __LINE__,    \
            "[%s] " fmt, ctx->current_test_name, ##__VA_ARGS__)

#endif /* MUD_TEST_LOG_H */
