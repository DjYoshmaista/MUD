#ifndef MUD_TESTCTX_H
#define MUD_TESTCTX_H

/*
    mud_textctx.h -- Minimal failure-accumulation context for a C test framework

    Usage pattern (recc):
	- Each test fucntion signature: void test_fn(MudTestCtx* ctx)
	- Runner does:
	    MudTestCtx ctx;
	    mud_textctx_init(&ctx);
	    mud_testctx_begin(%ctx, "test_name");
	    test_fn(&ctx);
	    // If ctx.abort_current_test != 0 runner may stop early [or test can check it]
	    // Runner prints ctx.failures entries afterwards
*/

#include <stddef.h>	// size_t
#include <stdarg.h>	// va_list
#include <stdint.h>	// uint32_t
#include <stdio.h>	// snprintf
#include <string.h>	// memset, strncpy

#ifdef __cplusplus
extern "C" {
#endif

/*
-------------------------------------------------------------------------------
    Configuration Defaults
-------------------------------------------------------------------------------
*/

#ifndef MUD_TEST_MAX_FAILURES
#define MUD_TEST_MAX_FAILURES 512
#endif

#ifndef MUD_TEST_MSG_MAX
#define MUD_TEST_MSG_MAX 512
#endif

#ifndef MUD_TEST_EXPR_MAX
#define MUD_TEST_EXPR_MAX 512
#endif

#ifndef MUD_MAX_TESTS
#define MUD_MAX_TESTS 2048
#endif

/*
-------------------------------------------------------------------------------
    Severity and Failure Entry
-------------------------------------------------------------------------------
*/

typedef enum MudTestSeverity {
    MUD_TEST_SEV_CHECK = 0,
    MUD_TEST_SEV_REQUIRE = 1,
} MudTestSeverity;

typedef struct MudTestFailure {
    const char* file;			/* __FILE__ */
    int line;				/* __LINE__ */
    MudTestSeverity severity;		/* CHECK vs SEVERITY */

    char expr[MUD_TEST_EXPR_MAX];	/* stringized expression */
    char msg[MUD_TEST_MSG_MAX];		/* message */
} MudTestFailure;

/*
-------------------------------------------------------------------------------
    Test Context
-------------------------------------------------------------------------------
*/

typedef struct MudTestCtx {
    const char* current_test_name;
    
    // Counts
    uint32_t assertions_failed;
    uint32_t total_failures;

    // Failure storage
    MudTestFailure failures[MUD_TEST_MAX_FAILURES];

    // Control flag: when REQUIRE fails, runner should abort current test
    int abort_current_test;
} MudTestCtx;

/* -----------------------------------------------------------------------------
    Core Operations
----------------------------------------------------------------------------- */

// Initialize context (call once per runner instance),
static inline void mud_testctx_init(MudTestCtx* ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
}

// Begin a test: resets per-test counters and sets current test name
static inline void mud_testctx_begin(MudTestCtx* ctx, const char* test_name) {
    if (!ctx) return;
    ctx->current_test_name = test_name;

    ctx->assertions_failed = 0;
    ctx->total_failures = 0;
    ctx->abort_current_test = 0;
}

/* -----------------------------------------------------------------------------
    Record a failure.  Safe: if you exceed MUD_TEST_MAX_FAILURES it still increments
    assertions_failed, but stop storing further entries (still fail-fast in CI if desired).
----------------------------------------------------------------------------- */
static inline void mud_testctx_record_failure(
    MudTestCtx* ctx,
    MudTestSeverity severity,
    const char* file,
    int line,
    const char* expr_str,
    const char* msg_str
) {
    if (!ctx) return;

    ctx->assertions_failed += 1;

    // Require requests abort
    if (severity == MUD_TEST_SEV_REQUIRE) {
	ctx->abort_current_test = 1;
    }

    // Store entry if space remains
    if (ctx->total_failures < (uint32_t)MUD_TEST_MAX_FAILURES) {
	MudTestFailure* f = &ctx->failures[ctx->total_failures];

	f->file = file;
	f->line = line;
	f->severity = severity;

	// Copy expr string (optional)
	if (expr_str && expr_str[0] != '\0') {
	    strncpy(f->expr, expr_str, (size_t)MUD_TEST_EXPR_MAX - 1);
	    f->expr[MUD_TEST_EXPR_MAX - 1] = '\0';
	} else { 
	    f->expr[0] = '\0';
	}

	// Copy msg string
	if (msg_str && msg_str[0] != '\0') {
	    strncpy(f->msg, msg_str, (size_t)MUD_TEST_MSG_MAX - 1);
	    f->msg[MUD_TEST_MSG_MAX - 1] = '\0';
	} else {
	    f->msg[0] = '\0';
	}

	ctx->total_failures += 1;
    }
}

// Convenience: record with a formatted message (stored in entry msg buffer).
static inline void mud_testctx_record_failuref(
    MudTestCtx* ctx,
    MudTestSeverity severity,
    const char* file,
    int line,
    const char* expr_str,
    const char* fmt,
    ...
) {
    if (!ctx) return;

    char buf[MUD_TEST_MSG_MAX];
    buf[0] = '\0';

    if (fmt && fmt[0] != '\0') {
	va_list args;
	va_start(args, fmt);
	(void)vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
    }

    mud_testctx_record_failure(ctx, severity, file, line, expr_str, buf);
}

/* -----------------------------------------------------------------------------
    CHECK / REQUIRE macros
----------------------------------------------------------------------------- */

#ifndef MUD_TEST_STRINGIZE_IMPL
#define MUD_TEST_STRINGIZE_IMPL(x) #x
#endif
#ifndef MUD_TEST_STRINGIZE
#define MUD_TEST_STRINGIZE(x) MUD_TEST_STRINGIZE_IMPL(x)
#endif

/*
    These macros assume a variable named "ctx" is in scope;
	void my_test(MudTestCtx* ctx) { CHECK(1==2); ... }
    If diff name preferred can use the *_CTX variants.
*/

#define CHECK_CTX(_ctx, expr)							\
    do {									\
	if (!(expr)) {								\
	    mud_testctx_record_failure((_ctx), MUD_TEST_SEV_CHECK,		\
		__FILE__, __LINE__, MUD_TEST_STRINGIZE(expr), "CHECK failed");	\
	}									\
    } while (0)

#define REQUIRE_CTX(_ctx, expr)								\
    do {										\
	if (!(expr)) {									\
	    mud_testctx_record_failure((_ctx), MUD_TEST_SEV_REQUIRE,			\
		__FILE__, __LINE__, MUD_TEST_STRINGIZE(expr), "REQUIRE failed");	\
	}										\
    } while (0)

// Default forms assume 'ctx'
#define CHECK(expr) CHECK_CTX(ctx, expr)
#define REQUIRE(expr) REQUIRE_CTX(ctx, expr)

// Optional "message" forms
#define CHECK_MSG_CTX(_ctx, expr, msg_cstr)						\
    do {										\
	if (!(expr)) {									\
	    mud_testctx_record_failure((_ctx), MUD_TEST_SEV_CHECK,			\
		__FILE__, __LINE__, MUD_TEST_STRINGIZE(expr), (msg_cstr));		\
	}										\
    } while (0)

#define REQUIRE_MSG_CTX(_ctx, expr, msg_cstr)						\
    do {										\
	if (!(expr)) {									\
	    mud_testctx_record_failure((_ctx), MUD_TEST_SEV_REQUIRE,			\
		__FILE__, __LINE__, MUD_TEST_STRINGIZE(expr), (msg_cstr));		\
	}										\
    } while (0)

#define CHECK_MSG(expr, msg_cstr) CHECK_MSG_CTX(ctx, expr, msg_cstr)
#define REQUIRE_MSG(expr, msg_cstr) REQUIRE_MSG_CTX(ctx, expr, msg_cstr)

// Optional printf-style forms
#define CHECKF_CTX(_ctx, expr, fmt, ...)						\
    do {										\
	if (!(expr)) {									\
	    mud_testctx_record_failuref((_ctx), MUD_TEST_SEV_CHECK,			\
		__FILE__, __LINE__, MUD_TEST_STRINGIZE(expr), (fmt), ##__VA_ARGS__);	\
	}										\
    } while (0)

#define REQUIREF_CTX(_ctx, expr, fmt, ...)						\
    do {										\
	if (!(expr)) {									\
	    mud_testctx_record_failuref((_ctx), MUD_TEST_SEV_REQUIRE,			\
		__FILE__, __LINE__, MUD_TEST_STRINGIZE(expr), (fmt), ##__VA_ARGS__);	\
	}										\
    } while(0)

#define CHECKF(expr, fmt, ...)		CHECKF_CTX(ctx, expr, fmt, ##__VA_ARGS__)
#define REQUIREF(expr, fmt, ...)	REQUIREF_CTX(ctx, expr, fmt, ##__VA_ARGS__)

/* -----------------------------------------------------------------------------
    Simple printing helper (runner may prefer its own formatting)
----------------------------------------------------------------------------- */

static inline const char* mud_test_severity_str(MudTestSeverity s) {
    return (s == MUD_TEST_SEV_REQUIRE) ? "REQUIRE" : "CHECK";
}

static inline void mud_testctx_print_failures(FILE* out, const MudTestCtx* ctx) {
    if (!out || !ctx) return;

    fprintf(out, "Test: %s\n", ctx->current_test_name ? ctx->current_test_name : "(unnamed)");
    fprintf(out, "Failures stored: %u   Assertions Failed: %u\n", (unsigned)ctx->total_failures, (unsigned)ctx->assertions_failed);

    for (uint32_t i = 0; i < ctx->total_failures; ++i) {
	 const MudTestFailure* f = &ctx->failures[i];

	 fprintf(out, "  [%u] %s at %s:%d\n",
	    (unsigned) (i + 1),
	    mud_test_severity_str(f->severity),
	    f->file ? f->file : "(unknown)",
	    f->line);

	 if (f->expr[0] != '\0') {
	    fprintf(out, " expr: %s\n", f->expr);
	 }
	 if (f->msg[0] != '\0') {
	    fprintf(out, " msg : %s\n", f->msg);
	 }
    }
}

#ifdef __cplusplus
}
#endif

#endif /* MUD_TESTCTX_H */
