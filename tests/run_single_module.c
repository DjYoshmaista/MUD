#include "test/test_autoreg.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    MudTestRegistry* reg = mud_test_get_registry();

    int passed = 0;
    int failed = 0;

    printf("Running %zu tests...\n\n", reg->count);

    clock_t start = clock();
    for (size_t i = 0; i < reg->count; i++) {
	const MudTestInfo* test = &reg->tests[i];
	
	MudTestCtx ctx;
	mud_testctx_init(&ctx);
	mud_testctx_begin(&ctx, test->name);

	test->func(&ctx);

	bool test_passed = (ctx.assertions_failed == 0);

	if (test_passed) {
	    printf("  PASS: %s\n", test->name);
	    passed++;
	} else {
	    printf("  FAIL: %s\n", test->name);
	    failed++;
	}
    }

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    printf("\n");
    printf("================================================================================\n");
    printf("Results: %d passed, %d failed\n", passed, failed);
    printf("Time: %.3f seconds\n", elapsed);
    printf("================================================================================\n");

    return (failed > 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}
