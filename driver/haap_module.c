#include <linux/module.h>
#include <linux/moduleparam.h>
#include <asm/current.h>

#include "irq_facility.h"

#include "intel/pmu.h"
#include "device/device.h"

#include "haap_fops.h"
#include "dependencies.h"

unsigned long long audit = 0;
int sample = 0;


int check_for_pebs_support(void)
{
	unsigned a, b, c, d;

	/* Architectural performance monitoring version ID */
	cpuid(0xA, &a, &b, &c, &d);

	pr_info("[CPUID_0AH] version: %u, counters: %u\n", a & 0xFF, (a >> 8) & 0XFF);

	pr_info("[CPUID_0AH] Fixed PMC bit width: %u\n", (d << 0x4) & 0XFF);

	return 0;
}

static __init int haap_module_init(void)
{
	int err = 0;
	//check_for_pebs_support();

	struct hook_pair hooks;

	check_for_pebs_support();

	pmc_cleanup();
	//enable_nmi();
	// irq_init();
	// pmc_init();
	pmu_init();

	device_init(&module_fops);
	
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
