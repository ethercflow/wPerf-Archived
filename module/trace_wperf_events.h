#undef TRACE_SYSTEM
#define TRACE_SYSTEM wperf

#if !defined(_TRACE_WPERF_EVENTS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_WPERF_EVENTS_H

#include <linux/tracepoint.h>

TRACE_EVENT(__switch_to,

    TP_PROTO(struct task_struct *prev_p, struct task_struct *next_p, u64 tsc),

    TP_ARGS(prev_p, next_p, tsc),

    TP_STRUCT__entry(
        __field(int,   type)
        __field(u64,   tsc)
        __array(char,  prev_comm, TASK_COMM_LEN)
        __array(char,  next_comm, TASK_COMM_LEN)
        __field(pid_t, prev_pid)
        __field(pid_t, next_pid)
        __field(long,  prev_state)
        __field(long,  next_state)
        __field(int,   in_which_ctx)
    ),

    TP_fast_assign(
        struct per_cpu_wperf_data *data;

        __entry->type       = WAIT;
        __entry->tsc        = tsc;
        memcpy(__entry->prev_comm, prev_p->comm, TASK_COMM_LEN);
        memcpy(__entry->next_comm, next_p->comm, TASK_COMM_LEN);
        __entry->prev_pid   = prev_p->pid;
        __entry->next_pid   = next_p->pid;
        __entry->prev_state = prev_p->state;
        __entry->next_state = next_p->state;

        data = &__get_cpu_var(wperf_cpu_data);

        if (in_irq())
            __entry->in_which_ctx = HARDIRQ;
        else if (in_serving_softirq()) /* Are we currently processing softirq? */
            __entry->in_which_ctx = -data->softirqs_nr;
        else
            __entry->in_which_ctx = KERNEL;
    ),

    TP_printk("type=%d, tsc=%llu, prev_comm=%s, next_comm=%s, prev_pid=%d, next_pid=%d, prev_state=%ld,"
              " next_state=%ld, in_which_ctx=%d",
              __entry->type, __entry->tsc,
              __entry->prev_comm, __entry->next_comm,
              __entry->prev_pid, __entry->next_pid,
              __entry->prev_state, __entry->next_state,
              __entry->in_which_ctx)
);

TRACE_EVENT(try_to_wake_up,

    TP_PROTO(struct task_struct *p, unsigned int state, int wake_flags, u64 tsc),

    TP_ARGS(p, state, wake_flags, tsc),

    TP_STRUCT__entry(
        __field(int,   type)
        __field(u64,   tsc)
        __array(char,  prev_comm, TASK_COMM_LEN)
        __array(char,  next_comm, TASK_COMM_LEN)
        __field(pid_t, prev_pid)
        __field(pid_t, next_pid)
        __field(long,  prev_state)
        __field(long,  next_state)
        __field(int,   in_which_ctx)
    ),

    TP_fast_assign(
        struct per_cpu_wperf_data *data;

        __entry->type       = WAKEUP;
        __entry->tsc        = tsc;
        memcpy(__entry->prev_comm, current->comm, TASK_COMM_LEN);
        memcpy(__entry->next_comm, p->comm, TASK_COMM_LEN);
        __entry->prev_pid   = current->pid;
        __entry->next_pid   = p->pid;
        __entry->prev_state = current->state;
        __entry->next_state = p->state;

        data = &__get_cpu_var(wperf_cpu_data);

        if (in_irq())
            __entry->in_which_ctx = HARDIRQ;
        else if (in_serving_softirq()) /* Are we currently processing softirq? */
            __entry->in_which_ctx = data->softirqs_nr;
        else
            __entry->in_which_ctx = KERNEL;
    ),

    TP_printk("type=%d, tsc=%llu, prev_comm=%s, next_comm=%s, prev_pid=%d, next_pid=%d, prev_state=%ld,"
              " next_state=%ld, in_which_ctx=%d",
              __entry->type, __entry->tsc,
              __entry->prev_comm, __entry->next_comm,
              __entry->prev_pid, __entry->next_pid,
              __entry->prev_state, __entry->next_state,
              __entry->in_which_ctx)
);

TRACE_EVENT(__do_softirq_ret,

    TP_PROTO(u64 begin_time, u64 end_time),

    TP_ARGS(begin_time, end_time),

    TP_STRUCT__entry(
        __array(char, prev_comm, TASK_COMM_LEN)
        __field(u64, begin_time)
        __field(u64, end_time)
    ),

    TP_fast_assign(
        memcpy(__entry->prev_comm, current->comm, TASK_COMM_LEN);
        __entry->begin_time = begin_time;
        __entry->end_time = end_time;
    ),

    TP_printk("prev_comm=%s, begin_time=%llu, end_time=%llu",
              __entry->prev_comm, __entry->begin_time, __entry->end_time)
);

DECLARE_EVENT_CLASS(common_event,

    TP_PROTO(int type, u64 tsc),

    TP_ARGS(type, tsc),

    TP_STRUCT__entry(
        __field(int,   type)
        __field(u64,   tsc)
    ),

    TP_fast_assign(
        __entry->type = type;
        __entry->tsc  = tsc;
    ),

    TP_printk("type=%d, tsc=%llu", __entry->type, __entry->tsc)
);

DEFINE_EVENT(common_event, futex_wait_queue_me,

    TP_PROTO(int type, u64 tsc),

    TP_ARGS(type, tsc)
);

DEFINE_EVENT(common_event, do_futex,

    TP_PROTO(int type, u64 tsc),

    TP_ARGS(type, tsc)
);

TRACE_EVENT(journal_end_buffer_io_sync,

    TP_PROTO(bool in_serving_softirq),

    TP_ARGS(in_serving_softirq),

    TP_STRUCT__entry(
        __field(bool, in_serving_softirq)
    ),

    TP_fast_assign(
        __entry->in_serving_softirq = in_serving_softirq;
    ),

    TP_printk("in_serving_softirq=%d", __entry->in_serving_softirq)
);

TRACE_EVENT(wake_up_new_task,

    TP_PROTO(struct task_struct *p, u64 tsc),

    TP_ARGS(p, tsc),

    TP_STRUCT__entry(
        __field(int,   type)
        __field(u64,   tsc)
        __array(char,  prev_comm, TASK_COMM_LEN)
        __array(char,  next_comm, TASK_COMM_LEN)
        __field(pid_t, prev_pid)
        __field(pid_t, next_pid)
        __field(long,  prev_state)
        __field(long,  next_state)
        __field(int,   in_which_ctx)
    ),

    TP_fast_assign(
        __entry->type       = CREATE;
        __entry->tsc        = tsc;
        memcpy(__entry->prev_comm, current->comm, TASK_COMM_LEN);
        memcpy(__entry->next_comm, p->comm, TASK_COMM_LEN);
        __entry->prev_pid   = current->pid;
        __entry->next_pid   = p->pid;
        __entry->prev_state = current->state;
        __entry->next_state = p->state;
        __entry->in_which_ctx = KERNEL;
    ),

    TP_printk("type=%d, tsc=%llu, prev_comm=%s, next_comm=%s, prev_pid=%d, next_pid=%d, prev_state=%ld,"
              " next_state=%ld, in_which_ctx=%d",
              __entry->type, __entry->tsc,
              __entry->prev_comm, __entry->next_comm,
              __entry->prev_pid, __entry->next_pid,
              __entry->prev_state, __entry->next_state,
              __entry->in_which_ctx)
);

TRACE_EVENT(do_exit,

    TP_PROTO(u64 tsc),

    TP_ARGS(tsc),

    TP_STRUCT__entry(
        __field(int,   type)
        __field(u64,   tsc)
        __array(char,  prev_comm, TASK_COMM_LEN)
        __array(char,  next_comm, TASK_COMM_LEN)
        __field(pid_t, prev_pid)
        __field(pid_t, next_pid)
        __field(long,  prev_state)
        __field(long,  next_state)
        __field(int,   in_which_ctx)
    ),

    TP_fast_assign(
        __entry->type       = EXIT;
        memcpy(__entry->prev_comm, current->comm, TASK_COMM_LEN);
        memcpy(__entry->next_comm, "swapper", TASK_COMM_LEN);
        __entry->prev_pid   = current->pid;
        __entry->next_pid   = 0;
        __entry->prev_state = current->state;
        __entry->next_state = 0;
        __entry->in_which_ctx = KERNEL;
    ),

    TP_printk("type=%d, tsc=%llu, prev_comm=%s, next_comm=%s,  prev_pid=%d, next_pid=%d, prev_state=%ld,"
              " next_state=%ld, in_which_ctx=%d",
              __entry->type, __entry->tsc,
              __entry->prev_comm, __entry->next_comm,
              __entry->prev_pid, __entry->next_pid,
              __entry->prev_state, __entry->next_state,
              __entry->in_which_ctx)
);

DEFINE_EVENT(common_event, __lock_sock,

    TP_PROTO(int type, u64 tsc),

    TP_ARGS(type, tsc)
);

#endif // _TRACE_WPERF_EVENTS_H

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE trace_wperf_events
#include <trace/define_trace.h>
