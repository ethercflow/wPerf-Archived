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
static struct task_struct *
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

static void on_net_rx_action_ent(struct softirq_action *a)
{
    jprobe_return();
}

DECL_CMN_JRP(net_rx_action);

/*
 * Softirq action handler - move entries to local list and loop over them
 * while passing them to the queue registered handler.
 */
static void on_blk_done_softirq_ent(struct softirq_action *a)
{
    jprobe_return();
}

DECL_CMN_JRP(blk_done_softirq);

static void on_tasklet_action_ent(struct softirq_action *a)
{
    jprobe_return();
}

DECL_CMN_JRP(tasklet_action);

/*
 * run_rebalance_domains is triggered when needed from the scheduler tick.
 * Also triggered for nohz idle balancing (with nohz_balancing_kick set).
 */
static void on_run_rebalance_domains_ent(struct softirq_action *a)
{
    jprobe_return();
}

DECL_CMN_JRP(run_rebalance_domains);

/*
 * Do RCU core processing for the current CPU.
 */
static void on_rcu_process_callbacks_ent(struct softirq_action *unused)
{
    jprobe_return();
}

DECL_CMN_JRP(rcu_process_callbacks);

/*
 * The guts of the apic timer interrupt
 */
static void on_local_apic_timer_interrupt_ent(void)
{
    jprobe_return();
}

DECL_CMN_JRP(local_apic_timer_interrupt);

/*
 * do_IRQ handles all normal device IRQ's (the special
 * SMP cross-CPU interrupts have their own specific
 * handlers).
 */
static unsigned int __irq_entry on_do_IRQ_ent(struct pt_regs *regs)
{
    jprobe_return();
}

DECL_CMN_JRP(do_IRQ);

/* aio_complete
 * Called when the io request on the given iocb is complete.
 */
static void on_aio_complete_ent(struct kiocb *iocb, long res, long res2)
{
    jprobe_return();
}

DECL_CMN_JRP(aio_complete);

/*
 * we cannot loop indefinitely here to avoid userspace starvation,
 * but we also don't want to introduce a worst case 1/HZ latency
 * to the pending events, so lets the scheduler to balance
 * the softirq load for us.
 */
static void on_wakeup_softirqd_ent(void)
{
    jprobe_return();
}

DECL_CMN_JRP(wakeup_softirqd);

static void on___do_softirq_ent(void)
{
    jprobe_return();
}

DECL_CMN_JRP(__do_softirq);

static void on_add_interrupt_randomness_ent(int irq, int irq_flags)
{
    jprobe_return();
}

DECL_CMN_JRP(add_interrupt_randomness);

/**
 * part_round_stats() - Round off the performance stats on a struct disk_stats.
 * @cpu: cpu number for stats access
 * @part: target partition
 *
 * The average IO queue length and utilisation statistics are maintained
 * by observing the current state of the queue length and the amount of
 * time it has been in this state for.
 *
 * Normally, that accounting is done on IO completion, but that can result
 * in more than a second's worth of IO being accounted for within any one
 * second, leading to >100% utilisation.  To deal with that, we call this
 * function to do a round-off before returning the results when reading
 * /proc/diskstats.  This accounts immediately for all queue usage up to
 * the current jiffies and restarts the counters again.
 */
static void on_part_round_stats_ent(int cpu, struct hd_struct *part)
{
    jprobe_return();
}

DECL_CMN_JRP(part_round_stats);

/**
 * futex_wait_queue_me() - queue_me() and wait for wakeup, timeout, or signal
 * @hb:		the futex hash bucket, must be locked by the caller
 * @q:		the futex_q to queue up on
 * @timeout:	the prepared hrtimer_sleeper, or null for no timeout
 */
static void on_futex_wait_queue_me_ent(struct futex_hash_bucket *hb, struct futex_q *q,
                struct hrtimer_sleeper *timeout)
{
    jprobe_return();
}

DECL_CMN_JRP(futex_wait_queue_me);

/*
 * IO end handler for temporary buffer_heads handling writes to the journal.
 */
static void on_journal_end_buffer_io_sync_ent(struct buffer_head *bh, int uptodate)
{
    jprobe_return();
}

DECL_CMN_JRP(journal_end_buffer_io_sync);

/*
 * wake_up_new_task - wake up a newly created task for the first time.
 *
 * This function will do some initial scheduler statistics housekeeping
 * that must be done for every newly created context, then puts the task
 * on the runqueue and wakes it.
 */
static void on_wake_up_new_task_ent(struct task_struct *p)
{
    jprobe_return();
}

DECL_CMN_JRP(wake_up_new_task);

static void on_do_exit_ent(long code)
{
    jprobe_return();
}

DECL_CMN_JRP(do_exit);

static int on_tcp_sendmsg_ent(struct kiocb *iocb, struct sock *sk,
                              struct msghdr *msg, size_t size)
{
    jprobe_return();
    return 0;
}

DECL_CMN_JRP(tcp_sendmsg);

static int on_udp_sendmsg_ent(struct sock *sk, struct msghdr *msg, size_t len)
{
    jprobe_return();
    return 0;
}

DECL_CMN_JRP(udp_sendmsg);

static int on_tcp_sendpage_ent(struct sock *sk, struct page *page, int offset,
                               size_t size, int flags)
{
    jprobe_return();
    return 0;
}

DECL_CMN_JRP(tcp_sendpage);

static int on_udp_sendpage_ent(struct sock *sk, struct page *page, int offset,
                               size_t size, int flags)
{
    jprobe_return();
    return 0;
}

DECL_CMN_JRP(udp_sendpage);

static int on_sock_sendmsg_ent(struct socket *sock, struct msghdr *msg, size_t size)
{
    jprobe_return();
    return 0;
}

DECL_CMN_JRP(sock_sendmsg);

static long on_do_futex_ent(u32 __user *uaddr, int op, u32 val, ktime_t *timeout,
                            u32 __user *uaddr2, u32 val2, u32 val3)
{
    jprobe_return();
    return 0;
}

DECL_CMN_JRP(do_futex);

static void on___lock_sock_ent(struct sock *sk)
{
    jprobe_return();
}

DECL_CMN_JRP(__lock_sock);

static struct jprobe *wperf_jprobes[] = {
    &__switch_to_jp,
    &try_to_wake_up_jp,
    &tasklet_hi_action_jp,
    &run_timer_softirq_jp,
    &net_tx_action_jp,
    &net_rx_action_jp,
    &blk_done_softirq_jp,
    &tasklet_action_jp,
    &run_rebalance_domains_jp,
    &rcu_process_callbacks_jp,
    &local_apic_timer_interrupt_jp,
    &do_IRQ_jp,
    &aio_complete_jp,
    &wakeup_softirqd_jp,
    &__do_softirq_jp,
    &add_interrupt_randomness_jp,
    &part_round_stats_jp,
    &futex_wait_queue_me_jp,
    &journal_end_buffer_io_sync_jp,
    &wake_up_new_task_jp,
    &do_exit_jp,
    &tcp_sendmsg_jp,
    &udp_sendmsg_jp,
    &tcp_sendpage_jp,
    &udp_sendpage_jp,
    &sock_sendmsg_jp,
    &do_futex_jp,
    &__lock_sock_jp,
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
