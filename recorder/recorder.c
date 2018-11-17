#include "defs.h"

static const char *basedir = "/sys/kernel/debug/tracing/instances";
static const char *instances[] = { "switch", "softirq", "wait", NULL };
const int instances_num = 3;

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

    setup_ioworkers(cf, &recorder);
    setup_event_instances(cf, basedir, &instances[0]);

    err = record_events(&recorder);
    if (err)
        return -1;

    uv_timer_init(loop, &recorder.expire_handler);
    uv_timer_start(&recorder.expire_handler, timer_expire, cf->timeout, 0);

    return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
