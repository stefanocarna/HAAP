#ifndef _PMU_H
#define _PMU_H

#include <linux/types.h>

#define pmc_deep_copy(x, y) ({ \
		(x)->perf_evt_sel = (y)->perf_evt_sel; \
		(x)->reset = (y)->reset; \
		(x)->counter = (y)->counter; \
		(x)->pebs = (y)->pebs; \
		(x)->valid = (y)->valid; \
		(x)->fixed = (y)->fixed; \
	})

#define pmc_deep_copy_negate(x, y) ({ \
		(x)->perf_evt_sel = (y)->perf_evt_sel; \
		(x)->reset = ~(y)->reset; \
		(x)->counter = ~(y)->counter; \
		(x)->pebs = (y)->pebs; \
		(x)->valid = (y)->valid; \
		(x)->fixed = (y)->fixed; \
	})

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

	unsigned fixed;
	// u64 cpu_mask;
} __attribute__((packed));

struct pmc_cpu_cfg {
	unsigned  cpu;

	unsigned nr_fxd_pmcs;
	struct pmc_cfg *fxd_pmcs;
	
	unsigned nr_pmcs;
	struct pmc_cfg *pmcs;
} __attribute__((packed));

struct pmc_data {
	unsigned long event;
	unsigned long value;
};

struct pmc_data_sample {
	unsigned long tsc;
	unsigned nr_pmcs;
	struct pmc_data *pmcs;
	struct pmc_data_sample *next;
};

struct pmu_core_state {
	unsigned enabled;
	unsigned fixed0_sampling;
};

extern struct pmu_core_state core_state;

extern void profiler_sampling_enable(void);

extern void profiler_sampling_disable(void);

extern __percpu struct pmc_cfg *pcpu_pmc_cfg;
extern __percpu struct pmc_data_sample *pcpu_pmc_data_sample;
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

extern void reset_sampling_fixed0(void);

/* Require a list of pmc_cfg whose last element is NULL */
extern int pmc_setup(struct pmc_cpu_cfg *); // TODO rethink

// TODO change function signature
extern void pmc_setup_mode(u64 sampling_rate);

extern void pmc_print_status_on_syslog(void);

// extern void pmc_get_status(void);

extern struct pmc_cpu_cfg *pmc_get_status_on_cpu(unsigned cpu);

extern int pmc_set_periodic_sampling(void);

extern int pmc_record_active_sample(int irq_context);

extern void pmc_force_reset_value_on_cpu(void *arg);

/* PEBS stuff */

struct pebs_sample {
	u64 eflags;		// 0x00
	u64 eip;		// 0x08
	u64 eax;		// 0x10
	u64 ebx;		// 0x18
	u64 ecx;		// 0x20
	u64 edx;		// 0x28
	u64 esi;		// 0x30
	u64 edi;		// 0x38
	u64 ebp;		// 0x40
	u64 esp;		// 0x48
	u64 r8;			// 0x50
	u64 r9;			// 0x58
	u64 r10;		// 0x60
	u64 r11;		// 0x68
	u64 r12;		// 0x70
	u64 r13;		// 0x78
	u64 r14;		// 0x80
	u64 r15;		// 0x88
	u64 stat;		// 0x90 IA32_PERF_GLOBAL_STATUS
	u64 add;		// 0x98 Data Linear Address
	u64 enc;		// 0xa0 Data Source Encoding
	u64 lat;		// 0xa8 Latency value (core cycles)
	u64 eventing_ip;	// 0xb0 EventingIP
	u64 tsx;		// 0xb8 tx Abort Information
	u64 tsc;		// 0xc0	TSC
};				// 0xc8

struct debug_store {
	u64 bts_buffer_base;			// 0x00 
	u64 bts_index;				// 0x08
	u64 bts_absolute_maximum;		// 0x10
	u64 bts_interrupt_threshold;		// 0x18
	struct pebs_sample *pebs_buffer_base;		// 0x20
	struct pebs_sample *pebs_index;			// 0x28
	struct pebs_sample *pebs_absolute_maximum;	// 0x30
	struct pebs_sample *pebs_interrupt_threshold;	// 0x38
	u64 pebs_counter0_reset;		// 0x40
	u64 pebs_counter1_reset;		// 0x48
	u64 pebs_counter2_reset;		// 0x50
	u64 pebs_counter3_reset;		// 0x58
	u64 reserved;				// 0x60
};

struct pebs_buffer {
    int usage;
    int allocated;
    unsigned nr_samples;
    struct pebs_sample *samples;
};

// TODO Setup DS Area

extern void pebs_ds_update_on_cpu(void *);

extern int pebs_init(void);

extern void pebs_fini(void);

extern void pebs_buffer_store_n_replace(void);

extern void pebs_print_status_on_syslog(void);

#endif /* _PMU_H */