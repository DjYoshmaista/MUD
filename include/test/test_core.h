#ifndef MUD_TEST_CORE_H
#define MUD_TEST_CORE_H

#include "mud_testctx.h"
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*MudTestFn)(MudTestCtx* ctx);

typedef struct MudTestInfo {
    const char* name;
    const char* file;
    int line;
    MudTestFn func;
    const char* tags;
} MudTestInfo;

#ifndef MUD_MAX_TESTS
#define MUD_MAX_TESTS 2048
#endif

typedef struct MudTestRegistry {
    MudTestInfo tests[MUD_MAX_TESTS];
    size_t count;
} MudTestRegistry;

static inline MudTestRegistry* mud_test_get_registry(void) {
    static MudTestRegistry registry = {0};
    return &registry;
}

static inline int mud_test_register(MudTestInfo info) {
    MudTestRegistry* reg = mud_test_get_registry();
    if (reg->count >= MUD_MAX_TESTS) {
        return -1;
    }
    reg->tests[reg->count] = info;
    reg->count++;
    return 0;
}

#define CHECK_STR_EQ(ctx, actual, expected)                                     \
    do {                                                                        \
        const char* _a = {actual};                                              \
        const char* _e = {expected};                                            \
        if (_a == NULL || _e == NULL) {                                         \
            if (_a != _e) {                                                     \
                mud_testctx_record_failuref((ctx), MUD_TEST_SEV_CHECK,          \
                    __FILE__, __LINE__,                                         \
                    "CHECK_STR_EQ(" #actual ", " #expected ")",                 \
                    "NULL mismatch: actual=%p, expected=%p",                    \
                    (void*)_a, (void*)_e);                                      \
            }                                                                   \
        } else if (strcmp(_a, _e) != 0) {                                       \
        mud_testctx_record_failuref((ctx), MUD_TEST_SEV_CHECK,                  \
        __FILE__, __LINE__,                                                     \
        "CHECK_STR_EQ(" #actual ", " #expected ")",                             \
        "strings differ: actual=\%s\", expected=\"%s\"",                        \
        _a, _e);                                                                \
        }                                                                       \
    } while (0)

#define CHECK_INT_EQ(ctx, actual, expected)                                     \
    do {                                                                        \
        long long _a = (long long)(actual);                                     \
        long long _e = (long long)(expected);                                   \
        if (_a != _e) {                                                         \
            mud_testctx_record_failuref((ctx), MUD_TEST_SEV_CHECK,              \
            __FILE__, __LINE__,                                                 \
            "CHECK_INT_EQ(" #actual ", " #expected ")",                         \
            "values differ: actual=%lld, expected=%lld", _a, _e);               \
        }                                                                       \
    } while (0)

#define REQUIRE_STR_EQ(ctx, actual, expected)                                   \
    do {                                                                        \
        const char* _a = (actual);                                              \
        const char* _e = (expected);                                            \
        if (_a == NULL || _e == NULL) {                                         \
            if (_a != _e) {                                                     \
                mud_testctx_record_failuref((ctx), MUD_TEST_SEV_REQUIRE,        \
                    __FILE__, __LINE__,                                         \
                    "REQUIRE_STR_EQ(" #actual ", " #expected ")",               \
                    "NULL mismatch: actual=%p, expected=%p",                    \
                    (void*)_a, (void*)_e);                                      \
            } else if (strcmp(_a, _e) != 0) {                                   \
            mud_testctx_record_failuref((ctx), MUD_TEST_SEV_REQUIRE,            \
            __FILE__, __LINE__,                                                 \
            "REQUIRE_STR_EQ(" #actual ", " #expected ")",                       \
            "strings differ: actual=\%s\", expected=\"%s\"",                    \
            _a, _e);                                                            \
            }                                                                   \
        }                                                                       \
    } while (0)

#define CHECK_PTR_EQ(ctx, actual, expected)                                     \
    do {                                                                        \
        const void* _a = (actual);                                              \
        const void* _e = (expected);                                            \
        if (_a != _e) {                                                         \
            mud_testctx_record_failuref((ctx), MUD_TEST_SEV_CHECK,              \
            __FILE__, __LINE__,                                                 \
            "CHECK_PTR_EQ(" #actual ", " #expected ")",                         \
            "pointers differ: actual=%p, expected=%p", _a, _e);                 \
        }                                                                       \
    } while (0)

#define CHECK_NOT_NULL(ctx, ptr)                                                \
    do {                                                                        \
        if ((ptr) == NULL) {                                                    \
            mud_testctx_record_failuref((ctx), MUD_TEST_SEV_CHECK,              \
            __FILE__, __LINE__,                                                 \
            "CHECK_NOT_NULL(" #ptr ")",                                         \
            "pointer is NULL");                                                 \
        }                                                                       \
    } while (0)

#define CHECK_NULL(ctx, ptr)                                                    \
    do {                                                                        \
        if ((ptr) != NULL) {                                                    \
            mud_testctx_record_failuref((ctx), MUD_TEST_SEV_CHECK,              \
            __FILE__, __LINE__,                                                 \
            "CHECK_NULL(" #ptr ")",                                             \
            "pointer is NOT NULL");                                             \
        }                                                                       \
    } while (0)

#define CHECK_MEM_EQ(ctx, actual, expected, size)                               \
    do {                                                                        \
        const void* _a = (actual);                                              \
        const void* _e = (expected);                                            \
        size_t _s = (size);                                                     \
        if (memcmp(_a, _e, _s) != 0) {                                          \
            mud_testctx_record_failuref((ctx), MUD_TEST_SEV_CHECK,              \
            __FILE__, __LINE__,                                                 \
            "CHECK_MEM_EQ(" #actual ", " #expected ", " #size ")",              \
            "memory block differ (size=%zu)", _s);                              \
        }                                                                       \
    } while (0)

#define CHECK_TRUE(ctx, expr) CHECK_CTX(ctx, (expr))
#define CHECK_FALSE(ctx, expr) CHECK_CTX(ctx, !(expr))

#ifdef __cplusplus
}
#endif

#endif /* MUD_TEST_CORE_H */
