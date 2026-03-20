#ifndef MUD_TEST_LOG_H
#define MUD_TEST_LOG_H

#include "mud_log.h"

/*  @brief Log macros for use within TEST functions.

    These include the test name in the log message for easier debugging
    The 'ctx' variable must be in scope (which it always is in TEST fn's)
*/
#define TEST_LOG_TRACE(fmt, ...)                                  \
    LOG_TEST_TRACE("[%s] " fmt, ctx->current_test_name, ##__VA_ARGS__)

#define TEST_LOG_DEBUG(fmt, ...)                                  \
    LOG_TEST_DEBUG("[%s] " fmt, ctx->current_test_name, ##__VA_ARGS__)

#define TEST_LOG_INFO(fmt, ...)                                   \
    LOG_TEST_INFO("[%s] " fmt, ctx->current_test_name, ##__VA_ARGS__)

#define TEST_LOG_WARN(fmt, ...)                                   \
    LOG_TEST_WARN("[%s] " fmt, ctx->current_test_name, ##__VA_ARGS__)

#define TEST_LOG_ERROR(fmt, ...)                                  \
    LOG_TEST_ERROR("[%s] " fmt, ctx->current_test_name, ##__VA_ARGS__)

#define TEST_LOG_FATAL(fmt, ...)                                  \
    LOG_TEST_FATAL("[%s] " fmt, ctx->current_test_name, ##__VA_ARGS__)

/*  @brief Log an action being performed in a test */
#define TEST_LOG_ACTION(action)                                   \
    LOG_TEST_INFO("[%s] Action: %s", ctx->current_test_name, action)

/*  @brief Log a checkpoint in a test */
#define TEST_LOG_CHECKPOINT(name)                                 \
    LOG_TEST_INFO("[%s] Checkpoint: %s", ctx->current_test_name, name)

/* Context aware macro layer for tests 
#define TEST_LOG_TO(sink, level, fmt, ...)                        \
    mud_log_write_to_sink((sink), (level), __FILE__, __LINE__,    \
            "[%s] " fmt, ctx->current_test_name, ##__VA_ARGS__) */

#endif /* MUD_TEST_LOG_H */
