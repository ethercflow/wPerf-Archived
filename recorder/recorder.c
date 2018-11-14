#include "defs.h"

#define PATH_MAX_LEN    100

void setup_instances(const char *base, const char **p)
{
    uv_fs_t req;
    char dir[PATH_MAX_LEN];
    int r;

    while (*p) {
        snprintf(dir, PATH_MAX_LEN, "%s/%s", base, *p);
        r = uv_fs_mkdir(NULL, &req, dir, 0755, NULL);
        assert(r == 0 || r == UV_EEXIST);
        p++;
    }

    uv_fs_req_cleanup(&req);
}
