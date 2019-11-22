#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/wait.h>

#include "tracker.h"

// List of tracked processes
static LIST_HEAD(tracked_list);
static unsigned tracked_count;
// Wait queue for tracked programs
static DECLARE_WAIT_QUEUE_HEAD(wq);
// Lock the list access
static spinlock_t lock;


#define exit_func_name "do_exit"
#define fork_func_name "_do_fork"
#define exec_func_name "do_execve"

// static int register_pid(pid_t pid);
static int unregister_pid(pid_t pid);

static int exit_pre_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	// Directly check and remove to save double checks
	unregister_pid(current->pid);
	return 0;
}

static int fork_ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	unsigned long retval = 0;

	if (is_pid_present(current->pid)) {
		retval = regs_return_value(regs);
		if (is_syscall_success(regs)) {
			// ADD new pid (retval) to tracked list
			register_pid(retval);
		}
	}

	return 0;
}

static struct kretprobe krp_fork = {
	.handler = fork_ret_handler,
	.maxactive	= 20,
};

static struct kretprobe krp_exit = {
	.entry_handler = exit_pre_handler,
	.maxactive	= 20,
};

static int profilation_done(void)
{
	// return list_empty(&tracked_list);
	pr_info("%u going to sleep: %u\n", current->pid, tracked_count);
	return !tracked_count;
}

int register_pid(pid_t pid)
{

	struct tracked_process *tp;
	tp = kmalloc(sizeof(struct tracked_process), GFP_KERNEL);
	tp->pid = pid;

	spin_lock(&lock);
	list_add(&tp->list, &tracked_list);
	tracked_count++;
	spin_unlock(&lock);
	
	pr_info("Registered pid %u\n", pid);
	return 0;
}

static int unregister_pid(pid_t pid)
{
	struct list_head *p;
	struct tracked_process *tp;

	spin_lock(&lock);
	list_for_each(p, &tracked_list)
        {
                tp = list_entry(p, struct tracked_process, list);
                if (tp->pid == pid) 
                {
                        list_del(p);
                        pr_info("Unregistered pid %u\n", pid);
                        // write_unlock(&list_lock);
                        kfree(tp);
                        tracked_count--;
			if (profilation_done()) {
				wake_up_interruptible_all(&wq);
			}
                        goto end;
                }
        }
end:
        spin_unlock(&lock);
        return 0;
}

int is_pid_present(pid_t pid)
{
	struct list_head *p;
	struct tracked_process *tp;

	spin_lock(&lock);
	list_for_each(p, &tracked_list)
        {
                tp = list_entry(p, struct tracked_process, list);
                if (tp->pid == pid) {
			spin_unlock(&lock);
                	return 1;
        	}
        }
	spin_unlock(&lock);
	return 0;
}

int tracker_init(void)
{
	int err = 0;

	tracked_count = 0;

	spin_lock_init(&lock);
	init_waitqueue_head(&wq);

	krp_fork.kp.symbol_name = fork_func_name;
	krp_exit.kp.symbol_name = exit_func_name;

	err = register_kretprobe(&krp_fork);
	if (err) {
		pr_err("Cannot register probe on 'fork'\n");
		goto end;
	}

	err = register_kretprobe(&krp_exit);
	if (err) {
		pr_err("Cannot register probe on 'fork'\n");
		goto end;
	}

	pr_info("Tracker correctly initialized\n");

end:
	return err;
}

void tracker_fini(void)
{
	unregister_kretprobe(&krp_exit);
	unregister_kretprobe(&krp_fork);

	// pr_info("Missed probing %d instances of %s\n", krp.nmissed, krp.kp.symbol_name);
	
	pr_info("Tracker correctly uninstalled\n");
}

void wait_track(void)
{
	printk("MODULE: This module is goint to sleep....\n");
	wait_event_interruptible(wq, profilation_done());
} 