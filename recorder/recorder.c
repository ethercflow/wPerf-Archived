#include "defs.h"

static const char *basedir = "/sys/kernel/debug/tracing/instances";
static const char *instances[] = { "switch", "softirq", "wait", NULL };
const int instances_num = 3;

static void timer_expire(uv_timer_t *handle) {
    struct recorder *recorder;
    struct ioworker *worker;
    int worker_count;

    recorder = container_of(handle, struct recorder, expire_handler);
    recorder->expired = true;

    worker_count = recorder->worker_count;
    while (worker_count--) {
        worker = &recorder->workers[worker_count];
        uv_process_kill(&worker->req, /* SIGTERM */ 15);
    }
}

int recorder_run(struct config *cf, uv_loop_t *loop)
{
    struct recorder recorder;
    int err;

    recorder.loop = loop;
    recorder.cf = *cf;
    recorder.expired = false;
    recorder.worker_count = 0;

    setup_event_instances(cf, basedir, &instances[0]);
    setup_ioworkers(cf, &recorder);

    err = record_events(&recorder);
    if (err)
        return -1;

    err = record_ioworkers(&recorder);
    if (err)
        return -1;

    uv_timer_init(loop, &recorder.expire_handler);
    uv_timer_start(&recorder.expire_handler, timer_expire, cf->timeout, 0);

    return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}
