#include "test/test_core.h"

MudTestRegistry* mud_test_get_registry(void) {
    static MudTestRegistry registry = {0};
    return &registry;
}

int mud_test_register(MudTestInfo info) {
    MudTestRegistry* reg = mud_test_get_registry();
    if (reg->count >= MUD_MAX_TESTS) {
        return -1;
    }

    reg->tests[reg->count] = info;
    reg->count++;
    return 0;
}
