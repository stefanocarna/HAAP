#ifndef _HAAP_FOPS_H
#define _HAAP_FOPS_H

#include <linux/fs.h>
#include "intel_pmc_events.h"

extern struct file_operations module_fops;

extern long module_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

#define FIXED_MASK 	((1ULL << 48) - 1)

void init_pmc_struct(void);
void clear_pmc_struct(void);

u64 get_reset_value(int pmc_id);

extern u64 user_events[MAX_NUM_EVENT];
extern __percpu struct statistics *pcpu_head;
extern __percpu struct statistics *pcpu_tail;
extern __percpu u64 fixed_reset;

#endif