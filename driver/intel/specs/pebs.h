struct pebs_sample
{
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
				// 0xc8
};

struct debug_store_area
{
	u64 bts_buffer_base;			// 0x00 
	u64 bts_index;				// 0x08
	u64 bts_absolute_maximum;		// 0x10
	u64 bts_interrupt_threshold;		// 0x18
	pebs_arg_t *pebs_buffer_base;		// 0x20
	pebs_arg_t *pebs_index;			// 0x28
	pebs_arg_t *pebs_absolute_maximum;	// 0x30
	pebs_arg_t *pebs_interrupt_threshold;	// 0x38
	u64 pebs_counter0_reset;		// 0x40
	u64 pebs_counter1_reset;		// 0x48
	u64 pebs_counter2_reset;		// 0x50
	u64 pebs_counter3_reset;		// 0x58
	u64 reserved;					// 0x60
};