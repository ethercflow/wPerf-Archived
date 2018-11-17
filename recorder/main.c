#include "defs.h"

static void parse_opts(struct config *conf, int argc, char **argv);
static void usage(void);

static const char *progname = __FILE__;  /* Reset in main(). */

int main(int argc, char *argv[])
{
    struct config conf;
    int err;

    progname = argv[0];
    memset(&conf, 0, sizeof(conf));
    conf.instances_num = instances_num;
    conf.instances_in = malloc(sizeof(char*) * instances_num);
    conf.instances_out = malloc(sizeof(char*) * instances_num);
    parse_opts(&conf, argc, argv);

    err = recorder_run(&conf, uv_default_loop());
    if (err)
        exit(1);

    return 0;
}

static void parse_opts(struct config *cf, int argc, char **argv)
{
    int opt;

    /*
     * Default val
     */
    cf->output_dir = "/tmp/wperf";
    cf->timeout = 90000;

    char *arg1[] = { "ls", NULL };
    char *arg2[] = { "date", NULL };

    cf->argv = malloc(2 * sizeof(char **));
    cf->argv[0] = &arg1[0];
    cf->argv[1] = &arg2[0];

    while (-1 != (opt = getopt(argc, argv, "p:P:hd:n:o:"))) {
        switch (opt) {
            case 'p':
                cf->pid_list = optarg;
                break;
            case 'P':
                if (1 != sscanf(optarg, "%" PRIu64, &cf->timeout)) {
                    fprintf(stderr, "bad period: %s", optarg);
                    usage();
                }
                break;
            case 'd':
                cf->disk_list = optarg;
                break;
            case 'n':
                cf->nic_list = optarg;
                break;
            case 'o':
                cf->output_dir = optarg;
                break;
            default:
                usage();
        }
    }
}

static void usage(void)
{
    fprintf(stderr, "Usage: \n"
            "\n"
            "    %s -p <pids> -P <period> [-h] [-d <disks>] [-n <nics>] [-o output]\n"
            "\n"
            "Options:\n"
            "\n"
            "    -p <pids>      Pid list separated by ','\n"
            "    -P <period>    Default: 90000 (90s)\n"
            "    -h             Show this help message.\n"
            "    -d <disks>     Disk name list seprated by ','\n"
            "    -n <nics>      Nic name list seprated by ','\n"
            "    -o <output>    Output dir default: /tmp/wperf/\n"
            "",
            progname);
    exit(1);
}
