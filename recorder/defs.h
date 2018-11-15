#ifndef __DEFS_H_
#define __DEFS_H_

#include <uv.h>
#include <assert.h>
#include <stdbool.h>

#define container_of(ptr, type, field)                                        \
  ((type *) ((char *) (ptr) - ((char *) &((type *) 0)->field)))

struct config {
};

struct ctx {
    uv_loop_t *loop;
    union {
        uv_fs_t open;
        uv_fs_t read;
        uv_fs_t write;
    } req;
    uv_file fin;
    uv_file fout;
    uv_timer_t expire_handler;
    bool expired;
    uv_buf_t iov;
    char buf[4096];
};

struct recoder {
    struct config cf;
    struct ctx *ctxs;
    uv_loop_t *loop;
};

void setup_instances(struct config *cf, const char *base, const char **p);
void record(struct config *cf, uv_loop_t *loop);

#endif // __DEFS_H_
