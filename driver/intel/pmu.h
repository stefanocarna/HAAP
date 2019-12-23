#ifndef _PMU_H
#define _PMU_H

#define pmc_deep_copy(x, y) do { \
		(x)->perf_evt_sel = (y)->perf_evt_sel; \
		(x)->reset = (y)->reset; \
		(x)->counter = (y)->counter; \
		(x)->pebs = (y)->pebs; \
		(x)->valid = (y)->valid; \
	} while(0)

struct pmc_cfg {
	union {
		u64 perf_evt_sel;
		struct {
			u64 evt: 8, umask: 8, usr: 1, os: 1, edge: 1, pc: 1, pmi: 1, 
			any: 1, en: 1, inv: 1, cmask: 8, reserved: 32;
		};
	};
	u64 reset;
	u32 counter;
	unsigned pebs;
	unsigned valid;
	// u64 cpu_mask;
} __attribute__((packed));

struct pmc_cpu_cfg {
	unsigned  cpu;
	// insert fixed
	unsigned nr_pmcs;
	struct pmc_cfg *pmcs;
} __attribute__((packed));

extern __percpu struct pmc_cfg *pcpu_pmc_cfg;
// DEFINE_PER_CPU(struct pmc_cfg *, pcpu_pmc_cfg);

extern struct pmc_cfg *get_pmc_config(unsigned pmc_id, unsigned cpu);

extern int pmu_init(void);

extern void pmu_fini(void);

extern void pmc_cleanup(void);

extern int pmc_config_on_cpu(struct pmc_cfg *);

extern void pmc_on_on_cpu(void);

extern int pmc_on(void);

extern void pmc_off_on_cpu(void);

extern int pmc_off(void);

extern int pmc_reset(void);

extern unsigned pmc_max_available(void);

/* Require a list of pmc_cfg whose last element is NULL */
extern int pmc_setup(struct pmc_cpu_cfg *); // TODO rethink

extern void pmc_print_status_on_syslog(void);

// extern void pmc_get_status(void);

extern struct pmc_cpu_cfg *pmc_get_status_on_cpu(unsigned cpu);

#endif /* _PMU_H */