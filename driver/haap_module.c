#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm/current.h>

#include "tracker.h"
#include "irq_facility.h"
#include "ime_pebs.h"
#include "ime_joe.h"

#include "switcher/switch_hook.h"
#include "intel/pmu.h"
#include "device/device.h"

#include "haap_fops.h"

unsigned long long audit = 0;
int sample = 0;

void hook_function(void)
{
	// u64 msr;
	preempt_disable();

	if (!iso_struct.state)
		goto off;
	//pr_info("current tgid: %lu ~~ profiled: %lu\n", current->pid, __this_cpu_read(pcpu_profiled));
	if (is_pid_present(current->pid)) {
	// if(current->pid == __this_cpu_read(pcpu_profiled)){
		//pr_info("\n\n\nENABLE\n\n\n");
		pmc_on_on_cpu(NULL);
		goto end;
	}
	
off:
	pmc_off_on_cpu(NULL);
end:
	//put_cpu(); /* enable preemption */
	preempt_enable();
}


static __init int haap_module_init(void)
{
	int err = 0;
	//check_for_pebs_support();

	pmc_cleanup();
	//enable_nmi();
	// irq_init();
	// pmc_init();
	pmu_init();

	device_init(&module_fops);

	// TODO
	init_pebs_struct();
	
	switch_hook_init();
	switch_hook_add_func((void *) hook_function);

	err = tracker_init();
	if (err) goto end;

	pr_info("IME module loaded\n");

end:
	return err;
}// haap_module_init

static void __exit haap_module_exit(void)
{
	tracker_fini();
	switch_hook_fini();

	// TODO
	exit_pebs_struct();
	// disable_nmi();

	device_fini();

	pmu_fini();
	// pmc_fini();
	// irq_fini();
	pmc_cleanup();
	pr_info("IME module removed %u\n", sample);
}// haap_module_exit


module_init(haap_module_init);
module_exit(haap_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Stefano Carna'");
