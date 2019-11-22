#include <asm/apic.h>

#include "ime_handler.h"
#include "intel/specs/msr.h"
#include "ime_pebs.h"
#include "haap_fops.h"
#include "irq_facility.h"

u64 samples_pmc = 0;

static inline int handle_ime_event(struct pt_regs *regs)
{
	u64 msr;
	u64 global, reset; 
	int i;
	pr_info("IN NMI HANDLER\n");
	rdmsrl(MSR_IA32_PERF_GLOBAL_STATUS, global);
	if(global & BIT(62)){ 
		wrmsrl(MSR_IA32_PERF_GLOBAL_STATUS_RESET, BIT(62));
		write_buffer();

		for(i = 0; i < MAX_ID_PMC; i++){
			rdmsrl(MSR_IA32_PMC(i), msr);
			reset = get_reset_value(i);
			if(msr < reset){
				wrmsrl(MSR_IA32_PMC(i), reset);
			}
		}
		return 1;
	}
	if(global & 0xfULL){
		int k = 0;
		for(i = 0; i < MAX_ID_PMC; i++){
			if(global & BIT(i)){ 
				wrmsrl(MSR_IA32_PMC(i), get_reset_value(i));
				wrmsrl(MSR_IA32_PERF_GLOBAL_STATUS_RESET, BIT(i));
				k++;
			}
		}
		return k;
	}
	return 0;
}// handle_ibs_event


int handle_ime_nmi(unsigned int cmd, struct pt_regs *regs)
{
	return handle_ime_event(regs);
}// handle_ime_nmi
