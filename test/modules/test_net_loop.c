#include "test/test_autoreg.h"

#include "mud_net_loop.h"

TEST(net_loop_init_get_and_close) {
    CHECK_TRUE(ctx, mud_net_loop_init());
    CHECK_NOT_NULL(ctx, mud_net_loop_get());

    mud_net_loop_stop();
    mud_net_loop_close();

    CHECK_NULL(ctx, mud_net_loop_get());
}
