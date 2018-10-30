#include <linux/module.h>
#include <linux/kprobes.h>

#define _DECL_CMN_JRP(fn, symbol) static struct jprobe fn##_jp = { \
    .entry              = on_##fn##_ent,                           \
    .kp.symbol_name     = ""#symbol"",                             \
};

#define DECL_CMN_JRP(fn) _DECL_CMN_JRP(fn, fn)

__notrace_funcgraph struct task_struct *
on___switch_to_ent(struct task_struct *prev_p, struct task_struct *next_p)
{
    jprobe_return();
    return NULL;
}

DECL_CMN_JRP(__switch_to);

static struct jprobe *wperf_jprobes[] = {
    &__switch_to_jp,
};

static int __init trace_wperf_events_init(void)
{
    int ret;

    ret = register_jprobes(wperf_jprobes, sizeof(wperf_jprobes) / sizeof(wperf_jprobes[0]));
    if(ret) {
        pr_err("Register wperf jprobes failed\n");
        return ret;
    }

    return 0;
}

static void __exit trace_wperf_events_exit(void)
{
    unregister_jprobes(wperf_jprobes, sizeof(wperf_jprobes) / sizeof(wperf_jprobes[0]));
}

MODULE_AUTHOR("Zwb <ethercflow@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
module_init(trace_wperf_events_init);
module_exit(trace_wperf_events_exit);
