#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/smp.h>
#include <linux/vmalloc.h>

#include "intel_pmc_events.h"
#include "ime_pebs.h"
// #include "irq_facility.h"
#include "intel/pmu.h"

#include "haap_fops.h"
#include "haap_uapi.h"
#include "dependencies.h"

struct file_operations module_fops = {
	.owner 		= THIS_MODULE,
	.unlocked_ioctl = module_ioctl,
};

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

u64 get_reset_value(int pmc_id){
	int cpu_id = smp_processor_id();
	struct pmc_cfg *cfg = per_cpu(pcpu_pmc_cfg, cpu_id);
	return cfg[pmc_id].reset;
}

// void readSTATS(void* args){
// 	unsigned long flags;
// 	struct statistics_user *arg = (struct statistics_user*) args;
// 	int k;
// 	struct statistics *current_node;
// 	int cpu_id = smp_processor_id();
// 	spin_lock_irqsave(&lock_stats, flags);
// 	while(per_cpu(pcpu_head, cpu_id) != NULL){
// 		current_node = per_cpu(pcpu_head, cpu_id);
// 		arg[stat_index].fixed0 = current_node->fixed0;
// 		arg[stat_index].fixed1 = current_node->fixed1;
// 		for(k = 0; k < MAX_ID_PMC; k++){
// 			arg[stat_index].event[k].stat = current_node->event[k].stat;
// 			arg[stat_index].event[k].event = (get_pmc_config(k, cpu_id)->umask << 8) | get_pmc_config(k, cpu_id)->evt;
// 		}
// 		per_cpu(pcpu_head, cpu_id) = per_cpu(pcpu_head, cpu_id)->next;
// 		kfree(current_node);
// 		++stat_index;
// 	}
// 	spin_unlock_irqrestore(&lock_stats, flags);
// 	per_cpu(pcpu_tail, cpu_id) = NULL;
// 	per_cpu(pcpu_head, cpu_id) = NULL;
// }

static long __setup_pmc(void *arg)
{
	int err, i;
	int n_cpus = 1; //num_online_cpus();
	struct pmc_cpu_cfg cpu_cfg;
	struct pmc_conf_list *pmc_list;
	struct pmc_conf *user_ptr;

	pmc_list = (struct pmc_conf_list *) kzalloc(n_cpus * sizeof(struct pmc_conf_list), GFP_KERNEL);
	if (!pmc_list) {
		err = -ENOMEM;
		goto err_list;
	}

	err = access_ok((void *)arg, n_cpus * sizeof(struct pmc_conf_list));
	if(!err) goto err_list;


	err = copy_from_user(pmc_list, (void *)arg, n_cpus * sizeof(struct pmc_conf_list));
	if(err) goto err_list;

	// Copy pointer reference for the deep copy
	user_ptr = pmc_list->pmcs;

	// TODO change the hardcode 9 value with a dynamic one

	pmc_list->pmcs = (struct pmc_conf *) kzalloc(pmc_list->size * sizeof(struct pmc_conf), GFP_KERNEL);
	if (!pmc_list->pmcs) goto err_list;

	pr_info("DEBUG before copy - PMC1.counter: %llx\n", pmc_list->pmcs[1].start);

	err = access_ok((void *) user_ptr, pmc_list->size * sizeof(struct pmc_conf));
	
	if(!err) goto err_pmcs;

	err = copy_from_user(pmc_list->pmcs, (void *)user_ptr, pmc_list->size * sizeof(struct pmc_conf));
	if(err) goto err_pmcs;

	cpu_cfg.nr_pmcs = pmc_list->size;
	cpu_cfg.pmcs = kzalloc((pmc_list->size) * sizeof(struct pmc_cfg), GFP_KERNEL);
	
	if(err) goto err_cfg;
	
	for (i = 0; i < pmc_list->size; ++i) {
		// It's a union, do it before assigning other values
		cpu_cfg.pmcs[i].perf_evt_sel = pmc_list->pmcs[i].event;
		cpu_cfg.pmcs[i].en = 1;
		cpu_cfg.pmcs[i].pmi = !pmc_list->pmcs[i].counting && !pmc_list->pmcs[i].pebs;
		cpu_cfg.pmcs[i].usr = pmc_list->pmcs[i].user;
		cpu_cfg.pmcs[i].os = pmc_list->pmcs[i].kernel;
		cpu_cfg.pmcs[i].pebs = pmc_list->pmcs[i].pebs;
		//TODO reset e start
		cpu_cfg.pmcs[i].counter = pmc_list->pmcs[i].start;
		pr_info("DEBUG inside - PMC%u.counter: %llx\n", i, pmc_list->pmcs[i].start);	
		cpu_cfg.pmcs[i].reset = pmc_list->pmcs[i].reset;
		cpu_cfg.pmcs[i].valid = 1;
	}

	// TODO setup the PMCs

	pmc_setup(&cpu_cfg);
	// TODO send pebs information
	// on_each_cpu(pebs_init, (void *) args, 1);

	kfree(cpu_cfg.pmcs);
err_cfg:
err_pmcs:
	// kfree(pmc_list->pmcs);
err_list:
	// kfree(pmc_list);
	return err;
}

static long __reset_pmc(void)
{

	pmc_reset();
	return 0;
}

static long __register_pid(pid_t pid)
{
	return pid_register(pid);
}

static long __pack_data_to_user(void *arg)
{
	int cpu, k, written = 0, err = 0;
	struct event_stat_list stat_list;
	struct event_stat *list_tmp;
	struct pmc_cpu_cfg *cpu_cfg;
	
	err = access_ok((void *)arg, sizeof(struct event_stat_list));
	if(!err) goto err_list;

	err = copy_from_user(&stat_list, (void *)arg, sizeof(struct event_stat_list));
	if(err) {
		goto err_list;
	}

	if (stat_list.size <= 0) {
		goto no_pmc;
	}

	err = access_ok((void *)stat_list.pmcs, stat_list.size * sizeof(struct event_stat));

	if(!err) goto err_list;

	// TODO WARNING should we stop the profilation?

	for_each_possible_cpu(cpu) {

		cpu_cfg = pmc_get_status_on_cpu(cpu);

		list_tmp = kzalloc(cpu_cfg->nr_pmcs * sizeof(struct event_stat), GFP_KERNEL);
		if (!list_tmp) {
			pr_warn("Cannot allocate memory...\n");
			goto err_list;
		}

		for (k = 0; written < stat_list.size && k < cpu_cfg->nr_pmcs; ++k) {
			list_tmp[k].event = cpu_cfg->pmcs[k].perf_evt_sel;
			list_tmp[k].value = cpu_cfg->pmcs[k].counter;

		}
		copy_to_user((void *)&stat_list.pmcs[written], list_tmp, k * sizeof(struct event_stat));

		written += cpu_cfg->nr_pmcs;
	}

no_pmc:
err_list:
	return err;
}

long module_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int err = 0;

	switch (cmd) {
	case HAAP_PROFILER_ON:
		pmc_on();
		break;
	case HAAP_PROFILER_OFF:
		pmc_off();
		break;
	case HAAP_PMC_SETUP:
		return __setup_pmc((void *)arg);
	case HAAP_DEBUG_PRINT:
		pmc_print_status_on_syslog();
		break;
	case HAAP_PMC_RESET:
		return __reset_pmc();
	case HAAP_PID_ADD:
		return __register_pid((pid_t)arg);
	case HAAP_SWITCH_ON:
		switch_hook_resume();
		break;
	case HAAP_SWITCH_OFF:
		switch_hook_pause();
		break;
	case HAAP_DATA_READ:
		__pack_data_to_user((void *)arg);
		break;
	default:
		break;
	}


    if(cmd == IME_SIZE_BUFFER){
		unsigned long* args = (unsigned long*)kzalloc (sizeof(unsigned long), GFP_KERNEL);
		if (!args) return -ENOMEM;
		err = copy_from_user(args, (void *)arg, sizeof(unsigned long));
		if(err) {
			kfree(args);
			return -1;
		}
		*args = retrieve_buffer_size();
		size_buffer_samples = *args;
		err = copy_to_user((void *)arg, args, sizeof(unsigned long));
		if(err) {
			kfree(args);
			return -1;
		}

		kfree(args);
		return 0;
	}

	if(cmd == IME_READ_BUFFER){
		struct buffer_struct* args = (struct buffer_struct*) vmalloc (sizeof(struct buffer_struct)*size_buffer_samples);
        	if (!args) return -ENOMEM;
		err = copy_from_user(args, (void *)arg, sizeof(struct buffer_struct)*size_buffer_samples);
		if(err) {
			vfree(args);
			return -1;
		}
		read_buffer_samples(args);
		reset_hashtable();
		err = copy_to_user((void *)arg, args, sizeof(struct buffer_struct)*size_buffer_samples);
		if(err) {
			vfree(args);
			return -1;
		}

		vfree(args);
		return 0;
	}

	if(cmd == IME_SIZE_STATS){
		unsigned long* args = (unsigned long*)kzalloc (sizeof(unsigned long), GFP_KERNEL);
		if (!args) return -ENOMEM;
		err = copy_from_user(args, (void *)arg, sizeof(unsigned long));
		if(err) {
			kfree(args);
			return -1;
		}

		*args = size_stat;

		err = copy_to_user((void *)arg, args, sizeof(unsigned long));
		if(err) {
			kfree(args);
			return -1;
		}

		kfree(args);
		return 0;
	}

	// if(cmd == IME_EVT_STATS){
	// 	// int k;
	// 	struct statistics_user* args = (struct statistics_user*) vmalloc (sizeof(struct statistics_user)*size_stat);
 //        	if (!args) return -ENOMEM;
	// 	err = copy_from_user(args, (void *)arg, sizeof(struct statistics_user)*size_stat);
	// 	if(err) {
	// 		vfree(args);
	// 		return 0;
	// 	}
	// 	// on_each_cpu(readSTATS, (void*)args, 1);
	// 	err = copy_to_user((void *)arg, args, sizeof(struct statistics_user)*size_stat);
	// 	if(err) {
	// 		vfree(args);
	// 		return -1;
	// 	}
	// 	size_stat = 0;
	// 	stat_index = 0;
	// 	vfree(args);
	// 	return 0;
	// }

	// if(cmd == IME_WAIT) {
	// 	wait_track();
	// 	pr_info("IME_WAIT\n");
	// 	goto end;
	// }

	// if(cmd == HAAP_SWITCH_ON){
	// 	enable_monitor();
	// 	pr_info("JOE_PROFILER_ON\n");
	// }

	// if(cmd == HAAP_SWITCH_OFF){
	// 	disable_monitor();
	// 	pr_info("JOE_PROFILER_OFF\n");
	// }

	// if(cmd == HAAP_PID_ADD){
	// 	register_thread((pid_t) arg);
	// 	pr_info("JOE_ADD_TID: %lu\n", arg);
	// }

	// if(cmd == HAAP_PID_DEL){
	// 	unregister_thread((pid_t) arg);
	// 	pr_info("JOE_DEL_TID: %lu\n", arg);
	// }
	return err;
}// ime_ctl_ioctl
