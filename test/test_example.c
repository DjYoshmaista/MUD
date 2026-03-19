#include "mud_testctx.h"
#include <stdio.h>

static void example_test(MudTestCtx* ctx) {
    CHECK(1 == 2);
    CHECK_MSG(3 == 4, "math is feeling rebellious today");
    CHECKF(5 == 6, "expected %d == %d (demo)", 5, 6);

    REQUIRE(0 && "this will fail and request abort");
    /* if runner respects ctx->abort_current_test, it will stop after REQUEST failure
       if you still execute here, you can also self-stop the test */
    if (ctx->abort_current_test) return;

    CHECK(7 == 8); /* should not run if aborted */
}

int main(void) {
    MudTestCtx ctx;
    mud_testctx_init(&ctx);

    mud_testctx_begin(&ctx, "example_test");
    example_test(&ctx);

    mud_testctx_print_failures(stdout, &ctx);

    /* return non-zero if any failures occurred */
    return (ctx.assertions_failed != 0) ? 1 : 0;
}
