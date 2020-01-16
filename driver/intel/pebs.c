#include <linux/percpu.h>	/* Macro per_cpu */
#include <linux/slab.h>		/* Kernel memory */

#include "pmu.h"

int nBuffer_array = 10;
int nRecords_pebs = (int)(65536 / sizeof(struct pebs_sample));

DEFINE_PER_CPU(struct debug_store *, percpu_ds_bkp);
DEFINE_PER_CPU(struct debug_store, percpu_ds);
DEFINE_PER_CPU(struct pebs_buffer *, percpu_pebs_buffer);

static void pebs_resource_alloc_on_cpu(void *arg)
{
	unsigned i;
	int *err = (int *)arg;
	struct debug_store *ds = this_cpu_ptr(&percpu_ds);
	struct pebs_buffer *buffer = (struct pebs_buffer *)
		kzalloc(sizeof(struct pebs_buffer) * nBuffer_array, GFP_KERNEL);

	if (!buffer) {
		pr_err("Cannot allocate a PEBS buffer struct\n");
		goto no_mem;
	}

	for (i = 0; i < nBuffer_array; ++i) {
		buffer[i].samples = (struct pebs_sample *)
			kzalloc(sizeof(struct pebs_sample) * nRecords_pebs, GFP_KERNEL);

		if (!buffer[i].samples) {
			pr_err("Cannot allocate sample buffer at struct %d\n", i);
			goto no_samples;
		}

		buffer[i].allocated = 1;
	}

	buffer[0].usage = 1;

	ds->bts_buffer_base 		= 0;
	ds->bts_index			= 0;
	ds->bts_absolute_maximum	= 0;
	ds->bts_interrupt_threshold	= 0;
	ds->pebs_buffer_base		= buffer[0].samples;
	ds->pebs_index			= buffer[0].samples;
	ds->pebs_absolute_maximum	= buffer[0].samples + (nRecords_pebs - 1);
	ds->pebs_interrupt_threshold	= buffer[0].samples + (nRecords_pebs - 1);
	ds->reserved			= 0;
	
	/* equivalent to &buffer[0] */
	this_cpu_write(percpu_pebs_buffer, buffer);
	return;

no_samples:
	for (; i > 0; --i) {
		kfree(buffer[i - 1].samples);
	}
	kfree(buffer);
no_mem:
	*err = -ENOMEM;
}

void pebs_ds_update_on_cpu(void *arg)
{
	unsigned k;
	// u64 msr, 
	u64 mask = 0ULL;
	u64 *reset;
	struct pmc_cfg *cur_cfg;
	struct debug_store *ds = this_cpu_ptr(&percpu_ds);

	cur_cfg = this_cpu_read(pcpu_pmc_cfg);

	for (k = 0, reset = &ds->pebs_counter0_reset; k < pmc_max_available(); ++k, ++reset) {
		/* Set the bit only if the pmc requires PEBS */
		if (!cur_cfg[k].pebs) continue;
		mask |= BIT_ULL(k);
		/* Set PMC's reset values */
		*reset = cur_cfg[k].reset;
	}

	if (!mask) return;

	// TODO Do we really want to preserve the old PEBS config?
	// rdmsrl(MSR_IA32_PEBS_ENABLE, msr);
	// wrmsrl(MSR_IA32_PEBS_ENABLE, msr | mask);
	wrmsrl(MSR_IA32_PEBS_ENABLE, mask);
}

static void pebs_ds_init_on_cpu(void *arg)
{
	u64 msr;
	struct debug_store *ds = this_cpu_ptr(&percpu_ds);

	rdmsrl(MSR_IA32_DS_AREA, msr);
	/* Backup DS' value */
	this_cpu_write(percpu_ds_bkp, (struct debug_store *)msr);
	/* Update DS' value */
	wrmsrl(MSR_IA32_DS_AREA, (u64) ds);

	pebs_ds_update_on_cpu(NULL);
}

static void pebs_ds_fini_on_cpu(void *arg)
{
	struct debug_store *ds = this_cpu_read(percpu_ds_bkp);

	/* Restore BKP DS' value */
	wrmsrl(MSR_IA32_DS_AREA, (u64) ds);
}

static void pebs_resource_dealloc_on_cpu(void *arg)
{
	unsigned i;
	struct pebs_buffer *buffer = this_cpu_read(percpu_pebs_buffer);

	for (i = 0; i < nBuffer_array; ++i) {
		kfree(buffer[i].samples);
	}

	kfree(buffer);
	
	this_cpu_write(percpu_pebs_buffer, NULL);
}

int pebs_init(void)
{
	int err = 0;

	on_each_cpu(pebs_resource_alloc_on_cpu, &err, 1);
	if (err) goto err;
	on_each_cpu(pebs_ds_init_on_cpu, &err, 1);
	if (err) goto err;

	goto end;
err:
	pr_err("Something wrong happened during pebs init\n");
end:
	return err;
}

void pebs_fini(void)
{
	on_each_cpu(pebs_ds_fini_on_cpu, NULL, 1);

	on_each_cpu(pebs_resource_dealloc_on_cpu, NULL, 1);
}


void pebs_buffer_store_n_replace(void)
{
	unsigned i;
	struct pebs_buffer *buffer;
	struct debug_store *ds;

	buffer = this_cpu_read(percpu_pebs_buffer);

	for (i = 0; i < nBuffer_array; ++i) {
		if (buffer[i].usage) continue;

		if(!buffer[i].allocated) {
			buffer[i].samples = (struct pebs_sample *) 
				kzalloc(sizeof(struct pebs_sample) * nRecords_pebs, GFP_ATOMIC);
			if (!buffer[i].samples) {
				pr_err("Cannot allocate PEBS buffer\n");
				goto no_mem;
			}
			buffer[i].allocated = 1;
		}
		// TODO ???
		// schedule_task(index_array, (unsigned long) ds->pebs_buffer_base, (unsigned long) ds->pebs_index);
		break;
	}

	if (i == nBuffer_array) {
		pr_warn("PEBS used up all the available buffers\n");
		goto no_mem;
	}

	ds = this_cpu_ptr(&percpu_ds);

	buffer[i].usage = 1;
	//pr_info("[CPU%d]Switch to index: %d\n",smp_processor_id(), i);
	ds->pebs_buffer_base		= buffer[i].samples;
	ds->pebs_index			= buffer[i].samples;
	ds->pebs_absolute_maximum	= buffer[i].samples + (nRecords_pebs - 0x1);
	ds->pebs_interrupt_threshold	= buffer[i].samples + (nRecords_pebs - 0xF);
	//ds->pebs_index = ds->pebs_buffer_base;
	// ++collected_samples;

	// TODO sync ds

no_mem:
	return;
}

void pebs_print_status_on_syslog(void)
{
	u64 msr;
	struct debug_store *ds;

	rdmsrl(MSR_IA32_DS_AREA, msr);
	ds = (struct debug_store *)  msr;

	pr_info("ds->pebs_buffer_base: %llx\n", (u64) ds->pebs_buffer_base);
	pr_info("ds->pebs_index: %llx\n", (u64) ds->pebs_index);
	pr_info("ds->pebs_absolute_maximum: %llx\n", (u64) ds->pebs_absolute_maximum);
	pr_info("ds->pebs_interrupt_threshold: %llx\n", (u64) ds->pebs_interrupt_threshold);
	pr_info("ds->pebs_counter0_reset: %llx\n", (u64) ds->pebs_counter0_reset);
	pr_info("ds->pebs_counter1_reset: %llx\n", (u64) ds->pebs_counter1_reset);
	pr_info("ds->pebs_counter2_reset: %llx\n", (u64) ds->pebs_counter2_reset);
	pr_info("ds->pebs_counter3_reset: %llx\n", (u64) ds->pebs_counter3_reset);
}

// void read_buffer_samples(struct buffer_struct* args){
// 	int c;
// 	u64 l = 0;
// 	sample_arg_t *cursor;
// 	hash_for_each(hash_samples, c, cursor, sample_node){
// 		args[l].address = cursor->address;
// 		args[l].times = cursor->times;
// 		++l;
// 	}
// }

// u64 retrieve_buffer_size(void){
// 	int c;
// 	u64 n_samples = 0;
// 	sample_arg_t *cursor;
// 	hash_for_each(hash_samples, c, cursor, sample_node){
// 		++n_samples;
// 	}
// 	return n_samples;
// }

// void empty_pebs_buffer(void){
// 	int index_array;
// 	debug_store_t *ds;
// 	bh_data_t *data;
// 	ds = this_cpu_ptr(percpu_ds);
// 	index_array = __this_cpu_read(percpu_index);

// 	data = (bh_data_t*) kzalloc (sizeof(bh_data_t), GFP_ATOMIC);
// 	if (!data) {
// 		pr_err("Cannot allocate DATA buffer\n");
// 		return;
// 	}
// 	data->start = (unsigned long) ds->pebs_buffer_base;
// 	data->end = (unsigned long) ds->pebs_index;
// 	data->index = index_array;
// 	tasklet_handler(data);

// 	ds->pebs_index = ds->pebs_buffer_base;
// }