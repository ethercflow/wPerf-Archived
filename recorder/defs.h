#ifndef __DEFS_H_
#define __DEFS_H_

#include <uv.h>
#include <assert.h>

struct config {
};

void setup_instances(struct config *cf, const char *base, const char **p);
void record(struct config *cf, uv_loop_t *loop);

#endif // __DEFS_H_
