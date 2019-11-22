#ifndef _SWITCH_HOOK_H
#define _SWITCH_HOOK_H

#define SWITCH_PRE_FUNC "perf_event_task_sched_out"
#define SWITCH_POST_FUNC "finish_task_switch"

typedef void hook_func(void);

/* maskbit that indicates which core has the active support */
extern u64 core_state;

int switch_hook_init(void);

int switch_hook_add_func(void *addr);

void switch_hook_fini(void); 

#endif /* _SWITCH_HOOK_H */