#include "defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void parse_opts(struct config *conf, int argc, char **argv);
static void usage(void);

static const char *basedir = "/sys/kernel/debug/tracing/instances";
static const char *instances[] = { "switch", "softirq", "wait", NULL };

int main(int argc, char *argv[])
{
    struct config conf;

    memset(&conf, 0, sizeof(conf));

    parse_opts(&conf, argc, argv);

    setup_instances(basedir, &instances[0]);

    return 0;
}

static void parse_opts(struct config *conf, int argc, char **argv)
{
    usage();
}

static void usage(void)
{

}
