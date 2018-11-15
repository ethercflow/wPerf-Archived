#include "defs.h"

#define PATH_MAX_LEN    100

void on_read(uv_fs_t *req);

void on_write(uv_fs_t *req)
{
    struct ctx *ctx;

    ctx = container_of(req, struct ctx, req);

    if (req->result < 0) {
        fprintf(stderr, "Write error: %s\n", uv_strerror((int)req->result));
    }
    else {
        if (!ctx->expired)
            uv_fs_read(ctx->loop, &ctx->req.read, ctx->fin, &ctx->iov, 1, -1, on_read);
    }
}

void on_read(uv_fs_t *req)
{
    struct ctx *ctx;

    ctx = container_of(req, struct ctx, req);

    if (req->result < 0) {
        fprintf(stderr, "Read error: %s\n", uv_strerror(req->result));
    } else if (req->result == 0) {
        fprintf(stderr, "should close\n");
    } else if (req->result > 0) {
        ctx->iov.len = req->result;
        uv_fs_write(ctx->loop, &ctx->req.write, 1, &ctx->iov, 1, -1, on_write);
    }
}

void on_open(uv_fs_t *req)
{
    struct ctx *ctx;

    ctx = container_of(req, struct ctx, req);

    if (req->result >= 0) {
        ctx->fin = req->result;
        ctx->iov = uv_buf_init(ctx->buf, 4096);
        uv_fs_read(ctx->loop, &ctx->req.read, ctx->fin,
                   &ctx->iov, 1, -1, on_read);
    } else {
        fprintf(stderr, "error opening file: %s\n", uv_strerror((int)req->result));
    }
}

void setup_instances(struct config *cf, const char *base, const char **p)
{
    uv_fs_t req;
    char dir[PATH_MAX_LEN];
    int r;

    while (*p) {
        snprintf(dir, PATH_MAX_LEN, "%s/%s", base, *p);
        r = uv_fs_mkdir(NULL, &req, dir, 0755, NULL);
        assert(r == 0 || r == UV_EEXIST);
        uv_fs_req_cleanup(&req);
        p++;
    }
}

static void timer_expire(uv_timer_t *handle) {
    struct ctx *ctx;

    ctx = container_of(handle, struct ctx, expire_handler);
    ctx->expired = true;
}

void record(struct config *cf, uv_loop_t *loop)
{
    struct ctx ctx;
    ctx.loop = loop;

    uv_fs_open(loop, &ctx.req.open, "/sys/kernel/debug/tracing/instances/switch/trace_pipe", O_RDONLY, 0, on_open);
    uv_timer_init(loop, &ctx.expire_handler);
    uv_timer_start(&ctx.expire_handler, timer_expire, 10000, 0);
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    uv_fs_req_cleanup(&ctx.req.open);
    uv_fs_req_cleanup(&ctx.req.read);
    uv_fs_req_cleanup(&ctx.req.write);
}
