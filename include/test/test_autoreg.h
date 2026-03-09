#ifndef MUD_TEST_AUTOREG_H
#define MUD_TEST_AUTOREG_H

#include "test_core.h"

#if defined(__GNUC__) || defined(__clang__)
    #define MUD_TEST_CONSTRUCTOR __attribute__((constructor))
#else
    #error "Automatic test registration requires GCC or Clang"
#endif

#define MUD_TEST_CONCAT_IMPL(a, b) a##b
#define MUD_TEST_CONCAT(a, b) MUD_TEST_CONCAT_IMPL(a, b)
#define MUD_TEST_UNIQUE(prefix) MUD_TEST_CONCAT(prefix, __LINE__)

#define TEST(testname)                                              \
    static void testname(MudTestCtx* ctx);                          \
    MUD_TEST_CONSTRUCTOR                                        \
    static void MUD_TEST_UNIQUE(mud_test_register_)(void) {    \
        MudTestInfo info = {                                    \
            .name = #testname,                                      \
            .file = __FILE__,                                   \
            .line = __LINE__,                                   \
            .func = testname,                                       \
            .tags = ""                                          \
        };                                                      \
        mud_test_register(info);                                \
    }                                                           \
    static void testname(MudTestCtx* ctx)

#define TEST_TAGGED(testtagname, tag_string)                           \
    static void testtagname(MudTestCtx* ctx);                          \
    MUD_TEST_CONSTRUCTOR                                        \
    static void MUD_TEST_UNIQUE(mud_test_register_)(void) {     \
        MudTestInfo info = {                                    \
            .name = #testtagname,                                      \
            .file = __FILE__,                                   \
            .line = __LINE__,                                   \
            .func = testtagname,                                       \
            .tags = tag_string                                  \
        };                                                      \
        mud_test_register(info);                                \
    }                                                           \
    static void testtagname(MudTestCtx* ctx)

#define TEST_DISABLED(disabledname)                                     \
    static void disabledname(MudTestCtx* ctx)

#endif /* MUD_TEST_AUTOREG_H */
