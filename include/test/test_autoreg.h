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

#define TEST(name)                                              \
    static void name(MudTestCtx* ctx);                          \
    MUD_TEST_CONSTRUCTOR                                        \
    static void MUD_TEST_UNIEQUE(mud_test_register_)(void) {    \
        MudTestInfo info = {                                    \
            .name = #name,                                      \
            .file = __FILE__,                                   \
            .line = __LINE__,                                   \
            .func = name,                                       \
            .tags = ""                                          \
        };                                                      \
        mud_test_register(info);                                \
    }                                                           \
    static void name(MudTestCtx* ctx)

#define TEST_TAGGED(name, tag_string)                           \
    static void name(MudTestCtx* ctx);                          \
    MUD_TEST_CONSTRUCTOR                                        \
    static void MUD_TEST_UNIQUE(mud_test_register_)(void) {     \
        MudTestInfo info = {                                    \
            .name = #name,                                      \
            .file = __FILE__,                                   \
            .line = __LINE__,                                   \
            .func = name,                                       \
            .tags = tag_string                                  \
        };                                                      \
        mud_test_register(info);                                \
    }                                                           \
    static void name(MudTestCtx* ctx)

#define TEST_DISABLED(name)                                     \
    static void name(MudTestCtx* ctx)

#endif /* MUD_TEST_AUTOREG_H */
