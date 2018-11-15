#ifndef __DEFS_H_
#define __DEFS_H_

#include <uv.h>
#include <assert.h>

struct config {
};

struct ctx {
    uv_loop_t *loop;
    uv_fs_t open_req;
    uv_fs_t read_req;
    uv_fs_t write_req;
};

struct recoder {
    struct config cf;
    struct ctx *ctxs;
    uv_loop_t *loop;
};

void setup_instances(struct config *cf, const char *base, const char **p);
void record(struct config *cf, uv_loop_t *loop);

#endif // __DEFS_H_
