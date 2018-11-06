#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/interrupt.h>
#include <linux/genhd.h>
#include <linux/jiffies.h>
#include <linux/debugfs.h>

enum {
    HARDIRQ = NR_SOFTIRQS + 1,
    KSOFTIRQ,
    KERNEL
};

#define MAX_DISK_NUM      20
#define MAX_DNAME_SIZE    20

static char dnames[MAX_DISK_NUM][MAX_DNAME_SIZE];
static long dutils[MAX_DISK_NUM];
static int didx = 0;
static bool denabled = false;

static struct dentry *wperf_root = NULL;

struct per_cpu_wperf_data {
    int softirqs_nr;
    int softirq_btime; // begin time
};

static DEFINE_PER_CPU(struct per_cpu_wperf_data, wperf_cpu_data);

/*
 * FIXME: use assoc_array or sth else
 */
struct tx_stat {
    #define MAX_PID_NUM    65535
    long bytes[MAX_PID_NUM];
};

static struct tx_stat tx_stat;

struct ev_ops {
    int (*open)(void *data);
    int (*read)(void *data);
    int (*write)(void *data);
    int (*release)(void *data);
};

struct ev_file {
    const char *name;
    const struct file_operations *fops;
    struct ev_ops *ops;
};

struct event {
    char *name;
    struct ev_file **ev_file;
};

#define CREATE_TRACE_POINTS
#include "trace_wperf_events.h"

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
    trace___switch_to(prev_p, next_p, rdtsc_ordered());
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
 *
 * called by wake_up_process
 */
static int
on_try_to_wake_up_ent(struct task_struct *p, unsigned int state, int wake_flags)
{
    trace_try_to_wake_up(p, state, wake_flags, rdtsc_ordered());
    jprobe_return();
    return 0;
}

DECL_CMN_JRP(try_to_wake_up);

/*
 * we cannot loop indefinitely here to avoid userspace starvation,
 * but we also don't want to introduce a worst case 1/HZ latency
 * to the pending events, so lets the scheduler to balance
 * the softirq load for us.
 *
 * __do_softirq          ----------
 *                                 \
 * invoke_softirq        ---------- \__
 *                                  ___ --- ==> wakeup_softirqd
 * raise_softirq_irqoff  ----------/
 *
 * TODO
 * needn't kretprobe? why?
 */
static void on_wakeup_softirqd_ent(void)
{
    struct per_cpu_wperf_data *data;
    data = &__get_cpu_var(wperf_cpu_data);
    data->softirqs_nr = KSOFTIRQ;
    jprobe_return();
}

DECL_CMN_JRP(wakeup_softirqd);

#define _DECL_SOFTIRQ_KRP(nr, fn, symbol)                          \
static int                                                         \
on_##fn##_ent(struct kretprobe_instance *ri, struct pt_regs *regs) \
{                                                                  \
    struct per_cpu_wperf_data *data;                               \
                                                                   \
    data = &__get_cpu_var(wperf_cpu_data);                         \
    data->softirqs_nr = nr;                                        \
                                                                   \
    return 0;                                                      \
}                                                                  \
                                                                   \
static int                                                         \
on_##fn##_ret(struct kretprobe_instance *ri, struct pt_regs *regs) \
{                                                                  \
    struct per_cpu_wperf_data *data;                               \
                                                                   \
    data = &__get_cpu_var(wperf_cpu_data);                         \
    data->softirqs_nr = NR_SOFTIRQS;                               \
                                                                   \
    return 0;                                                      \
}                                                                  \
static struct kretprobe fn##_krp = {                               \
    .entry_handler      = on_##fn##_ent,                           \
    .handler            = on_##fn##_ret,                           \
    .data_size          = 0,                                       \
    .maxactive          = NR_CPUS * 2,                             \
    .kp.symbol_name     = ""#symbol"",                             \
};

#define DECL_SOFTIRQ_KRP(nr, fn) _DECL_SOFTIRQ_KRP(nr, fn, fn)

/*
 * Priority level: 0
 * inited in softirq_init:
 *     open_softirq(HI_SOFTIRQ, tasklet_hi_action);
 * is mainly used by sound card device drivers or no use now?
 */
DECL_SOFTIRQ_KRP(HI_SOFTIRQ, tasklet_hi_action);

/*
 * Priority level: 1
 * inited in init_timers:
 *     open_softirq(TIMER_SOFTIRQ, run_timer_softirq);
 * run_timer_softirq runs timers and the timer-tq in bottom half context.
 */
DECL_SOFTIRQ_KRP(TIMER_SOFTIRQ, run_timer_softirq);


/*
 * Priority level: 2
 * inited in net_dev_init:
 *     open_softirq(NET_TX_SOFTIRQ, net_tx_action);
 */
DECL_SOFTIRQ_KRP(NET_TX_SOFTIRQ, net_tx_action);

/*
 * Priority level: 3
 * inited in net_dev_init:
 *     open_softirq(NET_RX_SOFTIRQ, net_rx_action);
 */
DECL_SOFTIRQ_KRP(NET_RX_SOFTIRQ, net_rx_action);

/*
 * Priority level: 4
 * inited in blk_softirq_init:
 *     open_softirq(BLOCK_SOFTIRQ, blk_done_softirq);
 * Softirq action handler - move entries to local list and loop over them
 * while passing them to the queue registered handler.
 */
DECL_SOFTIRQ_KRP(BLOCK_SOFTIRQ, blk_done_softirq);

/*
 * Priority level: 5
 * inited in irq_poll_setup:
 *     open_softirq(IRQ_POLL_SOFTIRQ, irq_poll_softirq);
 * irq_poll: Functions related to interrupt-poll handling in the block layer.
 * This is similar to NAPI for network devices.
 */
DECL_SOFTIRQ_KRP(IRQ_POLL_SOFTIRQ, irq_poll_softirq);

/*
 * Priority level: 6
 * inited in softirq_init:
 *     open_softirq(TASKLET_SOFTIRQ, tasklet_action);
 */
DECL_SOFTIRQ_KRP(TASKLET_SOFTIRQ, tasklet_action);

/*
 * Priority level: 7
 * inited in init_sched_fair_class:
 *     open_softirq(SCHED_SOFTIRQ, run_rebalance_domains);
 * run_rebalance_domains is triggered when needed from the scheduler tick.
 * Also triggered for nohz idle balancing (with nohz_balancing_kick set).
 */
DECL_SOFTIRQ_KRP(SCHED_SOFTIRQ, run_rebalance_domains);

/*
 * Priority level: 8
 * HRTIMER_SOFTIRQ, Unused, but kept as tools rely on the numbering. Sigh!
 */

/*
 * Priority level: 9
 * inited in rcu_init:
 *     open_softirq(RCU_SOFTIRQ, rcu_process_callbacks);
 * Do RCU core processing for the current CPU.
 */
DECL_SOFTIRQ_KRP(RCU_SOFTIRQ, rcu_process_callbacks);

#define _DECL_CMN_KRP(fn, symbol, cond) _DECL_CMN_KRP_##cond(fn, symbol)

#define _DECL_CMN_KRP_0(fn, symbol) static struct kretprobe fn##_krp = { \
    .entry_handler      = NULL,                                          \
    .handler            = on_##fn##_ret,                                 \
    .data_size          = 0,                                             \
    .maxactive          = NR_CPUS * 2,                                   \
    .kp.symbol_name     = ""#symbol"",                                   \
};

#define _DECL_CMN_KRP_1(fn, symbol) static struct kretprobe fn##_krp = { \
    .entry_handler      = on_##fn##_ent,                                 \
    .handler            = on_##fn##_ret,                                 \
    .data_size          = sizeof(struct fn##_args),                      \
    .maxactive          = NR_CPUS * 2,                                   \
    .kp.symbol_name     = ""#symbol"",                                   \
};

#define _DECL_CMN_KRP_2(fn, symbol) static struct kretprobe fn##_krp = { \
    .entry_handler      = on_##fn##_ent,                                 \
    .handler            = on_##fn##_ret,                                 \
    .data_size          = 0,                                             \
    .maxactive          = NR_CPUS * 2,                                   \
    .kp.symbol_name     = ""#symbol"",                                   \
};

#define DECL_CMN_KRP(fn, cond) _DECL_CMN_KRP(fn, fn, cond)

#define WITHOUT_ENTEY        0
#define WITH_ENTEY           1
#define WITH_NODATA_ENTEY    2

/*
 * called by run_ksoftirqd
 */
static int on___do_softirq_ent(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct per_cpu_wperf_data *data;

    data = &__get_cpu_var(wperf_cpu_data);
    data->softirq_btime = rdtsc_ordered();

    return 0;
}

static int on___do_softirq_ret(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct per_cpu_wperf_data *data;

    data = &__get_cpu_var(wperf_cpu_data);

    trace___do_softirq_ret(1, data->softirq_btime); /* FIXME */
    return 0;
}

DECL_CMN_KRP(__do_softirq, WITH_NODATA_ENTEY);

static inline int dev_idx(const char *dname)
{
    int i;
    for (i = 0; i < didx; i++) {
        if (!strncmp(dname, dnames[i], MAX_DNAME_SIZE))
            return i;
    }
    return -1;
}

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
    unsigned long now = jiffies;
    struct device *ddev = NULL;
    const char *dname;
    int idx;

    if (part->partno)
        part = &part_to_disk(part)->part0;

    ddev = part_to_dev(part);
    dname = dev_name(ddev);
    idx = dev_idx(dname);

    if (idx < 0)
        goto end;

    if (time_after(now, part->stamp) && part_in_flight(part)) {
        dutils[idx] += now - part->stamp; // TODO: is this safe?
    }

end:
    jprobe_return();
}

DECL_CMN_JRP(part_round_stats);

/**
 * futex_wait_queue_me() - queue_me() and wait for wakeup, timeout, or signal
 * @hb:		the futex hash bucket, must be locked by the caller
 * @q:		the futex_q to queue up on
 * @timeout:	the prepared hrtimer_sleeper, or null for no timeout
 *
 * futex: Fast Userspace muTEXes
 *
 */
static void on_futex_wait_queue_me_ent(struct futex_hash_bucket *hb, struct futex_q *q,
                struct hrtimer_sleeper *timeout)
{
    trace_futex_wait_queue_me(1, rdtsc_ordered()); /* FIXME */
    jprobe_return();
}

DECL_CMN_JRP(futex_wait_queue_me);

/*
 * called by SYSC_futex/C_SYSC_futex http://man7.org/linux/man-pages/man2/futex.2.html
 */
static long on_do_futex_ent(u32 __user *uaddr, int op, u32 val, ktime_t *timeout,
                            u32 __user *uaddr2, u32 val2, u32 val3)
{
    trace_do_futex(1, rdtsc_ordered()); /* FIXME */
    jprobe_return();
    return 0;
}

DECL_CMN_JRP(do_futex);

/*
 * IO end handler for temporary buffer_heads handling writes to the journal.
 *
 * ext4's jbd2
 */
static void on_journal_end_buffer_io_sync_ent(struct buffer_head *bh, int uptodate)
{
    trace_journal_end_buffer_io_sync(in_serving_softirq() >0);
    jprobe_return();
}

DECL_CMN_JRP(journal_end_buffer_io_sync);

/*
 * wake_up_new_task - wake up a newly created task for the first time.
 *
 * This function will do some initial scheduler statistics housekeeping
 * that must be done for every newly created context, then puts the task
 * on the runqueue and wakes it.
 *
 * kernel_thread --------- \
 *                          \
 * sys_fork      --------- \_\__
 *                          _ __ --- ==> do_fork => wake_up_new_task
 * sys_vfork     --------- / /
 *                          /
 * SYSC_clone    --------- /
 * already have a trace point: trace_sched_wakeup_new/sched:sched_wakeup_new
 * TODO:
 * 1. make sure the infos we need to trace
 */
static void on_wake_up_new_task_ent(struct task_struct *p)
{
    trace_wake_up_new_task(p, rdtsc_ordered());
    jprobe_return();
}

DECL_CMN_JRP(wake_up_new_task);

static void on_do_exit_ent(long code)
{
    trace_do_exit(rdtsc_ordered());
    jprobe_return();
}

DECL_CMN_JRP(do_exit);

/*
 * In the TCP output engine, all paths lead to tcp_transmit_skb() regardless of
 * whether we are sending a TCP data packet for the first time, or a retransmit,
 * or even a SYN packet in response to a connect() system call.
 *
 * At the top-level, tcp_sendmsg() and tcp_sendpage() gather up data (either from
 * userspace or the page cache) into SKB packets and tack them onto the
 * sk_write_queue() of the TCP socket. At appropriate times they invoke either
 * tcp_write_xmit() or tcp_push_one() to try and output those data frames.
 */
static int on_tcp_sendmsg_ent(struct kiocb *iocb, struct sock *sk,
                              struct msghdr *msg, size_t size)
{
    tx_stat.bytes[current->pid] += size;
    jprobe_return();
    return 0;
}

DECL_CMN_JRP(tcp_sendmsg);

static int on_udp_sendmsg_ent(struct sock *sk, struct msghdr *msg, size_t len)
{
    tx_stat.bytes[current->pid] += len;
    jprobe_return();
    return 0;
}

DECL_CMN_JRP(udp_sendmsg);

/*
 * Look at on_tcp_sendmsg_ent comment
 */
static int on_tcp_sendpage_ent(struct sock *sk, struct page *page, int offset,
                               size_t size, int flags)
{
    tx_stat.bytes[current->pid] += size;
    jprobe_return();
    return 0;
}

DECL_CMN_JRP(tcp_sendpage);

static int on_udp_sendpage_ent(struct sock *sk, struct page *page, int offset,
                               size_t size, int flags)
{
    tx_stat.bytes[current->pid] += size;
    jprobe_return();
    return 0;
}

DECL_CMN_JRP(udp_sendpage);

/*
 * called by SYS_sendto
 */
static int on_sock_sendmsg_ent(struct socket *sock, struct msghdr *msg, size_t size)
{
    tx_stat.bytes[current->pid] += size;
    jprobe_return();
    return 0;
}

DECL_CMN_JRP(sock_sendmsg);

/*
 * lock_sock and release_sock do not hold a normal spinlock directly but instead hold
 * the owner field and do other housework as well.
 *
 * lock_sock grabs the lock sk→sk_lock.slock, disables local bottom halves and then it
 * checks to see if there is an owner. If it does it spins until this releases, sets
 * the owner and then releases sk→sk_lock.slock. This means bh_lock_sock can still
 * execute even if the socket is “locked” provided of course that the lock_sock call
 * isn't in execution at that very point in time.
 *
 * release_sock grabs the sk_lock.slock, processes any receive backlog, clears the owner,
 * wakes up any wait queue on sk_lock.wq and then releases sk_lock.slock and enables
 * bottom halves.
 *
 * bh_lock_sock and bh_release_sock just grab and release sk→sk_lock.slock
 *
 * TODO:
 *     Do we need to trace release_sock?
 */
static void on___lock_sock_ent(struct sock *sk)
{
    trace___lock_sock(3, rdtsc_ordered());
    jprobe_return();
}

DECL_CMN_JRP(__lock_sock);

static struct jprobe *wperf_jps[] = {
    &__switch_to_jp,
    &try_to_wake_up_jp,
    &wakeup_softirqd_jp,
    &part_round_stats_jp,
    &futex_wait_queue_me_jp,
    &do_futex_jp,
    &journal_end_buffer_io_sync_jp,
    &wake_up_new_task_jp,
    &do_exit_jp,
    &tcp_sendmsg_jp,
    &udp_sendmsg_jp,
    &tcp_sendpage_jp,
    &udp_sendpage_jp,
    &sock_sendmsg_jp,
    &__lock_sock_jp,
};

static struct kretprobe *wperf_krps[] = {
    &tasklet_hi_action_krp,
    &run_timer_softirq_krp,
    &net_tx_action_krp,
    &net_rx_action_krp,
    &blk_done_softirq_krp,
    &irq_poll_softirq_krp,
    &tasklet_action_krp,
    &run_rebalance_domains_krp,
    &rcu_process_callbacks_krp,
    &__do_softirq_krp,
};

static int filter_open_generic(struct inode *inode, struct file *file)
{
    struct ev_file *ef = inode->i_private;

    return ef->ops->open(ef);
}

static ssize_t filter_read_generic(struct file *filp, char __user *ubuf, size_t cnt, loff_t *ppos)
{
    struct ev_file *ef = file_inode(filp)->i_private;

    return ef->ops->read(ef);
}

static ssize_t filter_write_generic(struct file *filp, const char __user *ubuf, size_t cnt, loff_t *ppos)
{
    struct ev_file *ef = file_inode(filp)->i_private;
    char *buf;
    int err;

    if (cnt >= PAGE_SIZE)
        return -EINVAL;

    buf = (char *)__get_free_page(GFP_TEMPORARY);
    if (!buf)
        return -ENOMEM;

    if (copy_from_user(buf, ubuf, cnt)) {
        free_page((unsigned long) buf);
        return -EFAULT;
    }
    buf[cnt] = '\0';

    err = !ef->ops->write(buf);
    free_page((unsigned long)buf);
    if (err < 0)
        return -EINVAL;

    *ppos += cnt;

    return cnt;
}

static int filter_release_generic(struct inode *inode, struct file *file)
{
    struct ev_file *ef = inode->i_private;

    return ef->ops->release(ef);
}

static const struct file_operations filter_fops_generic = {
    .open = filter_open_generic,
    .read = filter_read_generic,
    .write = filter_write_generic,
    .release = filter_release_generic,
};

static int dutils_filter_open(void *data)
{
    pr_warn("dutils_filter_open");
    return 0;
}

static int dutils_filter_read(void *data)
{
    int i;

    pr_warn("dutils_filter_read");
    for (i = 0; i < didx; i++)
        pr_warn("dnames[%d]: %s", i, dnames[i]);

    return 0;
}

static int dutils_filter_write(void *data)
{
    char *buf, *s, *token;
    size_t len;

    buf = kstrdup(data, GFP_KERNEL);
    if (buf == NULL)
        return -ENOMEM;
    s = strstrip(buf);

    didx = 0;
    disable_jprobe(&part_round_stats_jp);

    while (1) {
        token = strsep(&s, ",");
        if (token == NULL)
            break;

        if (*token == '\0')
            continue;

        len = strlen(token) + 1;
        if (len > MAX_DNAME_SIZE)
            return -EINVAL;

        strncpy(dnames[didx++], token, len);
    }

    if (denabled)
        enable_jprobe(&part_round_stats_jp);

    kfree(buf);

    return 0;
}

static int dutils_filter_release(void *data)
{
    pr_warn("dutils_filter_release");
    return 0;
}

static struct ev_ops ev_dutils_filter_ops = {
    .open = &dutils_filter_open,
    .read = &dutils_filter_read,
    .write = &dutils_filter_write,
    .release = &dutils_filter_release,
};

static struct ev_file ev_dutils_filter = {
    .name = "filter",
    .fops = &filter_fops_generic,
    .ops = &ev_dutils_filter_ops,
};

static struct ev_file *ev_dutils_files[] = {
    &ev_dutils_filter,
    NULL,
};

static struct event ev_dutils = {
    .name = "dutils",
    .ev_file = &ev_dutils_files[0],
};

static int create_event_files(struct dentry *parent, struct event *event)
{
    while (*event->ev_file) {
        BUG_ON(!debugfs_create_file((*event->ev_file)->name, 0644, parent,
                                    *event->ev_file, (*event->ev_file)->fops));
        event->ev_file++;
    }
    return 0;
}

static int create_event_dir(struct dentry *parent, struct event *event)
{
    struct dentry *dir;

    dir = debugfs_create_dir(event->name, parent);
    BUG_ON(!dir);

    return create_event_files(dir, event);
}

static struct dentry *wperf_lookup(void)
{
    struct dentry *parent = NULL;
    const char *dirs[] = { "tracing", "events", "wperf", NULL };
    const char **pdir = &dirs[0];

    while (*pdir)
        parent = debugfs_lookup(*pdir++, parent);

    BUG_ON(!parent);

    return parent;
}

static int __init trace_wperf_events_init(void)
{
    int ret;

    ret = register_jprobes(wperf_jps,
                           sizeof(wperf_jps) / sizeof(wperf_jps[0]));
    if(ret) {
        pr_err("Register wperf jprobes failed\n");
        return ret;
    }

    ret = register_kretprobes(wperf_krps,
                              sizeof(wperf_krps) / sizeof(wperf_krps[0]));
    if(ret) {
        pr_err("Register wperf kretprobes failed\n");
        return ret;
    }

    wperf_root = wperf_lookup();
    create_event_dir(wperf_root, &ev_dutils);

    return 0;
}

static void __exit trace_wperf_events_exit(void)
{
    unregister_jprobes(wperf_jps,
                       sizeof(wperf_jps) / sizeof(wperf_jps[0]));
    unregister_kretprobes(wperf_krps,
                          sizeof(wperf_krps) / sizeof(wperf_krps[0]));
}

MODULE_AUTHOR("Zwb <ethercflow@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
module_init(trace_wperf_events_init);
module_exit(trace_wperf_events_exit);
