#ifndef IME_JOE_H
#define IME_JOE_H

#include <linux/kernel.h>

extern __percpu u64 pcpu_profiled;

struct iso_struct {
	u8 state;
	// TODO Change to BitsToLong
	u64 cpus_state; // it can manage at most 64 cpus
	u64 cpus_pmc;
};

enum core_states {
	CORE_ON 	= 0,
	CORE_OFF	= 1,
	CORE_MAX_STATES,
};

extern struct iso_struct iso_struct;

extern void *pcpu_core_dev; // DEPRECATED

int enable_monitor(void);
void disable_monitor(void);

int register_thread(pid_t pid);
int unregister_thread(pid_t pid);

#endif /* IME_JOE_H */