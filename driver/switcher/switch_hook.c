#include <linux/kprobes.h>

#include "switch_hook.h"

u64 core_state = 0ULL;

static unsigned long switch_hook_func = 0;

static int system_hooked = 0;
static pid_t last_scheduled = 0;

// static int switch_pre_handler(struct kretprobe_instance *ri, struct pt_regs *regs);
static int switch_post_handler(struct kretprobe_instance *ri, struct pt_regs *regs);

// static struct kretprobe krp_pre = {
// 	.handler = switch_pre_handler,
// };

static struct kretprobe krp_post = {
	.handler = switch_post_handler,
};

// static int switch_pre_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
// {
// 	pr_info("[CPU %u] PRE HANDLER PID %u\n", smp_processor_id(), current->pid);
// 	return 0;
// }

static int switch_post_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	if (smp_processor_id() != 0) return 0;
	
	if (unlikely(last_scheduled < 0)) {
		last_scheduled = current->pid;
	}
	// pr_info("[CPU %u] POST HANDLER PID %u\n", smp_processor_id(), current->pid);
	// pr_info("[CPU %u] LAST %u - CURRENT %u\n", smp_processor_id(), last_scheduled, current->pid);
	
	last_scheduled = current->pid;

	if (system_hooked && switch_hook_func)
		((hook_func *) switch_hook_func)();
	
	return 0;
}

int switch_hook_add_func(void *addr)
{
	if (!system_hooked) return -1;
	
	switch_hook_func = (unsigned long) addr;
	return 0;
}

int switch_hook_init(void)
{
	int err = 0;

	// /* Hook pre function */
	// krp_pre.kp.symbol_name = SWITCH_PRE_FUNC;

	// err = register_kretprobe(&krp_pre);
	// if (err) {
	// 	pr_warn("Cannot hook pre function - ERR_CODE: %d\n", err);
	// 	goto pre_err;
	// };

	/* Hook post function */
	krp_post.kp.symbol_name = SWITCH_POST_FUNC;

	err = register_kretprobe(&krp_post);
	if (err) {
		pr_warn("Cannot hook post function - ERR_CODE: %d\n", err);
		goto post_err;
	};

	system_hooked = 1;

	return err;

post_err:
// 	unregister_kretprobe(&krp_pre);
// pre_err:
	return err;
}


void switch_hook_fini(void)
{
	if (!system_hooked) {
		pr_info("The system is not hooked\n");
		return;
	}

	unregister_kretprobe(&krp_post);

	/* nmissed > 0 suggests that maxactive was set too low. */
	if (krp_post.nmissed) pr_info("Missed %u invocations\n", krp_post.nmissed);

	// unregister_kretprobe(&krp_pre);

	// /* nmissed > 0 suggests that maxactive was set too low. */
	// if (krp_pre.nmissed) pr_info("Missed %u invocations\n", krp_pre.nmissed);

	system_hooked = 0;
}// hook_exit