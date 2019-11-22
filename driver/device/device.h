#define MODULE_MINORS		1
#define MODULE_DEV_MINOR 	0
#define MODULE_DEVICE_NAME 	"generic"

// struct minor {
// 	int min;
// 	struct list_head node;
// };

extern int device_init(struct file_operations *fops);

extern void device_fini(void);

// int setup_resources(void);

// void cleanup_resources(void);