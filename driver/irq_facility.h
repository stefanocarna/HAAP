#include <linux/sched.h>

#define NMI_NAME	"ime"
#define MAX_ID_PMC 4
#define SWITCH_FUNC "finish_task_switch"

#define PTR_BITS		48
#define PTR_THR_BIT		BIT_ULL(PTR_BITS - 1)				/* threshold bit */
#define PTR_USED_MASK 		(BIT_ULL(PTR_BITS) - 1)				// 0x0000F...F
#define PTR_FREE_MASK		(~PTR_USED_MASK)				// 0xFFFF0...0
#define PTR_CRC_MASK		(((1ULL << (64 - PTR_BITS - 2)) - 1) << 48)	// 0x3FFF0...0
#define PTR_CCB_MASK		((1ULL << (64 - PTR_BITS - 2)) - 1)		// 0x0...03FFF
#define PROCESS_BIT		63
#define ENABLED_BIT		62
#define PROCESS_MASK		BIT_ULL(PROCESS_BIT)
#define ENABLED_MASK		BIT_ULL(ENABLED_BIT)

#define CRC_MAGIC		0xc
#define get_crc(stk)		((((unsigned long)stk & PTR_CCB_MASK) ^ CRC_MAGIC) << 48)
#define check_crc(stk)		(((stk & PTR_CCB_MASK) ^ CRC_MAGIC) == ((stk & PTR_CRC_MASK) >> 48))
#define build_ptr(ptr) 		((ptr & PTR_THR_BIT) ? PTR_FREE_MASK | ptr : PTR_USED_MASK & ptr)
#define get_ptr(ptr)		(((unsigned long) ptr) & PTR_USED_MASK)

// #define magic_entry		(end_of_stack(current) + 1)
#define is_current_enabled	(current->flags & BIT(24))
#define is_enabled(task)	(((struct task_struct*)task)->flags & BIT(24))
// #define is_current_valid	check_crc(*magic_entry)
// #define current_ptr		build_ptr(*magic_entry)

int enable_nmi(void);
void disable_nmi(void);

void cleanup_pmc(void);
void print_reg(void);

int switch_patcher_init (unsigned long func_addr);
void switch_patcher_exit(void);

extern void pmi_entry(void);
int pmc_init(void);
void pmc_fini(void);
extern int sample;