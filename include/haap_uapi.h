#ifndef _HAAP_UAPI_H
#define _HAAP_UAPI_H


/* Use 'j' as magic number */
#define HAAP_IOC_MAGIC			'j'

#define _IO_NB	1

#define HAAP_PMC_SETUP			_IO(HAAP_IOC_MAGIC, _IO_NB)
#define HAAP_PROFILER_ON		_IO(HAAP_IOC_MAGIC, _IO_NB + 1)
#define HAAP_PROFILER_OFF		_IO(HAAP_IOC_MAGIC, _IO_NB + 2)
#define HAAP_PMC_RESET			_IO(HAAP_IOC_MAGIC, _IO_NB + 3)

#define HAAP_DATA_READ			_IO(HAAP_IOC_MAGIC, _IO_NB + 5)
// TODO
#define HAAP_READ_STATUS		_IO(HAAP_IOC_MAGIC, _IO_NB + 4)
#define HAAP_RESET_BUFFER		_IO(HAAP_IOC_MAGIC, _IO_NB + 6)
#define HAAP_SIZE_BUFFER		_IO(HAAP_IOC_MAGIC, _IO_NB + 7)

// Related to context switcher
#define HAAP_SWITCH_ON			_IO(HAAP_IOC_MAGIC, _IO_NB + 8)
#define HAAP_SWITCH_OFF			_IO(HAAP_IOC_MAGIC, _IO_NB + 9)
#define HAAP_PID_ADD			_IO(HAAP_IOC_MAGIC, _IO_NB + 10)
#define HAAP_PID_DEL			_IO(HAAP_IOC_MAGIC, _IO_NB + 11)
// TODO 
#define HAAP_EVT_STATS			_IO(HAAP_IOC_MAGIC, _IO_NB + 12)
#define HAAP_SIZE_STATS			_IO(HAAP_IOC_MAGIC, _IO_NB + 13)
#define HAAP_WAIT			_IO(HAAP_IOC_MAGIC, _IO_NB + 14)
// Re-arrange
#define HAAP_DEBUG_PRINT		_IO(HAAP_IOC_MAGIC, _IO_NB + 15)

struct pmc_conf {
	uint64_t start;
	uint64_t reset;
	unsigned user;
	unsigned kernel;
	unsigned counting;
	
	uint64_t event;
	
	unsigned pebs;

	unsigned fixed;
	// unsigned offcore;
};

struct pmc_conf_list {
	uint64_t cpu_mask;
	unsigned nr_pmcs;
	struct pmc_conf *pmcs;

	unsigned nr_fxd_pmcs;
	struct pmc_conf *fxd_pmcs;
	
	/* Disabled if 0 */
	uint64_t sampling_period;
	// unsigned nr_off_pmcs;
	// struct pmc_conf *off_pmcs;
};

struct event_stat {
	uint64_t event;
	uint64_t value;
};

struct event_stat_list {
	unsigned size;
	struct event_stat *pmcs;
};

struct cpu_pmc_sample {
	unsigned cpu_id;
	struct event_stat *pmcs;	
	unsigned nr_pmcs;
};

struct cpu_pmc_sample_list {
	struct cpu_pmc_sample *cpus;	
	unsigned nr_cpus;
};

#endif /* _HAAP_UAPI_H */