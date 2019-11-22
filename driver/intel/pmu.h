#ifndef _PMU_H
#define _PMU_H

extern __percpu struct pmc_cfg *pcpu_pmc_cfg;
// DEFINE_PER_CPU(struct pmc_cfg *, pcpu_pmc_cfg);

extern struct pmc_cfg *get_pmc_config(unsigned pmc_id, unsigned cpu);

extern int pmu_init(void);

extern void pmu_fini(void);

extern void pmc_cleanup(void);

struct pmc_cfg {
	union {
		u64 perf_evt_sel;
		struct {
			u64 evt: 8, umask: 8, usr: 1, os: 1, edge: 1, pc: 1, pmi: 1, 
			any: 1, en: 1, inv: 1, cmask: 8, reserved: 32;
		};
	};
	u32 counter;
	u64 reset;
	unsigned pebs;
	// u64 cpu_mask;
} __attribute__((packed));

extern int pmc_config_on_cpu(struct pmc_cfg* cfg);

extern void pmc_on_on_cpu(void* dummy);

extern int pmc_on(void);

extern void pmc_off_on_cpu(void* dummy);

extern int pmc_off(void);

extern int pmc_reset(void);

extern int pmc_setup(void); // TODO rethink

#endif /* _PMU_H */