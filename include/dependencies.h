#ifndef _DEPENDENCIES_H
#define _DEPENDENCIES_H

extern int idt_patcher_install_entry(unsigned long handler, unsigned vector, unsigned dpl);

typedef void h_func(void);

struct hook_pair {
	h_func *func_pos;
	h_func *func_neg;
};

extern void switch_hook_pause(void);

extern void switch_hook_resume(void);

extern int hook_register(struct hook_pair *hooks);

extern int pid_register(pid_t pid);

extern int pid_unregister(pid_t pid);

#endif /* _DEPENDENCIES_H */