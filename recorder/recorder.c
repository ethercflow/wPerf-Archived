#include "defs.h"

static void timer_expire(uv_timer_t *handle) {
    struct recorder *recorder;

    recorder = container_of(handle, struct recorder, expire_handler);
    recorder->expired = true;
}

int recorder_run(struct config *cf, uv_loop_t *loop)
{
    struct recorder recorder;
    int err;

    recorder.loop = loop;
    recorder.cf = *cf;
    recorder.expired = false;

    err = record_events(&recorder);
    if (err)
        return -1;

    uv_timer_init(loop, &recorder.expire_handler);
    uv_timer_start(&recorder.expire_handler, timer_expire, cf->timeout, 0);

    return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
