void print_reg(void)
{
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
