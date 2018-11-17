#include "defs.h"

static void close_process_handle(uv_process_t *req, int64_t exit_status, int term_signal)
{
    fprintf(stderr, "Process exited with status %" PRId64 ", signal %d\n", exit_status, term_signal);
    uv_close((uv_handle_t*) req, NULL);
}

void setup_ioworkers(struct config *cf, struct recorder *recorder)
{
    struct ioworker *worker;
    int worker_count = 1; /* FIXME */
    char **args;

    recorder->workers = calloc(sizeof(struct ioworker), worker_count);
    while (worker_count--) {
        worker = &recorder->workers[worker_count];
        args = cf->argv[worker_count];
        uv_pipe_init(recorder->loop, &worker->pipe, 1);

        uv_stdio_container_t child_stdio[3];
        child_stdio[0].flags = UV_IGNORE;
        child_stdio[0].data.stream = (uv_stream_t*)&worker->pipe;
        child_stdio[1].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
        child_stdio[2].flags = UV_INHERIT_FD;
        child_stdio[2].data.fd = 2;

        worker->options.stdio = child_stdio;
        worker->options.stdio_count = 3;

        worker->options.exit_cb = close_process_handle;
        worker->options.file = args[0];
        worker->options.args = args;

        fprintf(stderr, "Setup worker %d\n", worker->req.pid);
        uv_spawn(recorder->loop, &worker->req, &worker->options);
    }
}
