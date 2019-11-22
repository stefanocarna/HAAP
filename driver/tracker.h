int register_pid(pid_t pid);
// int unregister_pid(pid_t pid);

int is_pid_present(pid_t pid);

int tracker_init(void);
void tracker_fini(void);

void wait_track(void);

struct tracked_process {
	pid_t pid;
	struct list_head list;
};