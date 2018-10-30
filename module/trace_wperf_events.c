#include <linux/module.h>
#include <linux/kprobes.h>

#define _DECL_CMN_JRP(fn, symbol) static struct jprobe fn##_jp = { \
    .entry              = on_##fn##_ent,                           \
    .kp.symbol_name     = ""#symbol"",                             \
};

#define DECL_CMN_JRP(fn) _DECL_CMN_JRP(fn, fn)

/*
 * switch_to(x,y) should switch tasks from x to y.
 *
 * This could still be optimized:
 * - fold all the options into a flag word and test it with a single test.
 * - could test fs/gs bitsliced
 *
 * Kprobes not supported here. Set the probe on schedule instead.
 * Function graph tracer not supported too.
 */
static __notrace_funcgraph struct task_struct *
on___switch_to_ent(struct task_struct *prev_p, struct task_struct *next_p)
{
    jprobe_return();
    return NULL;
}

DECL_CMN_JRP(__switch_to);

/**
 * try_to_wake_up - wake up a thread
 * @p: the thread to be awakened
 * @state: the mask of task states that can be woken
 * @wake_flags: wake modifier flags (WF_*)
 *
 * Put it on the run-queue if it's not already there. The "current"
 * thread is always on the run-queue (except when the actual
 * re-schedule is in progress), and as such you're allowed to do
 * the simpler "current->state = TASK_RUNNING" to mark yourself
 * runnable without the overhead of this.
 *
 * Return: %true if @p was woken up, %false if it was already running.
 * or @state didn't match @p's state.
 */
static int
on_try_to_wake_up_ent(struct task_struct *p, unsigned int state, int wake_flags)
{
    jprobe_return();
    return 0;
}

DECL_CMN_JRP(try_to_wake_up);

static void on_tasklet_hi_action_ent(struct softirq_action *a)
{
    jprobe_return();
}

DECL_CMN_JRP(tasklet_hi_action);

/*
 * run_timer_softirq runs timers and the timer-tq in bottom half context.
 */
static void on_run_timer_softirq_ent(struct softirq_action *h)
{
    jprobe_return();
}

DECL_CMN_JRP(run_timer_softirq);

static void on_net_tx_action_ent(struct softirq_action *h)
{
    jprobe_return();
}

DECL_CMN_JRP(net_tx_action);

static struct jprobe *wperf_jprobes[] = {
    &__switch_to_jp,
    &try_to_wake_up_jp,
    &tasklet_hi_action_jp,
    &run_timer_softirq_jp,
    &net_tx_action_jp,
};

static int __init trace_wperf_events_init(void)
{
    int ret;

    ret = register_jprobes(wperf_jprobes,
                           sizeof(wperf_jprobes) / sizeof(wperf_jprobes[0]));
    if(ret) {
        pr_err("Register wperf jprobes failed\n");
        return ret;
    }

    return 0;
}

static void __exit trace_wperf_events_exit(void)
{
    unregister_jprobes(wperf_jprobes,
                       sizeof(wperf_jprobes) / sizeof(wperf_jprobes[0]));
}

MODULE_AUTHOR("Zwb <ethercflow@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
module_init(trace_wperf_events_init);
module_exit(trace_wperf_events_exit);
