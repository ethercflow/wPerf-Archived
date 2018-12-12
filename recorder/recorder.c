#include "defs.h"

static void cleanup(struct recorder *recorder)
{
    struct ioworker *worker;
    int worker_count;
    char *nop = "0";
    uv_buf_t iov;

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

static void sig_ignore(uv_signal_t* handle, int signum)
{
    (void)handle;
    fprintf(stdout, "Ignore sig: %d\n", signum);
}

static void sig_cleanup(uv_signal_t* handle, int signum)
{
    struct recorder *recorder;

    recorder = container_of(handle, struct recorder, sig_handler);
    cleanup(recorder);
    uv_timer_stop(&recorder->expire_handler);

    fprintf(stdout, "Recv sig: %d, cleanup and exit\n", signum);
}

static void timer_expire(uv_timer_t *handle)
{
    struct recorder *recorder;

    recorder = container_of(handle, struct recorder, expire_handler);
    cleanup(recorder);
    uv_signal_stop(&recorder->sig_handler);
}

int recorder_run(struct config *cf, uv_loop_t *loop)
{
    struct recorder recorder;
    int err;

    recorder.loop = loop;
    recorder.cf = *cf;
    recorder.expired = false;
    recorder.worker_count = 0;
    uv_signal_init(loop, &recorder.sig_handler);

    uv_signal_start(&recorder.sig_handler, sig_ignore, SIGINT);

    setup_event_instances(cf, basedir, &instances[0]);
    setup_ioworkers(cf, &recorder);

    set_instances_bufsize(cf);
    set_filter_and_enable(cf);

    uv_signal_start(&recorder.sig_handler, sig_cleanup, SIGINT);

    err = record_events(&recorder);
    if (err)
        return -1;

    err = record_ioworkers(&recorder);
    if (err)
        return -1;

    uv_timer_init(loop, &recorder.expire_handler);
    uv_timer_start(&recorder.expire_handler, timer_expire, cf->timeout, 0);

    return uv_run(loop, UV_RUN_DEFAULT);
}
