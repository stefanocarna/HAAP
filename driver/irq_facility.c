#include <asm/apicdef.h> 	// Intel apic constants
#include <asm/desc.h>
#include <asm/apic.h>
#include <asm/nmi.h>
#include <linux/kprobes.h>
#include <linux/slab.h>

#include "tracker.h"
#include "intel/specs/msr.h"
#include "irq_facility.h"
#include "ime_handler.h"
#include "haap_fops.h"
#include "ime_joe.h"
#include "../include/ime-ioctl.h"

static int system_hooked = 0;
extern u64 size_stat;
DEFINE_PER_CPU(u64, scheduled) = 0;
DEFINE_PER_CPU(u32, pcpu_lvt_bkp);

// static void pmc_init_on_cpu(void *dummy)
// {
// 	per_cpu(pcpu_lvt_bkp, smp_processor_id()) = apic_read(APIC_LVTPC); 
// 	apic_write(APIC_LVTPC, 241);
// }

// int pmc_init(void)
// {
// 	on_each_cpu(pmc_init_on_cpu, NULL, 1);
// 	return 0;
// }

// static void pmc_fini_on_cpu(void *dummy)
// {
// 	/* Restore LVT entry */
// 	apic_write(APIC_LVTPC, per_cpu(pcpu_lvt_bkp, smp_processor_id()));
// }

// void pmc_fini(void)
// {
// 	on_each_cpu(pmc_fini_on_cpu, NULL, 1);
// }

static int switch_post_handler(struct kretprobe_instance *ri, struct pt_regs *regs);

static struct kretprobe krp = {
	.handler = switch_post_handler,
};

static int switch_post_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{	
	// u64 msr;
	if (!iso_struct.state)
		goto off;
	//pr_info("current tgid: %lu ~~ profiled: %lu\n", current->pid, __this_cpu_read(pcpu_profiled));
	if (is_pid_present(current->pid)) {
	// if(current->pid == __this_cpu_read(pcpu_profiled)){
		//pr_info("\n\n\nENABLE\n\n\n");
		goto end;
	}
	
off:
end:
	//put_cpu(); /* enable preemption */
	preempt_enable();
	return 0;
}// post_handler



int switch_patcher_init (unsigned long func_addr)
{
	int ret;

	krp.kp.symbol_name = SWITCH_FUNC;
	ret = register_kretprobe(&krp);
	if (ret < 0) {
		pr_info("hook init failed, returned %d\n", ret);
		goto no_probe;
	};

	pr_info("hook module correctly loaded\n");
	system_hooked = 1;
	
	return 0;

no_probe:
	return ret;
}// hook_init

void switch_patcher_exit(void)
{
	if (!system_hooked) {
		pr_info("The system is not hooked\n");
		goto end;
	} 

	unregister_kretprobe(&krp);

	/* nmissed > 0 suggests that maxactive was set too low. */
	if (krp.nmissed) pr_info("Missed %u invocations\n", krp.nmissed);

	pr_info("hook module unloaded\n");
end:
	return;
}// hook_exit


static void setup_ime_lvt(void *err)
{
	apic_write(APIC_LVTPC, BIT(10));
	return;
} // setup_ime_lvt

int setup_ime_nmi(int (*handler) (unsigned int, struct pt_regs*))
{
	int err = 0;
	static struct nmiaction handler_na;

	on_each_cpu(setup_ime_lvt, &err, 1);
	if (err) goto out;

	handler_na.handler = handler;
	handler_na.name = NMI_NAME;
	handler_na.flags = NMI_FLAG_FIRST;
	err = __register_nmi_handler(NMI_LOCAL, &handler_na);
out:
	return err;
}// setup_ime_nmi


void disable_nmi(void)
{
	unregister_nmi_handler(NMI_LOCAL, NMI_NAME);
}// cleanup_ime_nmi


int enable_nmi(void)
{
	setup_ime_nmi(handle_ime_nmi);
	unregister_nmi_handler(NMI_LOCAL, "perf_event_nmi_handler");
	return 0;
}// enable_pebs_on_system



void disableAllPMC(void* arg)
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

void cleanup_pmc(void){
	on_each_cpu(disableAllPMC, NULL, 1);
}

void print_reg(void){
	u64 msr;
	int i;

	rdmsrl(MSR_IA32_PERF_GLOBAL_STATUS_RESET, msr);
	pr_info("[CPU%d] MSR_IA32_PERF_GLOBAL_STATUS_RESET: %llx\n", smp_processor_id(), msr);

	rdmsrl(MSR_IA32_PERF_GLOBAL_STATUS, msr);
	pr_info("[CPU%d] MSR_IA32_PERF_GLOBAL_STATUS: %llx\n", smp_processor_id(), msr);

	rdmsrl(MSR_IA32_PERF_GLOBAL_CTRL, msr);
	pr_info("[CPU%d] MSR_IA32_PERF_GLOBAL_CTRL: %llx\n", smp_processor_id(), msr);

	rdmsrl(MSR_IA32_PEBS_ENABLE, msr);
	pr_info("[CPU%d] MSR_IA32_PEBS_ENABLE: %llx\n", smp_processor_id(), msr);

	rdmsrl(MSR_IA32_DS_AREA, msr);
	pr_info("[CPU%d] MSR_IA32_DS_AREA: %llx\n", smp_processor_id(), msr);

	rdmsrl(MSR_IA32_FIXED_CTR_CTRL, msr);
	pr_info("[CPU%d] MSR_IA32_FIXED_CTR_CTRL: %llx\n", smp_processor_id(), msr);

	for(i = 0; i < MAX_ID_PMC; i++){
		rdmsrl(MSR_IA32_PERFEVTSEL(i), msr);
		pr_info("[CPU%d] MSR_IA32_PERFEVTSEL%d: %llx\n", smp_processor_id(), i, msr);
	}
}


// /* Performance Monitor Interrupt handler */
// void pmi_handler(void)
// {
// 	int cpu_id, i;
// 	u64 msr;
// 	struct statistics *stat, *head, *tail;
// 	// preempt_disable();
// 	sample++;
// 	goto end;


// 	cpu_id = smp_processor_id();
// 	stat = kzalloc(sizeof(struct statistics), GFP_ATOMIC);
// 	if (!stat){ 
// 		pr_info("Failed kzalloc STAT\n");
// 		return -ENOMEM;
// 	}
// 	head = per_cpu(pcpu_head, cpu_id);
// 	tail = per_cpu(pcpu_tail, cpu_id);

// 	// Does it make sense?
// 	rdmsrl(IA32_PERF_FIXED_CTR0, msr);
// 	stat->fixed0 = msr;
// 	rdmsrl(IA32_PERF_FIXED_CTR1, msr);
// 	stat->fixed1 = ~(__this_cpu_read(fixed_reset)) & FIXED_MASK;
// 	for(i = 0; i < MAX_ID_PMC; i++){
// 		rdmsrl(MSR_IA32_PMC(i), msr);
// 		stat->event[i].stat = msr;
// 	}
// 	stat->next = NULL;
// 	//add the new struct to the linked list
// 	if(per_cpu(pcpu_head, cpu_id) == NULL){
// 		per_cpu(pcpu_head, cpu_id) = stat;
// 		per_cpu(pcpu_tail, cpu_id) = stat;
// 	}
// 	else{
// 		per_cpu(pcpu_tail, cpu_id)->next = stat;
// 		per_cpu(pcpu_tail, cpu_id) = stat;
// 	}
// 	__sync_fetch_and_add(&size_stat, 1);

// 	//reset PMcs
// 	wrmsrl(IA32_PERF_FIXED_CTR0, 0ULL);
// 	wrmsrl(IA32_PERF_FIXED_CTR1, __this_cpu_read(fixed_reset));
// 	for(i = 0; i < MAX_ID_PMC; i++){
// 		wrmsrl(MSR_IA32_PMC(i), 0ULL);
// 	}

// end:
// 	/* ack apic */
// 	apic_eoi();
// 	/* Unmask PMI as, as it got implicitely masked. */
// 	apic_write(APIC_LVTPC, 241);

// 	// pr_info("PMI done on cpu %x\n", get_cpu());
// 	// put_cpu();
// 	// preempt_enable();
// }

// // extern void pmi_handler(void);
// asm("	.globl pmi_entry\n"
// 	"pmi_entry:\n"
// 	"	cld\n"
// 	"	testq $3,8(%rsp)\n"
// 	"	jz 1f\n"
// 	"	swapgs\n"
// 	"1:\n"
// 	"	pushq $0\n" /* error code */
// 	"	pushq %rdi\n"
// 	"	pushq %rsi\n"
// 	"	pushq %rdx\n"
// 	"	pushq %rcx\n"
// 	"	pushq %rax\n"
// 	"	pushq %r8\n"
// 	"	pushq %r9\n"
// 	"	pushq %r10\n"
// 	"	pushq %r11\n"
// 	"	pushq %rbx\n"
// 	"	pushq %rbp\n"
// 	"	pushq %r12\n"
// 	"	pushq %r13\n"
// 	"	pushq %r14\n"
// 	"	pushq %r15\n"
// 	"1:  call pmi_handler\n"
// 	"	popq %r15\n"
// 	"	popq %r14\n"
// 	"	popq %r13\n"
// 	"	popq %r12\n"
// 	"	popq %rbp\n"
// 	"	popq %rbx\n"
// 	"	popq %r11\n"
// 	"	popq %r10\n"
// 	"	popq %r9\n"
// 	"	popq %r8\n"
// 	"	popq %rax\n"
// 	"	popq %rcx\n"
// 	"	popq %rdx\n"
// 	"	popq %rsi\n"
// 	"	popq %rdi\n"
// 	"	addq $8,%rsp\n" /* error code */
// 	"	testq $3,8(%rsp)\n"
// 	"	jz 2f\n"
// 	"	swapgs\n"
// 	"2:\n"
// 	"	iretq");
