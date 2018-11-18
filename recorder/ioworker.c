#include "defs.h"

enum iotype {
    DISK = 0,
    NIC,
};

static void close_process_handle(uv_process_t *req, int64_t exit_status,
                                 int term_signal)
{
    struct ioworker *worker;

    worker = container_of(req, struct ioworker, req);

    uv_fs_close(worker->loop, &worker->fs_req.close, worker->fd, NULL);
    uv_close((uv_handle_t*)req, NULL);

    fprintf(stderr, "Process exited with status %" PRId64 ", signal %d\n",
            exit_status, term_signal);
}

static inline void cal_worker_count(int *count, const char *dev_list)
{
    if (!dev_list)
        return;

    *count += 1;
}

/*
 * FIXME support multi io dev
 */
static inline void build_disk_worker_args(struct config *cf)
{
    if (cf->disk_list) {
        static char *disk_args[] = {"iostat", NULL, "-d", "1", NULL};
        disk_args[1] = (char*)cf->disk_list;
        cf->argv[0] = &disk_args[0];
    }
}

static inline void build_nic_worker_args(struct config *cf)
{
    if (cf->nic_list) {
        static char *nic_args[] = {"ifstat", "-i", NULL, NULL};
        nic_args[2] = (char*)cf->nic_list;
        cf->argv[1] = &nic_args[0];
    }
}

static void __setup_ioworkers(struct config *cf, struct recorder *recorder,
                              const char *dlist, enum iotype t)
{
    struct ioworker *worker;
    char dir[MAX_PATH_LEN];
    char *fname;
    char **args;
    uv_fs_t req;
    int r;

    if (!dlist)
        return;

    worker = &recorder->workers[recorder->worker_count++];
    worker->loop = recorder->loop;

    create_instance_output(dir, &req, cf->output_dir, dlist);
    fname = get_instance_output(dir, cf->output_dir, dlist);

    r = uv_fs_open(NULL, &worker->fs_req.open, fname,
                   O_CREAT | O_RDWR, 0644, NULL);
    if (r < 0) {
        fprintf(stderr, "Open error: %s\n", uv_strerror(r));
        exit(1);
    }
    worker->fd = r;

    worker->child_stdio[0].flags = UV_IGNORE;
    worker->child_stdio[1].flags = UV_INHERIT_FD;
    worker->child_stdio[1].data.fd = worker->fd;
    worker->child_stdio[2].flags = UV_IGNORE;

    worker->options.stdio = worker->child_stdio;
    worker->options.stdio_count = 3;

    worker->options.exit_cb = close_process_handle;

    args = cf->argv[t];
    worker->options.file = args[0];
    worker->options.args = args;

    uv_spawn(worker->loop, &worker->req, &worker->options);
    free(fname);
}

void setup_ioworkers(struct config *cf, struct recorder *recorder)
{
    int worker_count = 0;

    cal_worker_count(&worker_count, cf->disk_list);
    cal_worker_count(&worker_count, cf->nic_list);

    if (!worker_count)
        return;

    cf->argv = calloc(sizeof(char **), worker_count);

    build_disk_worker_args(cf);
    build_nic_worker_args(cf);

    recorder->workers = calloc(sizeof(struct ioworker), worker_count);

    __setup_ioworkers(cf, recorder, cf->disk_list, DISK);
    __setup_ioworkers(cf, recorder, cf->nic_list, NIC);
}

int record_ioworkers(struct recorder *recorder)
{
    struct ioworker *worker;
    int worker_count;
    int r;

    worker_count = recorder->worker_count;
    while (worker_count--) {
        worker = &recorder->workers[worker_count];
        r = uv_spawn(worker->loop, &worker->req, &worker->options);
        if (r) {
            fprintf(stderr, "uv_spawn failed: %s\n", uv_strerror(r));
            return -1;
        }
        fprintf(stderr, "Setup worker %d\n", worker->req.pid);
    }

    return 0;
}
