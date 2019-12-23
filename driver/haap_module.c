#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm/current.h>

#include "irq_facility.h"
#include "ime_pebs.h"

#include "intel/pmu.h"
#include "device/device.h"

#include "haap_fops.h"
#include "dependencies.h"

unsigned long long audit = 0;
int sample = 0;

static __init int haap_module_init(void)
{
	int err = 0;
	//check_for_pebs_support();

	struct hook_pair hooks;

	pmc_cleanup();
	//enable_nmi();
	// irq_init();
	// pmc_init();
	pmu_init();

	device_init(&module_fops);

	// TODO
	init_pebs_struct();
	
	hooks.func_pos = pmc_on_on_cpu;
	hooks.func_neg = pmc_off_on_cpu;

	hook_register(&hooks);

	// switch_hook_init();
	// switch_hook_add_func((void *) hook_function);

	// err = tracker_init();
	if (err) goto end;

	pr_info("HAAP module loaded\n");

end:
	return err;
}// haap_module_init

static void __exit haap_module_exit(void)
{
	// tracker_fini();
	// switch_hook_fini();

	// TODO it should finilize the switch hook
	switch_hook_pause();

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
