
#define LVT_NMI		(0x4 << 8) 
#define NMI_LINE	2
#define RESERVED_LINES	32
#define NMI_NAME	"CAHAP"

extern int pmi_init(u8 type);

extern void pmi_fini(void);