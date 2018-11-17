#include "defs.h"

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

static void cal_worker_count(int *count, const char *dev_list)
{
    char *s, *token;

    if (!dev_list)
        return;

    s = strdup(dev_list);

    while (1) {
        token = strsep(&s, ",");
        if (token == NULL)
            break;

        if (*token == '\0')
            continue;

        *count += 1;
    }
}

/*
 * FIXME support multi io dev
 */
static void build_worker_args(struct config *cf, int worker_count)
{
    int idx = 0;

    cf->argv = calloc(sizeof(char **), worker_count);

    if (cf->disk_list) {
        static char *disk_args[] = {"iostat", NULL, "-d", "1", NULL};
        disk_args[1] = (char*)cf->disk_list;
        cf->argv[idx++] = &disk_args[0];
    }
}

void setup_ioworkers(struct config *cf, struct recorder *recorder)
{
    struct ioworker *worker;
    int worker_count = 0;
    char dir[MAX_PATH_LEN];
    char *fname;
    uv_fs_t req;
    char **args;
    int r;

    cal_worker_count(&worker_count, cf->disk_list);
    cal_worker_count(&worker_count, cf->nic_list);

    if (!worker_count)
        return;

    recorder->worker_count = worker_count;

    build_worker_args(cf, worker_count);
    recorder->workers = calloc(sizeof(struct ioworker), worker_count);

    while (worker_count--) {
        worker = &recorder->workers[worker_count];
        worker->loop = recorder->loop;

        create_instance_output(dir, &req, cf->output_dir, cf->disk_list);
        fname = get_instance_output(dir, cf->output_dir, cf->disk_list);

        r = uv_fs_open(NULL, &worker->fs_req.open, fname,
                       O_CREAT | O_RDWR, 0644, NULL);
        if (r < 0) {
            fprintf(stderr, "Open error: %s\n", uv_strerror(r));
            exit(1);
        }
        worker->fd = r;

        uv_stdio_container_t child_stdio[3];
        child_stdio[0].flags = UV_IGNORE;
        child_stdio[1].flags = UV_INHERIT_FD;
        child_stdio[1].data.fd = worker->fd;
        child_stdio[2].flags = UV_IGNORE;

        worker->options.stdio = child_stdio;
        worker->options.stdio_count = 3;

        worker->options.exit_cb = close_process_handle;

        args = cf->argv[worker_count];
        worker->options.file = args[0];
        worker->options.args = args;

        fprintf(stderr, "Setup worker %d\n", worker->req.pid);
        free(fname);
    }
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
    }

    return 0;
}
