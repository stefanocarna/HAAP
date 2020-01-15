#ifndef _IME_IOCTL_H_
#define _IME_IOCTL_H_

#define MAX_ID_PMC 4
#define MAX_ID_CPU 4
#define MAX_ID_EVENT 7
#define numCPU sysconf(_SC_NPROCESSORS_ONLN)
#define MAX_CPU 256
#define MAX_BUFFER_SIZE 65536

typedef struct{
	int valid;
	uint64_t event; 
	uint64_t start_value;
	uint64_t reset_value; 
	int offcore;
    int user;
    int kernel;
    int enable_PEBS;
}pmc_conf_t;

typedef struct{
	int valid_cpu;
	uint64_t fixed_value;
	pmc_conf_t pmcs[MAX_ID_PMC];
}configuration_t;

// struct event_stat{
// 	uint64_t event;
// 	uint64_t stat;
// };

// struct statistics_user{
// 	uint64_t fixed0;
// 	uint64_t fixed1;
// 	struct event_stat event[4];
// };

struct pebs_user{
	uint64_t eflags;	// 0x00
	uint64_t eip;	// 0x08
	uint64_t eax;	// 0x10
	uint64_t ebx;	// 0x18
	uint64_t ecx;	// 0x20
	uint64_t edx;	// 0x28
	uint64_t esi;	// 0x30
	uint64_t edi;	// 0x38
	uint64_t ebp;	// 0x40
	uint64_t esp;	// 0x48
	uint64_t r8;	// 0x50
	uint64_t r9;	// 0x58
	uint64_t r10;	// 0x60
	uint64_t r11;	// 0x68
	uint64_t r12;	// 0x70
	uint64_t r13;	// 0x78
	uint64_t r14;	// 0x80
	uint64_t r15;	// 0x88
	uint64_t stat;	// 0x90 IA32_PERF_GLOBAL_STATUS
	uint64_t add;	// 0x98 Data Linear Address
	uint64_t enc;	// 0xa0 Data Source Encoding
	uint64_t lat;	// 0xa8 Latency value (core cycles)
	uint64_t eventing_ip;	//0xb0 EventingIP
	uint64_t tsx;	// 0xb8 tx Abort Information
	uint64_t tsc;	// 0xc0	TSC
				// 0xc8
};

struct buffer_struct {
	unsigned long address;
	int times;
};

/* Use 'j' as magic number */
#define IME_IOC_MAGIC			'q'

#define _IO_NB	1

#define IME_SETUP_PMC			_IO(IME_IOC_MAGIC, _IO_NB)
#define IME_PROFILER_ON			_IO(IME_IOC_MAGIC, _IO_NB+1)
#define IME_PROFILER_OFF		_IO(IME_IOC_MAGIC, _IO_NB+2)
#define IME_RESET_PMC			_IO(IME_IOC_MAGIC, _IO_NB+3)
#define IME_READ_STATUS			_IO(IME_IOC_MAGIC, _IO_NB+4)
#define IME_READ_BUFFER			_IO(IME_IOC_MAGIC, _IO_NB+5)
#define IME_RESET_BUFFER		_IO(IME_IOC_MAGIC, _IO_NB+6)
#define IME_SIZE_BUFFER			_IO(IME_IOC_MAGIC, _IO_NB+7)
// #define JOE_PROFILER_ON			_IO(IME_IOC_MAGIC, _IO_NB+8)
// #define JOE_PROFILER_OFF		_IO(IME_IOC_MAGIC, _IO_NB+9)
// #define JOE_ADD_TID			_IO(IME_IOC_MAGIC, _IO_NB+10)
// #define JOE_DEL_TID			_IO(IME_IOC_MAGIC, _IO_NB+11)
#define IME_EVT_STATS			_IO(IME_IOC_MAGIC, _IO_NB+12)
#define IME_SIZE_STATS			_IO(IME_IOC_MAGIC, _IO_NB+13)
#define IME_WAIT			_IO(IME_IOC_MAGIC, _IO_NB+14)

#endif /* _IME_IOCTL_H_ */