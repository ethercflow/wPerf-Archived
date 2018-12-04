#include "defs.h"

const char *basedir = "/sys/kernel/debug/tracing/instances";

const char *instances[] = {
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

static void on_read(uv_fs_t *req);

static inline void cleanup(struct event_ctx *event)
{
    uv_fs_close(event->loop, &event->req.close, event->fd[0], NULL);
    uv_fs_fsync(event->loop, &event->req.close, event->fd[1], NULL);
    uv_fs_close(event->loop, &event->req.close, event->fd[1], NULL);
}

static void on_write(uv_fs_t *req)
{
    struct event_ctx *event;

    event = container_of(req, struct event_ctx, req);

    if (req->result < 0) {
        fprintf(stderr, "Write error: %s\n", uv_strerror((int)req->result));
        goto cleanup;
    }

    if (event->recorder->expired) {
        goto cleanup;
    }

    uv_fs_read(event->loop, &event->req.read, event->fd[0],
               &event->iov, 1, -1, on_read);
    return;

cleanup:
    cleanup(event);
}

static void on_read(uv_fs_t *req)
{
    struct event_ctx *event;

    event = container_of(req, struct event_ctx, req);

    if (event->recorder->expired)
        goto cleanup;

    if (req->result <= 0) {
        if (req->result != UV_EAGAIN) {
            fprintf(stderr, "Read error: %s\n", uv_strerror(req->result));
            goto cleanup;
        }

        uv_fs_read(event->loop, &event->req.read, event->fd[0],
                   &event->iov, 1, -1, on_read);
        return;
    }

    event->iov.len = req->result;
    uv_fs_write(event->loop, &event->req.write, event->fd[1],
                &event->iov, 1, -1, on_write);
    return;

cleanup:
    cleanup(event);
}

static void on_open(uv_fs_t *req)
{
    struct event_ctx *event;

    event = container_of(req, struct event_ctx, req);

    if (req->result < 0) {
        fprintf(stderr, "Open error: %s\n", uv_strerror((int)req->result));
        return;
    }

    event->fd[0] = req->result;
    uv_fs_read(event->loop, &event->req.read, event->fd[0],
               &event->iov, 1, -1, on_read);
}

static int init_event_ctx(struct event_ctx *event, struct recorder *recorder)
{
    char *buf;
    int len = 4096;

    buf = malloc(sizeof(char) * len);
    if (!event || !buf)
        return -1;

    event->loop = recorder->loop;
    event->recorder = recorder;
    event->fd[0] = -1;
    event->fd[1] = -1;

    event->iov = uv_buf_init(buf, len);

    return 0;
}

int record_events(struct recorder *recorder)
{
    struct config cf = recorder->cf;
    struct event_ctx *event;
    int i;

    recorder->events = malloc(cf.instances_num * sizeof(recorder->events[0]));
    if (!recorder->events)
        return -1;

    for (i = 0; i < cf.instances_num; i++) {
        event = &recorder->events[i];
        assert(!init_event_ctx(event, recorder));
        event->fd[1] = uv_fs_open(event->loop, &event->req.open,
                                  cf.instances_out[i],
                                  O_CREAT | O_RDWR, 0644, NULL);
        uv_fs_open(event->loop, &event->req.open, cf.instances_in[i],
                   O_RDONLY|O_NONBLOCK, 0, on_open);
    }

    return 0;
}

void setup_event_instances(struct config *cf, const char *base, const char **p)
{
    char dir[MAX_PATH_LEN];
    uv_fs_t req;
    int i, r;

    r = uv_fs_mkdir(NULL, &req, cf->output_dir, 0755, NULL);
    assert(r == 0 || r == UV_EEXIST);

    for (i = 0; i < cf->instances_num; i++) {
        create_instance_dir(dir, &req, base, *p);
        create_instance_output(dir, &req, cf->output_dir, *p);
        cf->instances_in[i] = get_instance_input(dir, base, *p);
        cf->instances_out[i] = get_instance_output(dir, cf->output_dir, *p);
        p++;
    }

    uv_fs_req_cleanup(&req);
}

static void __write_debugfs(char *fname, uv_buf_t *iov)
{
    uv_fs_t req;
    uv_file fd;
    int r;

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
}

void write_debugfs(uv_buf_t *iov,
                   void (*get_fname_func)(char *dir,
                                          const char *base,
                                          const char *instance,
                                          const char *event))
{
    char fname[MAX_PATH_LEN];
    const char **p;
    const char **q;
    int i;

    p = &instances[0];
    i = 0;
    while (*p) {
        q = pevents[i];
        while (*q) {
            get_fname_func(fname, basedir, *p, *q);
            __write_debugfs(fname, iov);
            q++;
        }
        p++;
        i++;
    }
}

void set_filter_and_enable(struct config *cf)
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

void set_instances_bufsize(struct config *cf)
{
    char fname[MAX_PATH_LEN];
    uv_buf_t iov;
    char *bufsize_kb;
    const char **p;

    bufsize_kb = strdup(cf->bufsize_kb);
    iov = uv_buf_init(bufsize_kb, strlen(bufsize_kb) + 1);

    p = &instances[0];
    while (*p) {
        get_instance_bufsize(fname, basedir, *p++);
        __write_debugfs(fname, &iov);
    }

    free(bufsize_kb);
}
