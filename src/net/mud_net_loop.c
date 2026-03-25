#include "mud_net_loop.h"
#include "mud_log.h"

#include <signal.h>

static uv_loop_t g_loop;
static uv_signal_t g_sigint;
static uv_signal_t g_sigterm;
static bool g_initialized = false;

static void on_signal(uv_signal_t* handle, int signum) {
    (void)handle;
    LOG_CORE_INFO("Received signal %d, stopping event loop", signum);
    mud_net_loop_stop();
}

static void close_walk_cb(uv_handle_t* handle, void* arg) {
    (void)arg;
    if (!uv_is_closing(handle)) {
        uv_close(handle, NULL);
    }
}

bool mud_net_loop_init(void) {
    int rc = 0;
    if (g_initialized) {
        return true;
    }

    rc = uv_loop_init(&g_loop);
    if (rc != 0) {
        LOG_CORE_ERROR("uv_loop_init failed: %s", uv_strerror(rc));
        return false;
    }

    rc = uv_signal_init(&g_loop, &g_sigint);
    if (rc != 0) {
        LOG_CORE_ERROR("uv_signal_init(SIGINT) failed: %s", uv_strerror(rc));
        uv_loop_close(&g_loop);
        return false;
    }

    rc = uv_signal_init(&g_loop, &g_sigterm);
    if (rc != 0) {
        LOG_CORE_ERROR("uv_signal_init(SIGTERM) failed: %s", uv_strerror(rc));
        uv_close((uv_handle_t*)&g_sigint, NULL);
        uv_run(&g_loop, UV_RUN_DEFAULT);
        uv_loop_close(&g_loop);
        return false;
    }

    rc = uv_signal_start(&g_sigint, on_signal, SIGINT);
    if (rc != 0) {
        LOG_CORE_ERROR("uv_signal_start(SIGINT) failed: %s", uv_strerror(rc));
        uv_close((uv_handle_t*)&g_sigint, NULL);
        uv_close((uv_handle_t*)&g_sigterm, NULL);
        uv_run(&g_loop, UV_RUN_DEFAULT);
        uv_loop_close(&g_loop);
        return false;
    }

    rc = uv_signal_start(&g_sigterm, on_signal, SIGTERM);
    if (rc != 0) {
        LOG_CORE_ERROR("uv_signal_start(SIGTERM) failed: %s", uv_strerror(rc));
        uv_signal_stop(&g_sigint);
        uv_close((uv_handle_t*)&g_sigint, NULL);
        uv_close((uv_handle_t*)&g_sigterm, NULL);
        uv_run(&g_loop, UV_RUN_DEFAULT);
        uv_loop_close(&g_loop);
        return false;
    }

    g_initialized = true;
    LOG_CORE_INFO("Event loop initialized");
    return true;
}

void mud_net_loop_run(void) {
    if (!g_initialized) {
        return;
    }

    LOG_CORE_INFO("Starting network event loop");
    uv_run(&g_loop, UV_RUN_DEFAULT);
    LOG_CORE_INFO("Network event loop exited");
}

void mud_net_loop_stop(void) {
    if (!g_initialized) {
        return;
    }

    uv_stop(&g_loop);
}

void mud_net_loop_close(void) {
    if (!g_initialized) {
        return;
    }

    uv_signal_stop(&g_sigint);
    uv_signal_stop(&g_sigterm);
    uv_walk(&g_loop, close_walk_cb, NULL);
    uv_run(&g_loop, UV_RUN_DEFAULT);
    uv_loop_close(&g_loop);
    g_initialized = false;
    LOG_CORE_INFO("Network event loop closed");
}

uv_loop_t* mud_net_loop_get(void) {
    return g_initialized ? &g_loop : NULL;
}
