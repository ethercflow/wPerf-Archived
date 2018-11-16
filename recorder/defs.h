#ifndef __DEFS_H_
#define __DEFS_H_

#include <uv.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#define MAX_PATH_LEN    100

#define container_of(ptr, type, field)                                        \
  ((type *) ((char *) (ptr) - ((char *) &((type *) 0)->field)))

struct config {
    char **instances_in;
    char **instances_out;
    int instances_num;
    const char *pid_list;
    const char *disk_list;
    const char *nic_list;
    uint64_t timeout;
    const char *output_dir;
};

struct recorder;

struct event_ctx {
    uv_loop_t *loop;
    struct recorder *recorder;
    union {
        uv_fs_t open;
        uv_fs_t read;
        uv_fs_t write;
        uv_fs_t close;
    } req;
    uv_file fd[2];
    uv_buf_t iov;
};

struct recorder {
    uv_loop_t *loop;
    struct config cf;
    uv_timer_t expire_handler;
    bool expired;
    struct event_ctx *events;
};

void setup_event_instances(struct config *cf, const char *base, const char **p);
int recorder_run(struct config *cf, uv_loop_t *loop);
int record_events(struct recorder *recorder);

#endif // __DEFS_H_
