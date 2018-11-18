#include "defs.h"

static const char *basedir = "/sys/kernel/debug/tracing/instances";
static const char *instances[] = {
    "switch",
    "softirq",
    "wait",
    NULL
};
const int instances_num = 3;

static const char *switch_events[] = {
    "__switch_to",
    "try_to_wake_up",
    "wake_up_new_task",
    "do_exit",
    NULL
};

static const char *softirq_events[] = {
    "__do_softirq_ret",
    NULL
};
static const char *wait_events[] = {
    "futex_wait_queue_me",
    "do_futex",
    "__lock_sock",
    NULL
};

static const char **pevents[] = {
    &switch_events[0],
    &softirq_events[0],
    &wait_events[0],
};

static void write_debugfs(uv_buf_t *iov,
                          void (*get_fname_func)(char *dir,
                                                 const char *base,
                                                 const char *instance,
                                                 const char *event))
{
    char fname[MAX_PATH_LEN];
    const char **p;
    const char **q;
    uv_fs_t req;
    uv_file fd;
    int i, r;

    p = &instances[0];
    i = 0;
    while (*p) {
        q = pevents[i];
        while (*q) {
            get_fname_func(fname, basedir, *p, *q);
            fd = uv_fs_open(NULL, &req, fname, O_RDWR, 0644, NULL);
            if (fd < 0) {
                fprintf(stderr, "open %s failed: %s\n", fname, uv_strerror(fd));
                exit(1);
            }
            r = uv_fs_write(NULL, &req, fd, iov, 1, -1, NULL);
            if (r < 0) {
                fprintf(stderr, "set %s failed: %s\n", fname, uv_strerror(r));
                exit(1);
            }
            q++;
        }
        p++;
        i++;
    }
}

static void set_filter_and_enable(struct config *cf)
{
    uv_buf_t iov;
    char *plist;
    char *enable = "1";

    plist = strdup(cf->pid_list);
    iov = uv_buf_init(plist, strlen(plist) + 1);
    write_debugfs(&iov, get_instance_filter);
    free(plist);

    iov = uv_buf_init(enable, strlen(enable) + 1);
    write_debugfs(&iov, get_instance_enable);
}

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
