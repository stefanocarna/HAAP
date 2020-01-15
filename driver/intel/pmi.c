#include <linux/percpu-defs.h>
#include <linux/smp.h>
#include <asm/apic.h>
#include <asm/nmi.h>

#include "pmu.h"
#include "pmi.h"

#include "specs/msr.h"
#include "dependencies.h"

static u8 pmi_line = 0;
static DEFINE_PER_CPU(u32, pcpu_lvt_bkp);

int pmi_nmi_handler(unsigned int cmd, struct pt_regs *regs);
int pmi_irq_handler(void);

// TODO the pmc IRQ is hard-coded to 241
static void pmi_lvt_setup_on_cpu(void *line)
{
	u8 irq_line = *((u8*) line);
	(*this_cpu_ptr(&pcpu_lvt_bkp)) = apic_read(APIC_LVTPC);
	// per_cpu(pcpu_lvt_bkp, smp_processor_id()) = apic_read(APIC_LVTPC);

	if (irq_line == NMI_LINE) {
		apic_write(APIC_LVTPC, LVT_NMI | (u8)irq_line);
	} else {
		apic_write(APIC_LVTPC, (u8)irq_line);		
	}
}

static void pmi_lvt_cleanup_on_cpu(void *dummy)
{
	/* Restore old LVT entry */
	apic_write(APIC_LVTPC, (*this_cpu_ptr(&pcpu_lvt_bkp)));
}


/* Setup the PMI's NMI handler */
static int pmi_nmi_setup(void)
{
	int err = 0;
	
	static struct nmiaction handler_na;

	if (err) goto out;

	handler_na.handler = pmi_nmi_handler;
	handler_na.name = NMI_NAME;
	handler_na.flags = NMI_FLAG_FIRST;

	err = __register_nmi_handler(NMI_LOCAL, &handler_na);
	pr_info("__register_nmi_handler done\n");

	// if (!err)
	// 	on_each_cpu(pmi_lvt_setup_on_cpu, NMI_LINE, 1);
	/* Try to remove default system PMI handler */
	// if (!err) 
	// 	unregister_nmi_handler(NMI_LOCAL, "perf_event_nmi_handler");
out:
	return err;
}

static void pmi_nmi_cleanup(void)
{
	unregister_nmi_handler(NMI_LOCAL, NMI_NAME);
	pr_info("unregister_nmi_handler done\n");
}

static int pmi_irq_setup(void)
{
	// TODO check IRQ line dynamically
	idt_patcher_install_entry((unsigned long) pmi_irq_entry, 241, 3);
	return 0;
}

static void pmi_irq_cleanup(void)
{
	// TODO Not implemented
	// Actually this method does not anything
	// Wait untill the module unload
}

int pmi_init(u8 type)
{
	int err = 0;
	if (type == NMI_LINE) {
		pr_info("pmi_init\n");
		err = pmi_nmi_setup();
		pr_info("pmi_nmi_setup\n");
		if (err) goto out;
		on_each_cpu(pmi_lvt_setup_on_cpu, &type, 1);
		// init nmi
	} else if (type > RESERVED_LINES) {
		err = pmi_irq_setup();
	} else {
		err = -1;
	}

	if (!err) pmi_line = type;

out:
	return err;
}

void pmi_fini(void)
{
	pr_info("pmi_fini: pmi_line = %u\n", pmi_line);
	on_each_cpu(pmi_lvt_cleanup_on_cpu, NULL, 1);

	if (pmi_line == NMI_LINE) {
		pmi_nmi_cleanup();
	} else if (pmi_line > RESERVED_LINES) {
		pmi_irq_cleanup();
	}
}

/*
 * Performance Monitor Interrupt handler
 * Can be used by both NMI and IRQ wrappers
 */
static inline int pmi_handler(unsigned apic_value)
{

	u64 global;//, msr, reset;
	int i, k = 0;
	pr_info("IN NMI HANDLER\n");
	rdmsrl(MSR_IA32_PERF_GLOBAL_STATUS, global);

	// PEBS check
	// if(global & BIT(62)){ 
	// 	wrmsrl(MSR_IA32_PERF_GLOBAL_STATUS_RESET, BIT(62));
	// 	// write_buffer();

	// 	for(i = 0; i < pmc_max_available(); i++){
	// 		rdmsrl(MSR_IA32_PMC(i), msr);
	// 		reset = get_reset_value(i);
	// 		if(msr < reset){
	// 			wrmsrl(MSR_IA32_PMC(i), reset);
	// 		}
	// 	}
	// 	return 1;
	// }

	// Plain PMCs check
	/* Check for PMI coming from PMCs */
	if (global & 0xFULL) {
		for (i = 0; i < pmc_max_available(); i++) {
			if (global & BIT_ULL(i)) {
				pr_info("Look for: %u - %llx\n", i, BIT_ULL(i));
				// reset the single pmc value
				// wrmsrl(MSR_IA32_PMC(i), get_reset_value(i));
				wrmsrl(MSR_IA32_PMC(i), ~0xFFFFULL);

				k++;
			}
		}
	}

	pmc_record_active_sample(1);


	/* Ack the PMI in the APIC */
	apic_write(APIC_LVTPC, apic_value);
	
	/* Reset the instance */
	wrmsrl(MSR_IA32_PERF_GLOBAL_STATUS_RESET, global);

	// int i;
	// int cpu_id;
	// u64 msr;
	// struct statistics *stat, *head, *tail;
	// preempt_disable();
	// sample++;
	goto end;


	// cpu_id = smp_processor_id();
	// stat = kzalloc(sizeof(struct statistics), GFP_ATOMIC);
	// if (!stat){ 
	// 	pr_info("Failed kzalloc STAT\n");
	// 	return -ENOMEM;
	// }
	// head = per_cpu(pcpu_head, cpu_id);
	// tail = per_cpu(pcpu_tail, cpu_id);

	// // Does it make sense?
	// rdmsrl(IA32_PERF_FIXED_CTR0, msr);
	// stat->fixed0 = msr;
	// rdmsrl(IA32_PERF_FIXED_CTR1, msr);
	// stat->fixed1 = ~(__this_cpu_read(fixed_reset)) & FIXED_MASK;
	// for(i = 0; i < MAX_ID_PMC; i++){
	// 	rdmsrl(MSR_IA32_PMC(i), msr);
	// 	stat->event[i].stat = msr;
	// }
	// stat->next = NULL;
	// //add the new struct to the linked list
	// if(per_cpu(pcpu_head, cpu_id) == NULL){
	// 	per_cpu(pcpu_head, cpu_id) = stat;
	// 	per_cpu(pcpu_tail, cpu_id) = stat;
	// }
	// else{
	// 	per_cpu(pcpu_tail, cpu_id)->next = stat;
	// 	per_cpu(pcpu_tail, cpu_id) = stat;
	// }
	// __sync_fetch_and_add(&size_stat, 1);

	//reset PMcs
	// wrmsrl(IA32_PERF_FIXED_CTR0, 0ULL);
	// wrmsrl(IA32_PERF_FIXED_CTR1, __this_cpu_read(fixed_reset));
	// for(i = 0; i < MAX_ID_PMC; i++){
	// 	wrmsrl(MSR_IA32_PMC(i), 0ULL);
	// }

end:
	return k;

	// pr_info("PMI done on cpu %x\n", get_cpu());
	// put_cpu();
	// preempt_enable();
}

asm(	".globl pmi_irq_entry\n"
	"pmi_irq_entry:\n"
	"	cld\n"
	"	testq $3,8(%rsp)\n"
	"	jz 1f\n"
	"	swapgs\n"
	"1:\n"
	"	pushq $0\n" /* error code */
	"	pushq %rdi\n"
	"	pushq %rsi\n"
	"	pushq %rdx\n"
	"	pushq %rcx\n"
	"	pushq %rax\n"
	"	pushq %r8\n"
	"	pushq %r9\n"
	"	pushq %r10\n"
	"	pushq %r11\n"
	"	pushq %rbx\n"
	"	pushq %rbp\n"
	"	pushq %r12\n"
	"	pushq %r13\n"
	"	pushq %r14\n"
	"	pushq %r15\n"
	"	call pmi_irq_handler\n"
	"	popq %r15\n"
	"	popq %r14\n"
	"	popq %r13\n"
	"	popq %r12\n"
	"	popq %rbp\n"
	"	popq %rbx\n"
	"	popq %r11\n"
	"	popq %r10\n"
	"	popq %r9\n"
	"	popq %r8\n"
	"	popq %rax\n"
	"	popq %rcx\n"
	"	popq %rdx\n"
	"	popq %rsi\n"
	"	popq %rdi\n"
	"	addq $8,%rsp\n" /* error code */
	"	testq $3,8(%rsp)\n"
	"	jz 2f\n"
	"	swapgs\n"
	"2:\n"
	"	iretq");


// u64 samples_pmc = 0;

// static inline int handle_ime_event(struct pt_regs *regs)
// {
// 	u64 msr;
// 	u64 global, reset; 
// 	int i;
// 	pr_info("IN NMI HANDLER\n");
// 	rdmsrl(MSR_IA32_PERF_GLOBAL_STATUS, global);
// 	if(global & BIT(62)){ 
// 		wrmsrl(MSR_IA32_PERF_GLOBAL_STATUS_RESET, BIT(62));
// 		write_buffer();

// 		for(i = 0; i < MAX_ID_PMC; i++){
// 			rdmsrl(MSR_IA32_PMC(i), msr);
// 			reset = get_reset_value(i);
// 			if(msr < reset){
// 				wrmsrl(MSR_IA32_PMC(i), reset);
// 			}
// 		}
// 		return 1;
// 	}
// 	if(global & 0xfULL){
// 		int k = 0;
// 		for(i = 0; i < MAX_ID_PMC; i++){
// 			if(global & BIT(i)){ 
// 				wrmsrl(MSR_IA32_PMC(i), get_reset_value(i));
// 				wrmsrl(MSR_IA32_PERF_GLOBAL_STATUS_RESET, BIT(i));
// 				k++;
// 			}
// 		}
// 		return k;
// 	}
// 	return 0;
// }// handle_ibs_event


int pmi_nmi_handler(unsigned int cmd, struct pt_regs *regs)
{
	int evt;

	// Do we need to check the interrupt condition here?

	evt = pmi_handler(LVT_NMI);

	return evt;
}

int pmi_irq_handler(void)
{
	int evt;

	evt = pmi_handler(241);

	/* ack apic */
	apic_eoi();
	/* Unmask PMI as, as it got implicitely masked. */
	apic_write(APIC_LVTPC, 241);

	return 0;
}
