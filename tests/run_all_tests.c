#include "test/test_autoreg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct RunnerConfig {
    const char* filter_name;
    const char* filter_tag;
    int verbose;
    int stop_on_failure;
} RunnerConfig;

static RunnerConfig parse_args(int argc, char* argv[]) {
    RunnerConfig cfg = {
	.filter_name = NULL,
	.filter_tag = NULL,
	.verbose = 0,
	.stop_on_failure = 0
    };

    for (int i = 1; i < argc; i++) {
	if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
	    cfg.verbose = 1;
	}
	else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--stop") == 0) {
	    cfg.stop_on_failure = 1;
	}
	else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--namme") == 0) {
	    if (i + 1 < argc) {
		cfg.filter_name = argv[++i];
	    }
	}
	else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--tag") == 0) {
	    if (i + 1 < argc) {
		cfg.filter_tag = argv[++i];
	    }
	}
	else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
	    printf("Usage; %s [options]\n", argv[0]);
	    printf("  -v, --verbose	Verbose output\n");
	    printf("  -s, --stop	Stop on first failure\n");
	    printf("  -n, --name STR	Only run tests containing STR in name\n");
	    printf("  -t, --tag STR	Only run tests containing STR in tags\n");
	    printf("  -h, --help	Show this help\n");
	    exit(EXIT_SUCCESS);
	}
    }

    return cfg;
}

static int test_matches_filter(const MudTestInfo* test, const RunnerConfig* cfg) {
    if (cfg->filter_name != NULL) {
	if (strstr(test->name, cfg->filter_name) == NULL) {
	    return 0;
	}
    }

    if (cfg->filter_tag != NULL) {
	if (test->tags == NULL || strstr(test->tags, cfg->filter_tag) == NULL) {
	    return 0;
	}
    }

    return 1;
}

static int run_single_test(const MudTestInfo* test, const RunnerConfig* cfg) {
    MudTestCtx ctx;
    mud_testctx_init(&ctx);
    mud_testctx_begin(&ctx, test->name);

    clock_t start = clock();
    test->func(&ctx);
    clock_t end = clock();

    double elapsed_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;

    int passed = (ctx.assertions_failed == 0);

    if (cfg->verbose || !passed) {
	printf("%s %s (%.2f ms)\n",
	    passed ? "PASS" : "FAIL",
	    test->name,
	    elapsed_ms);
	if (!passed) {
	    mud_testctx_print_failures(stdout, &ctx);
	}
    }

    return passed;
}

int main(int argc, char* argv[]) {
    RunnerConfig cfg = parse_args(argc, argv);
    MudTestRegistry* reg = mud_test_get_registry();

    int total = 0;
    int passed = 0;
    int failed = 0;
    int skipped = 0;

    printf("Running tests...\n");
    if (cfg.filter_name) printf("  Name filter: %s\n", cfg.filter_name);
    if (cfg.filter_tag) printf("  Tag filter: %s\n", cfg.filter_tag);
    printf("\n");

    clock_t suite_start = clock();

    for (size_t i = 0; i < reg->count; i++) {
	const MudTestInfo* test = &reg->tests[i];

	if (!test_matches_filter(test, &cfg)) {
	    skipped++;
	    continue;
	}

	total++;

	if (run_single_test(test, &cfg)) {
	    passed++;
	} else {
	    failed++;
	    if (cfg.stop_on_failure) {
		printf("\nStopping on first failure.\n");
		break;
	    }
	}
    }

    clock_t suite_end = clock();
    double suite_elapsed = ((double)(suite_end - suite_start) / CLOCKS_PER_SEC);

    printf("\n");
    printf("================================================================================\n");
    printf("Results: %d passed, %d failed, %d skipped\n", passed, failed, skipped);
    printf("Total time: %.3f seconds\n", suite_elapsed);
    printf("================================================================================\n");

    return (failed > 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}
