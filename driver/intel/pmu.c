#include <linux/preempt.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <asm/msr.h>

#include "pmu.h"
#include "specs/msr.h"
#include "pmi.h"

#define MAX_ID_PMC 4

DEFINE_PER_CPU(struct pmc_cfg *, pcpu_pmc_cfg);

struct pmc_cfg *get_pmc_config(unsigned pmc_id, unsigned cpu)
{
	/* Incorrect PMC ID */
	if (pmc_id < 0 || pmc_id >= MAX_ID_PMC || 
		cpu < 0 || cpu >= num_present_cpus())
		return NULL;

	return &(per_cpu(pcpu_pmc_cfg, cpu)[pmc_id]); // TODO CHECK
}

static int pmc_resource_alloc(void)
{
	unsigned cpu;
	unsigned err = 0;
	struct pmc_cfg *cfg;

	for_each_present_cpu(cpu) {
		cfg = kzalloc(sizeof(struct pmc_cfg) * MAX_ID_PMC, GFP_KERNEL);
		if (!cfg) goto no_mem;
		per_cpu(pcpu_pmc_cfg, cpu) = cfg;
		// per_cpu(pcpu_head, cpu) = NULL;
		// per_cpu(pcpu_tail, cpu) = NULL;
	}

	goto out;

no_mem:
	err = cpu;
	for_each_present_cpu(cpu) {
		if (cpu >= err) goto out;
		kfree(per_cpu(pcpu_pmc_cfg, cpu));
	}
	err = -ENOMEM;
out:
	return err;
}

static void pmc_resource_dealloc(void)
{
	unsigned cpu;

	for_each_present_cpu(cpu) {
		per_cpu(pcpu_pmc_cfg, cpu) = kzalloc(sizeof(struct pmc_cfg) * MAX_ID_PMC, GFP_KERNEL);
	}
}

int pmc_config_on_cpu(struct pmc_cfg *cfg)
{
	return 0;
}


int pmu_init(void)
{
	int err = 0;

	err = pmc_resource_alloc();
	if (err) goto out;

	err = pmi_init(NMI_LINE);
	if (err) goto no_pmi;
	// pmc_cleanup();

	goto out;
no_pmi:
	pmc_resource_dealloc();
out:
	return err;
}

void pmu_fini(void)
{
	pmi_fini();
	pmc_resource_dealloc();
}

static void pmc_shutdown_on_cpu(void* dummy)
{
	int pmc_id; 
	preempt_disable();
	for(pmc_id = 0; pmc_id < MAX_ID_PMC; pmc_id++){
		wrmsrl(MSR_IA32_PERFEVTSEL(pmc_id), 0ULL);
		wrmsrl(MSR_IA32_PMC(pmc_id), 0ULL);
	}
	wrmsrl(MSR_IA32_PERF_GLOBAL_CTRL, 0ULL);
	wrmsrl(MSR_IA32_PEBS_ENABLE, 0ULL);
	wrmsrl(MSR_IA32_FIXED_CTR_CTRL, 0ULL);
	preempt_enable();
}

void pmc_cleanup(void)
{
	on_each_cpu(pmc_shutdown_on_cpu, NULL, 1);
}

void pmc_on_on_cpu(void* dummy)
{
	preempt_disable();
	wrmsrl(MSR_IA32_PERF_GLOBAL_CTRL, 0xfULL | BIT(32) | BIT(33));
	preempt_enable();
}

int pmc_on(void)
{
	on_each_cpu(pmc_on_on_cpu, NULL, 1);
	return 0;
}

void pmc_off_on_cpu(void* dummy)
{
	preempt_disable();
	wrmsrl(MSR_IA32_PERF_GLOBAL_CTRL, 0ULL);
	preempt_enable();
}

int pmc_off(void)
{
	on_each_cpu(pmc_off_on_cpu, NULL, 1);
	return 0;
}

static void pmc_reset_on_cpu(void* arg)
{
	int cpu_id, k;//, offcore;
	struct pmc_cfg * cfg;

	pr_info("RESET\n");
	// configuration_t* args = (configuration_t*) arg;
	cpu_id = smp_processor_id();
	// if(args[cpu_id].valid_cpu == 0) return;
	wrmsrl(MSR_IA32_FIXED_CTR_CTRL, 0ULL);
	for(k = 0; k < MAX_ID_PMC; k++){
		// if(!args[cpu_id].pmcs[k].valid) continue;
		cfg = get_pmc_config(k, cpu_id);
		// offcore = args[cpu_id].pmcs[k].offcore;
		// switch(offcore){
		// 	case 1:
		// 		wrmsrl(MSR_OFFCORE_RSP_0, 0ULL);
		// 		break;
			
		// 	case 2:
		// 		wrmsrl(MSR_OFFCORE_RSP_1, 0ULL);
		// 		break;
		// }
		cfg->perf_evt_sel = 0ULL;
		cfg->pebs = 0;
		cfg->counter = 0;
		cfg->reset = 0;
		wrmsrl(MSR_IA32_PERFEVTSEL(k), cfg->perf_evt_sel);
		wrmsrl(MSR_IA32_PMC(k), cfg->counter);
	}
}

int pmc_reset(void)
{
	on_each_cpu(pmc_reset_on_cpu, NULL, 1);
	return 0;
}


static void pmc_setup_on_core(void* arg)
{
	int cpu_id, k;//, offcore;
	struct pmc_cfg * cfg;
	// u64 msr;
	// configuration_t* args = (configuration_t*) arg;
	cpu_id = smp_processor_id();
	// if(args[cpu_id].valid_cpu == 0) return;
	// if(!args[cpu_id].fixed_value) wrmsrl(MSR_IA32_FIXED_CTR_CTRL, BIT(1) | BIT(5));
	// else{
		// this_cpu_write(fixed_reset, args[cpu_id].fixed_value & FIXED_MASK);
		// wrmsrl(MSR_IA32_FIXED_CTR_CTRL, BIT(1) | BIT(5) | BIT(7));
		// wrmsrl(IA32_PERF_FIXED_CTR1, msr | __this_cpu_read(fixed_reset));
	// }
	for(k = 0; k < MAX_ID_PMC; k++){
		// if(!args[cpu_id].pmcs[k].valid) continue;
		cfg = get_pmc_config(k, cpu_id);
		// offcore = args[cpu_id].pmcs[k].offcore;
		// switch(offcore){
		// 	case 1:
		// 		cfg->perf_evt_sel = MSR_OFFCORE_RSP_0_EVENT;
		// 		// wrmsrl(MSR_OFFCORE_RSP_0, args[cpu_id].pmcs[k].event);
		// 		break;
			
		// 	case 2:
		// 		cfg->perf_evt_sel = MSR_OFFCORE_RSP_1_EVENT;
		// 		// wrmsrl(MSR_OFFCORE_RSP_1, args[cpu_id].pmcs[k].event);
		// 		break;
				
		// 	default:	
		// 		// cfg->perf_evt_sel = args[cpu_id].pmcs[k].event;
		// 		break;
		// }

		//cfg.usr = args[cpu_id].pmcs[k].user;
		//cfg.os = args[cpu_id].pmcs[k].kernel;
		// if(args[cpu_id].pmcs[k].enable_PEBS == 0)cfg->pmi = 1;
		cfg->en = 1; 
		cfg->usr = 1;
		cfg->os = 0;
		// cfg->pebs = args[cpu_id].pmcs[k].enable_PEBS;
		//TODO reset e start
		// cfg->counter = args[cpu_id].pmcs[k].start_value;
		// cfg->reset = args[cpu_id].pmcs[k].reset_value;
		wrmsrl(MSR_IA32_PERFEVTSEL(k), cfg->perf_evt_sel);
		wrmsrl(MSR_IA32_PMC(k), cfg->counter);
	}
}

int pmc_setup(void)
{
	on_each_cpu(pmc_setup_on_core, NULL, 1);
	return 0;
}