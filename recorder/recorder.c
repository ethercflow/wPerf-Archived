#include "defs.h"

static void timer_expire(uv_timer_t *handle) {
    struct recorder *recorder;
    struct ioworker *worker;
    int worker_count;
    char *nop = "0";
    uv_buf_t iov;

    recorder = container_of(handle, struct recorder, expire_handler);
    recorder->expired = true;

    worker_count = recorder->worker_count;
    while (worker_count--) {
        worker = &recorder->workers[worker_count];
        uv_process_kill(&worker->req, /* SIGTERM */ 15);
    }

    iov = uv_buf_init(nop, strlen(nop) + 1);
    write_debugfs(&iov, get_instance_enable);
    write_debugfs(&iov, get_instance_filter);
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

    set_instances_bufsize(cf);
    set_filter_and_enable(cf);

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
