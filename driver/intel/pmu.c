#include <linux/preempt.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <asm/msr.h>

#include "pmu.h"
#include "specs/msr.h"
#include "pmi.h"

DEFINE_PER_CPU(struct pmc_cfg *, pcpu_pmc_cfg);

// check at runtime
inline unsigned pmc_max_available(void)
{
	return 4;
}

struct pmc_cfg *get_pmc_config(unsigned pmc_id, unsigned cpu)
{
	/* Incorrect PMC ID */
	if (pmc_id < 0 || pmc_id >= pmc_max_available() || 
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
		cfg = kzalloc(sizeof(struct pmc_cfg) * pmc_max_available(), GFP_KERNEL);
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
		per_cpu(pcpu_pmc_cfg, cpu) = kzalloc(sizeof(struct pmc_cfg) * pmc_max_available(), GFP_KERNEL);
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
	for(pmc_id = 0; pmc_id < pmc_max_available(); pmc_id++){
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

static void __pmc_on_on_cpu(void* dummy)
{
	preempt_disable();
	wrmsrl(MSR_IA32_PERF_GLOBAL_CTRL, 0xfULL | BIT(32) | BIT(33));
	preempt_enable();
}

void pmc_on_on_cpu(void)
{
	__pmc_on_on_cpu(NULL);
}

int pmc_on(void)
{
	on_each_cpu(__pmc_on_on_cpu, NULL, 1);
	return 0;
}

static void __pmc_off_on_cpu(void* dummy)
{
	preempt_disable();
	wrmsrl(MSR_IA32_PERF_GLOBAL_CTRL, 0ULL);
	preempt_enable();
}

void pmc_off_on_cpu()
{
	__pmc_off_on_cpu(NULL);
}

int pmc_off(void)
{
	on_each_cpu(__pmc_off_on_cpu, NULL, 1);
	return 0;
}

static void pmc_reset_on_cpu(void* arg)
{
	int k;//, offcore;

	pr_info("[CPU %u] pmc_reset_on_cpu\n", smp_processor_id());

	wrmsrl(MSR_IA32_FIXED_CTR_CTRL, 0ULL);

	for(k = 0; k < pmc_max_available(); k++){
		wrmsrl(MSR_IA32_PERFEVTSEL(k), 0ULL);
		wrmsrl(MSR_IA32_PMC(k), 0ULL);
	}
}

int pmc_reset(void)
{
	on_each_cpu(pmc_reset_on_cpu, NULL, 1);
	return 0;
}

static int pmc_pull_state_on_cpu(void *dummy)
{
	int k;
	struct pmc_cfg *cur_cfg = this_cpu_read(pcpu_pmc_cfg);

	for (k = 0; k < pmc_max_available(); ++k) {
		rdmsrl(MSR_IA32_PERFEVTSEL(k), cur_cfg[k].perf_evt_sel);
		rdmsrl(MSR_IA32_PMC(k), cur_cfg[k].counter);
	}

	return 0;
	// rdmsrl(IA32_PERF_FIXED_CTR0, cfg[0]->perf_evt_sel);
	// rdmsrl(IA32_PERF_FIXED_CTR0, cfg[1]->perf_evt_sel);
	// rdmsrl(MSR_IA32_PERFEVTSEL(0), cfg[2]->perf_evt_sel);
	// rdmsrl(MSR_IA32_PMC(0), cfg[2]->counter);
}

static void pmc_push_state(void)
{
	int k;
	struct pmc_cfg *cur_cfg = this_cpu_read(pcpu_pmc_cfg);

	for (k = 0; k < pmc_max_available(); ++k) {
		wrmsrl(MSR_IA32_PERFEVTSEL(k), cur_cfg[k].perf_evt_sel);
		wrmsrl(MSR_IA32_PMC(k), cur_cfg[k].counter);
	}		
}

static void pmc_setup_on_core(void* arg)
{
	int cpu_id, k;//, offcore;
	struct pmc_cpu_cfg *cpu_cfg = arg;
	struct pmc_cfg *cur_cfg = this_cpu_read(pcpu_pmc_cfg);
	struct pmc_cfg *cfg;
	
	if (!cpu_cfg) return;

	// u64 msr;
	// configuration_t* args = (configuration_t*) arg;
	cpu_id = smp_processor_id();

	for (k = 0, cfg = cpu_cfg->pmcs; k < pmc_max_available() && k < cpu_cfg->nr_pmcs; ++k, ++cfg) {
		if (cfg->valid) {
			pmc_deep_copy(&cur_cfg[k], cfg);
		} else {
			cur_cfg[k].perf_evt_sel = 0ULL;
			cur_cfg[k].counter = 0;
			cur_cfg[k].valid = 0;
		}
	}

	pr_info("End at k: %u\n", k);

	for (; k < pmc_max_available(); ++k) {
		cur_cfg[k].valid = 0;
	}

	pmc_push_state();
}

int pmc_setup(struct pmc_cpu_cfg *cfg)
{
	on_each_cpu(pmc_setup_on_core, cfg, 1);
	return 0;
}

static void pmc_print_status_on_syslog_on_cpu(void *dummy)
{
	u64 msr;
	int cpu_id, k;

	cpu_id = smp_processor_id();
	
	for (k = 0; k < 4; k++){
		rdmsrl(MSR_IA32_PERFEVTSEL(k), msr);
		pr_info("[CPU%d] MSR_IA32_PERFEVTSEL%d: %llx\n", cpu_id, k, msr);
		rdmsrl(MSR_IA32_PMC(k), msr);
		pr_info("[CPU%d] PMC%d: %llx\n", cpu_id, k, msr);
	}
}

void pmc_print_status_on_syslog(void)
{
	on_each_cpu(pmc_print_status_on_syslog_on_cpu, NULL, 1);
}

// static int __pmc_get_status_on_cpu(void *arg)
// {
// 	struct pmc_cfg **cfg = (struct pmc_cfg **)arg;
// 	rdmsrl(IA32_PERF_FIXED_CTR0, cfg[0]->perf_evt_sel);
// 	rdmsrl(IA32_PERF_FIXED_CTR0, cfg[1]->perf_evt_sel);
// 	rdmsrl(MSR_IA32_PERFEVTSEL(0), cfg[2]->perf_evt_sel);
// 	rdmsrl(MSR_IA32_PMC(0), cfg[2]->counter);
// 	return 0;
// }

struct pmc_cpu_cfg *pmc_get_status_on_cpu(unsigned cpu)
{
	int k;
	struct pmc_cfg *cur_cfg;
	struct pmc_cpu_cfg *cpu_cfg = kzalloc(sizeof(struct pmc_cpu_cfg), GFP_KERNEL);
	
	if (!cpu_cfg) {
		pr_warn("Something wrogn while allocating memory...\n");
		goto err_mem;
	}
	
	cpu_cfg->nr_pmcs = pmc_max_available();

	cpu_cfg->pmcs = kzalloc(cpu_cfg->nr_pmcs * sizeof(struct pmc_cfg), GFP_KERNEL);

	if (!cpu_cfg->pmcs) {
		pr_warn("Something wrogn while allocating memory...\n");
		goto err_mem;
	}

	smp_call_on_cpu(cpu, pmc_pull_state_on_cpu, NULL, 0);

	cur_cfg = this_cpu_read(pcpu_pmc_cfg);

	for (k = 0; k < cpu_cfg->nr_pmcs; ++k) {
		pmc_deep_copy(&cpu_cfg->pmcs[k], &cur_cfg[k]);
	}

	return cpu_cfg;
err_mem:
	return NULL;
}