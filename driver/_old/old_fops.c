#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/smp.h>
#include <linux/vmalloc.h>

#include "tracker.h"

#include "ime_fops.h"
#include "msr_config.h"
#include "intel_pmc_events.h"
#include "ime_pebs.h"
#include "irq_facility.h"
#include "ime_joe.h"
#include "intel/pmu.h"

extern struct pebs_user* buffer_sample;
u64 reset_value_pmc[MAX_ID_PMC];
u64 size_buffer_samples;
// DEFINE_PER_CPU(struct pmc_cfg *, pcpu_pmc_cfg);
DEFINE_PER_CPU(struct statistics *, pcpu_head);
DEFINE_PER_CPU(struct statistics *, pcpu_tail);
DEFINE_PER_CPU(u64, fixed_reset) = 0;
u64 size_stat = 0;
spinlock_t lock_stats;
int stat_index = 0;

void enablePMC(void* arg){
	preempt_disable();
	wrmsrl(MSR_IA32_PERF_GLOBAL_CTRL, 0xfULL | BIT(32) | BIT(33));
	preempt_enable();
}

void disablePMC(void* arg){
	preempt_disable();
	wrmsrl(MSR_IA32_PERF_GLOBAL_CTRL, 0ULL);
	//empty_pebs_buffer();
	preempt_enable();
}

struct pmc_cfg *get_pmc_config(unsigned pmc_id, unsigned cpu)
{
	/* Incorrect PMC ID */
	if (pmc_id < 0 || pmc_id >= MAX_ID_PMC || 
		cpu < 0 || cpu >= num_present_cpus())
		return 0;

	return &(per_cpu(pcpu_pmc_cfg, cpu)[pmc_id]); // TODO CHECK
}

void debugPMC(void* arg){
	u64 msr;
	int cpu_id, k;
	configuration_t* args = (configuration_t*) arg;
	cpu_id = smp_processor_id();
	if(args[cpu_id].valid_cpu == 0) return;
	for(k = 0; k < MAX_ID_PMC; k++){
		if(!args[cpu_id].pmcs[k].valid) continue;
		rdmsrl(MSR_IA32_PMC(k), msr);
		pr_info("[CPU%d] PMC%d: %lx\n", cpu_id, k, msr);
	}
}

u64 get_reset_value(int pmc_id){
	int cpu_id = smp_processor_id();
	struct pmc_cfg *cfg = per_cpu(pcpu_pmc_cfg, cpu_id);
	return cfg[pmc_id].reset;
}

void resetPMC(void* arg){
	pr_info("RESET\n");
	int cpu_id, k, offcore;
	struct pmc_cfg * cfg;
	configuration_t* args = (configuration_t*) arg;
	cpu_id = smp_processor_id();
	if(args[cpu_id].valid_cpu == 0) return;
	wrmsrl(MSR_IA32_FIXED_CTR_CTRL, 0ULL);
	for(k = 0; k < MAX_ID_PMC; k++){
		if(!args[cpu_id].pmcs[k].valid) continue;
		cfg = get_pmc_config(k, cpu_id);
		offcore = args[cpu_id].pmcs[k].offcore;
		switch(offcore){
			case 1:
				wrmsrl(MSR_OFFCORE_RSP_0, 0ULL);
				break;
			
			case 2:
				wrmsrl(MSR_OFFCORE_RSP_1, 0ULL);
				break;
		}
		cfg->perf_evt_sel = 0ULL;
		cfg->pebs = 0;
		cfg->counter = 0;
		cfg->reset = 0;
		wrmsrl(MSR_IA32_PERFEVTSEL(k), cfg->perf_evt_sel);
		wrmsrl(MSR_IA32_PMC(k), cfg->counter);
	}
}

void setupPMC(void* arg){
	int cpu_id, k, offcore;
	struct pmc_cfg * cfg;
	u64 msr;
	configuration_t* args = (configuration_t*) arg;
	cpu_id = smp_processor_id();
	if(args[cpu_id].valid_cpu == 0) return;
	if(!args[cpu_id].fixed_value) wrmsrl(MSR_IA32_FIXED_CTR_CTRL, BIT(1) | BIT(5));
	else{
		this_cpu_write(fixed_reset, args[cpu_id].fixed_value & FIXED_MASK);
		wrmsrl(MSR_IA32_FIXED_CTR_CTRL, BIT(1) | BIT(5) | BIT(7));
		wrmsrl(IA32_PERF_FIXED_CTR1, msr | __this_cpu_read(fixed_reset));
	}
	for(k = 0; k < MAX_ID_PMC; k++){
		if(!args[cpu_id].pmcs[k].valid) continue;
		cfg = get_pmc_config(k, cpu_id);
		offcore = args[cpu_id].pmcs[k].offcore;
		switch(offcore){
			case 1:
				cfg->perf_evt_sel = MSR_OFFCORE_RSP_0_EVENT;
				wrmsrl(MSR_OFFCORE_RSP_0, args[cpu_id].pmcs[k].event);
				break;
			
			case 2:
				cfg->perf_evt_sel = MSR_OFFCORE_RSP_1_EVENT;
				wrmsrl(MSR_OFFCORE_RSP_1, args[cpu_id].pmcs[k].event);
				break;
				
			default:	
				cfg->perf_evt_sel = args[cpu_id].pmcs[k].event;
				break;
		}

		//cfg.usr = args[cpu_id].pmcs[k].user;
		//cfg.os = args[cpu_id].pmcs[k].kernel;
		if(args[cpu_id].pmcs[k].enable_PEBS == 0)cfg->pmi = 1;
		cfg->en = 1; 
		cfg->usr = 1;
		cfg->os = 0;
		cfg->pebs = args[cpu_id].pmcs[k].enable_PEBS;
		//TODO reset e start
		cfg->counter = args[cpu_id].pmcs[k].start_value;
		cfg->reset = args[cpu_id].pmcs[k].reset_value;
		wrmsrl(MSR_IA32_PERFEVTSEL(k), cfg->perf_evt_sel);
		wrmsrl(MSR_IA32_PMC(k), cfg->counter);
	}
}

void readSTATS(void* args){
	unsigned long flags;
	struct statistics_user *arg = (struct statistics_user*) args;
	int k;
	struct statistics *current_node;
	int cpu_id = smp_processor_id();
	spin_lock_irqsave(&lock_stats, flags);
	while(per_cpu(pcpu_head, cpu_id) != NULL){
		current_node = per_cpu(pcpu_head, cpu_id);
		arg[stat_index].fixed0 = current_node->fixed0;
		arg[stat_index].fixed1 = current_node->fixed1;
		for(k = 0; k < MAX_ID_PMC; k++){
			arg[stat_index].event[k].stat = current_node->event[k].stat;
			arg[stat_index].event[k].event = (get_pmc_config(k, cpu_id)->umask << 8) | get_pmc_config(k, cpu_id)->evt;
		}
		per_cpu(pcpu_head, cpu_id) = per_cpu(pcpu_head, cpu_id)->next;
		kfree(current_node);
		++stat_index;
	}
	spin_unlock_irqrestore(&lock_stats, flags);
	per_cpu(pcpu_tail, cpu_id) = NULL;
	per_cpu(pcpu_head, cpu_id) = NULL;
}


void init_pmc_struct(void){
	int i;
	unsigned cpu;
	u64 msr;
	for_each_present_cpu(cpu) {
		per_cpu(pcpu_pmc_cfg, cpu) = kzalloc(sizeof(struct pmc_cfg) * MAX_ID_PMC, GFP_KERNEL);
		per_cpu(pcpu_head, cpu) = NULL;
		per_cpu(pcpu_tail, cpu) = NULL;
	}
}

void clear_pmc_struct(void){
	unsigned cpu;
	for_each_present_cpu(cpu) {
		kfree(per_cpu(pcpu_pmc_cfg, cpu));
	}
}

long ime_ctl_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
    int err = 0;
	if(cmd == IME_PROFILER_ON) on_each_cpu(enablePMC, NULL, 1);
	if(cmd == IME_PROFILER_OFF) on_each_cpu(disablePMC, NULL, 1);

    if(cmd == IME_SETUP_PMC || cmd == IME_RESET_PMC || cmd == IME_READ_STATUS){
        int num_CPU = num_online_cpus();
        configuration_t* args;
        args = (configuration_t*) kzalloc (num_CPU*sizeof(configuration_t), GFP_KERNEL);
        if (!args) return -ENOMEM;
		err = access_ok((void *)arg, num_CPU*sizeof(configuration_t));
		if(!err) goto out_pmc;
		err = copy_from_user(args, (void *)arg, num_CPU*sizeof(configuration_t));
		if(err) goto out_pmc;

		switch(cmd){
			case IME_SETUP_PMC:
				on_each_cpu(setupPMC, (void *) args, 1);
				on_each_cpu(pebs_init, (void *) args, 1);
				break;

			case IME_RESET_PMC:
				this_cpu_write(fixed_reset, 0ULL);
				on_each_cpu(resetPMC, (void *) args, 1);
				on_each_cpu(pebs_exit, (void *) args, 1);
				//on_each_cpu(disablePMC, NULL, 1);
				break;

			case IME_READ_STATUS:
				on_each_cpu(debugPMC, (void *) args, 1);
				pr_info("STATUS\n");
		}
		kfree(args);
		return 0;
	out_pmc:
		kfree(args);
		return -1;
    }

    if(cmd == IME_SIZE_BUFFER){
		unsigned long* args = (unsigned long*)kzalloc (sizeof(unsigned long), GFP_KERNEL);
		if (!args) return -ENOMEM;
		err = copy_from_user(args, (void *)arg, sizeof(unsigned long));
		if(err) goto out_size;
		*args = retrieve_buffer_size();
		size_buffer_samples = *args;
		err = copy_to_user((void *)arg, args, sizeof(unsigned long));
		if(err) goto out_size;

		kfree(args);
		return 0;
	out_size:
		kfree(args);
		return -1;
	}

	if(cmd == IME_READ_BUFFER){
		struct buffer_struct* args = (struct buffer_struct*) vmalloc (sizeof(struct buffer_struct)*size_buffer_samples);
        if (!args) return -ENOMEM;
		err = copy_from_user(args, (void *)arg, sizeof(struct buffer_struct)*size_buffer_samples);
		if(err) goto out_read;
		read_buffer_samples(args);
		reset_hashtable();
		err = copy_to_user((void *)arg, args, sizeof(struct buffer_struct)*size_buffer_samples);
		if(err) goto out_read;

		vfree(args);
		return 0;
	out_read:
		vfree(args);
		return -1;
	}

	if(cmd == IME_SIZE_STATS){
		unsigned long* args = (unsigned long*)kzalloc (sizeof(unsigned long), GFP_KERNEL);
		if (!args) return -ENOMEM;
		err = copy_from_user(args, (void *)arg, sizeof(unsigned long));
		if(err) goto out_ssize;

		*args = size_stat;

		err = copy_to_user((void *)arg, args, sizeof(unsigned long));
		if(err) goto out_size;

		kfree(args);
		return 0;
	out_ssize:
		kfree(args);
		return -1;
	}

	if(cmd == IME_EVT_STATS){
		int k;
		struct statistics_user* args = (struct statistics_user*) vmalloc (sizeof(struct statistics_user)*size_stat);
        if (!args) return -ENOMEM;
		err = copy_from_user(args, (void *)arg, sizeof(struct statistics_user)*size_stat);
		if(err) goto out_stat;
		on_each_cpu(readSTATS, (void*)args, 1);
		err = copy_to_user((void *)arg, args, sizeof(struct statistics_user)*size_stat);
		if(err) goto out_read;
		size_stat = 0;
		stat_index = 0;
		vfree(args);
		return 0;
	out_stat:
		vfree(args);
		return -1;
	}

	if(cmd == IME_WAIT) {
		wait_track();
		pr_info("IME_WAIT\n");
		goto end;
	}

	if(cmd == JOE_PROFILER_ON){
		enable_monitor();
		pr_info("JOE_PROFILER_ON\n");
	}

	if(cmd == JOE_PROFILER_OFF){
		disable_monitor();
		pr_info("JOE_PROFILER_OFF\n");
	}

	if(cmd == JOE_ADD_TID){
		register_thread((pid_t) arg);
		pr_info("JOE_ADD_TID: %lu\n", arg);
	}

	if(cmd == JOE_DEL_TID){
		unregister_thread((pid_t) arg);
		pr_info("JOE_DEL_TID: %lu\n", arg);
	}

end:
	return err;
}// ime_ctl_ioctl
