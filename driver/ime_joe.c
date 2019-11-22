#include <linux/percpu.h>
#include <asm/atomic.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>

#include "tracker.h"
#include "ime_joe.h"

struct iso_struct iso_struct = {0, 0};
DEFINE_PER_CPU(u64, pcpu_profiled);
void *pcpu_core_dev;

int enable_monitor(void)
{
	iso_struct.state = 1;
	return 0;
}

void disable_monitor(void)
{
	iso_struct.state = 0;
}


// static int enable_thread_stack(pid_t pid)
// {
// 	struct task_struct *tsk = get_pid_task(find_get_pid(pid), PIDTYPE_PID);

// 	if (!tsk) goto no_pid;
// 	/* Use an unused bit of the thread flags */
// 	tsk->flags |= BIT(24);
// 	return 0;
// no_pid:
// 	return -1;
// }

static void disable_thread_stack(pid_t pid)
{
	struct task_struct *tsk = get_pid_task(find_get_pid(pid), PIDTYPE_PID);

	if (!tsk) return;
	/* Use an unused bit of the thread flags */
	tsk->flags &= ~BIT(24);
}


int register_thread(pid_t pid)
{
	// int cpu;
	// Check if the thread is already signed up
	//if (enable_thread_stack(pid))
	//	goto no_patch;
	register_pid(pid);
	// for_each_present_cpu(cpu) {
		// per_cpu(pcpu_profiled, cpu) = pid;
	// } 
	// if(current->pid == pid){
	// 	wrmsrl(MSR_IA32_PERF_GLOBAL_CTRL, 0xfULL);
	// 	cpu = get_cpu();
	// 	iso_struct.cpus_pmc |= BIT_ULL(cpu);
	// 	put_cpu();
	// }
	return 0;

// no_patch:
	// return -1;
}// register_thread

int unregister_thread(pid_t pid)
{
	disable_thread_stack(pid);
	// if (tsk) {
	// 	pr_info("Before release, Got task_struct of pid %u\n", tsk->pid);
	// 	put_task_struct(tsk);
	// 	pr_info("After release, Got task_struct of pid %u\n", tsk->pid);
	// }
	return 0; 
}