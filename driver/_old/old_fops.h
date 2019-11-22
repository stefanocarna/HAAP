#ifndef IME_FOPS_H
#define IME_FOPS_H

#include "intel_pmc_events.h"
#include "../include/ime-ioctl.h"

#define FIXED_MASK 	((1ULL << 48) - 1)

long ime_ctl_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

void disablePMC(void* arg);
void enablePMC(void* arg);

void init_pmc_struct(void);
void clear_pmc_struct(void);

u64 get_reset_value(int pmc_id);

extern u64 user_events[MAX_NUM_EVENT];
extern __percpu struct statistics *pcpu_head;
extern __percpu struct statistics *pcpu_tail;
extern __percpu u64 fixed_reset;

struct statistics{
	uint64_t fixed0;
	uint64_t fixed1;
	struct event_stat event[MAX_ID_PMC];
	struct statistics *next;
};

#endif