#undef TRACE_SYSTEM
#define TRACE_SYSTEM wperf

#if !defined(_TRACE_WPERF_EVENTS_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_WPERF_EVENTS_H

#include <linux/tracepoint.h>

/*
 * FIXME:
 * Maybe has a better way
 */
#define HARDIRQ_CTX    -1
#define KERNEL_CTX     -2

/*
 * FIXME: add wperf time
 * TODO:  determine which clock ftrace use
 */
TRACE_EVENT(__switch_to,

    TP_PROTO(struct task_struct *prev_p, struct task_struct *next_p),

    TP_ARGS(prev_p, next_p),

    TP_STRUCT__entry(
        __field(pid_t, prev_pid)
        __field(pid_t, next_pid)
        __field(long,  prev_state)
        __field(long,  next_state)
        __field(int,   in_which_ctx)
    ),

    TP_fast_assign(
        struct per_cpu_wperf_data *data;

        __entry->prev_pid   = prev_p->pid;
        __entry->next_pid   = next_p->pid;
        __entry->prev_state = prev_p->state;
        __entry->next_state = next_p->state;

        data = &__get_cpu_var(wperf_cpu_data);

        if (in_irq())
            __entry->in_which_ctx = HARDIRQ_CTX;
        else if (in_serving_softirq())
            __entry->in_which_ctx = data->softirqs_nr;
        else  /* TODO: maybe merged with in_serving_softirq */
            __entry->in_which_ctx = KERNEL_CTX;
    ),

    TP_printk("prev_pid=%d, next_pid=%d, prev_state=%llu, next_state=%llu,"
              " in_which_ctx=%d", __entry->prev_pid, __entry->next_pid,
              __entry->prev_state, __entry->next_state,
              __entry->in_which_ctx)
);

#endif // _TRACE_WPERF_EVENTS_H

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE trace_wperf_events
#include <trace/define_trace.h>
