#include <stdlib.h>	 /* exit func */
#include <stdio.h>	  /* printf func */
#include <fcntl.h>	  /* open syscall */
#include <getopt.h>	 /* args utility */
#include <sys/ioctl.h>  /* ioctl syscall*/
#include <unistd.h>	/* close syscall */
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/wait.h>

#include "ime-ioctl.h"
#include "intel_pmc_events.h"

const char * device = "/dev/generic";
int exit_ = 0;

#define ARGS "x:j:p:e:s:c:b:u:k:r:nofdta:w:"
#define MAX_LEN 256

configuration_t* current_config;
char binary[16][5] = {"0000", "0001", "0010", "0011", "0100",
 "0101", "0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111"};

char* convert_binary(char c){
	char* res;
	if(c >= '0' && c <= '9') return binary[atoi(&c)];
	if(c >= 'A' && c <= 'F') return binary[c-'A'+10];
	return NULL;
}

int convert_decimal(char c){
	int res;
	if(c >= '0' && c <= '9') return atoi(&c);
	if(c >= 'A' && c <= 'F') return (c-'A'+10);
	return -1;
}


int int_from_stdin()
{
	char buffer[12];
	int i = -1;
	if (fgets(buffer, sizeof(buffer), stdin)) {
		sscanf(buffer, "%d", &i);
	}
	return i;
}// int_from_stdin

int read_buffer(int fd){
	int err = 0;
	unsigned long size_buffer, i;
	struct buffer_struct* args;
	if ((err = ioctl(fd, IME_SIZE_BUFFER, &size_buffer)) < 0){
		printf("IOCTL: IME_SIZE_BUFFER failed\n");
		return err;
	}
	printf("IOCTL: IME_SIZE_BUFFER success ~~ size buffer: %lu\n", size_buffer);
	if(!size_buffer) return 0;
	args = (struct buffer_struct*) malloc (sizeof(struct buffer_struct)*size_buffer);
	if ((err = ioctl(fd, IME_READ_BUFFER, args)) < 0){
		printf("IOCTL: IME_READ_BUFFER failed\n");
		return err;
	}
	printf("IOCTL: IME_READ_BUFFER success\n");

	for(i = 0; i < size_buffer; i++){
		printf("0x%lx\t%d\n", args[i].address << 12, args[i].times);
	}

	free(args);
	return err;
}

int main(int argc, char **argv)
{
	// if (argc < 2) goto end;

	int fd, fd_j, option, err, val, joe_on, offcore = 1;
	long long pid;
	int len, i, k;
	uint64_t sval;
	char delim[] = ",";
	char *ptr;
	char *args[MAX_LEN];
	char* name;
	unsigned long size_stat = 0;
	struct statistics_user *stat;

	fd = open(device, 0666);
	if (fd < 0) {
		printf("Error, cannot open %s\n", device);
		return -1;
	}

	if ((err = ioctl(fd, JOE_PROFILER_ON)) < 0){
		printf("IOCTL: JOE_PROFILER_ON failed\n");
		return err;
	}

	printf("IOCTL: JOE_PROFILER_ON success\n");
	// break;

	// current_config = (configuration_t*) calloc (numCPU, sizeof(configuration_t));
	// err = 0;
	// while(!err && option != -1) {
	// 	switch(option) {

	// 	case 'j':
	// 		if(strncmp(optarg, "n", MAX_LEN) == 0){
	// 			if ((err = ioctl(fd, JOE_PROFILER_ON)) < 0){
	// 				printf("IOCTL: JOE_PROFILER_ON failed\n");
	// 				return err;
	// 			}
	// 			printf("IOCTL: JOE_PROFILER_ON success\n");
	// 			break;
	// 		}

	// 		if(strncmp(optarg, "f", MAX_LEN) == 0){
	// 			if ((err = ioctl(fd, JOE_PROFILER_OFF)) < 0){
	// 				printf("IOCTL: JOE_PROFILER_OFF failed\n");
	// 				return err;
	// 			}
	// 			printf("IOCTL: JOE_PROFILER_OFF success\n");
	// 			break;
	// 		}

	// 		sscanf(optarg, "%lld", &pid);
	// 		if(pid >= 0){
	// 			if ((err = ioctl(fd, JOE_ADD_TID, pid)) < 0){
	// 				printf("IOCTL: JOE_ADD_TID failed\n");
	// 				return err;
	// 			}
	// 			printf("IOCTL: JOE_ADD_TID success\n");
	// 		}
	// 		else{
	// 			pid = pid*(-1);
	// 			if ((err = ioctl(fd, JOE_DEL_TID, pid)) < 0){
	// 				printf("IOCTL: JOE_DEL_TID failed\n");
	// 				return err;
	// 			}
	// 			printf("IOCTL: JOE_DEL_TID success\n");
	// 		}
	// 		break;

	// 	case 'x':
	// 		ptr = strtok(optarg, delim);
	// 	i = 1;
	// 	name = ptr;
	// 	args[0] = name;
	// 	ptr = strtok(NULL, delim);

	// 	while (ptr != NULL){
	// 		printf("PTR: %s\n", ptr);
	// 		args[i] = ptr;
	// 		ptr = strtok(NULL, delim);
	// 		i++;
	// 	}

	// 	args[i] = NULL;
	// 	int pid = fork();

	// 	if (pid == 0) {
	// 		printf("This is the child process. My pid is %d and my parent's id is %d.\n", getpid(), getppid());
	// 		if ((err = ioctl(fd, JOE_ADD_TID, getpid())) < 0){
	// 			printf("IOCTL: JOE_ADD_TID failed\n");
	// 			return err;
	// 		}
	// 		printf("IOCTL: JOE_ADD_TID success\n");
	// 		if ((err = ioctl(fd, JOE_PROFILER_ON)) < 0){
	// 			printf("IOCTL: JOE_PROFILER_ON failed\n");
	// 			return err;
	// 		}
	// 		printf("IOCTL: JOE_PROFILER_ON success\n");
	// 		execvp(name,args); 
	// 		return 0;
	// 	}
	// 	else {
	// 		printf("This is the parent process. My pid is %d and my child's id is %d.\n", getpid(), pid);
	// 		waitpid(pid, NULL, 0);
	// 		if ((err = ioctl(fd, IME_WAIT)) < 0){
	// 			printf("IOCTL: IME_WAIT failed\n");
	// 		}
	// 		printf("Child is died\n");
	// 	}
	// 	break;

	// 	case 'p':
	// 		len = strnlen(optarg, MAX_LEN);
	// 		for(i = 0; i < numCPU && i < len; i++){
	// 			if(optarg[i] == '0'){
	// 				current_config[i].valid_cpu = 0;
	// 				continue;
	// 			}
	// 			current_config[i].valid_cpu = 1;
	// 			char* c = convert_binary(toupper(optarg[i]));
	// 			for(k = 0; k < MAX_ID_PMC; k++){
	// 				current_config[i].pmcs[k].valid = (c[MAX_ID_PMC-1-k] == '1')? 1 : 0;
	// 			}
	// 		}
	// 		break;

	// 	case 'b':
	// 		len = strnlen(optarg, MAX_LEN);
	// 		for(i = 0; i < numCPU && i < len; i++){
	// 			if(current_config[i].valid_cpu == 0) continue;
	// 			char* c = convert_binary(toupper(optarg[i]));
	// 			for(k = 0; k < MAX_ID_PMC; k++){
	// 				current_config[i].pmcs[k].enable_PEBS = (c[MAX_ID_PMC-1-k] == '1')? 1 : 0;
	// 			}
	// 		}
	// 		break;

	// 	case 'c':
	// 		len = strnlen(optarg, MAX_LEN);
	// 		for(i = 0; i < numCPU && i < len; i++){
	// 			if(current_config[i].valid_cpu == 0) continue;
	// 			char* c = convert_binary(toupper(optarg[i]));
	// 			for(k = 0; k < MAX_ID_PMC; k++){
	// 				if(c[MAX_ID_PMC-1-k] == '1' && offcore <= 2) {
	// 					current_config[i].pmcs[k].offcore = offcore;
	// 					++offcore;
	// 				}
	// 				else current_config[i].pmcs[k].offcore = 0;
	// 			}
	// 			offcore = 1;
	// 		}
	// 		//if the user does not specify the value for some CPUs, the default value is set
	// 		for(; i < numCPU; i++){
	// 			for(k = 0; k < MAX_ID_PMC; k++){
	// 				current_config[i].pmcs[k].offcore = 0;
	// 			}
	// 		}
	// 		break;

	// 	case 'e':
	// 		ptr = strtok(optarg, delim);
	// 		i = 0;
	// 		while(ptr != NULL && i < MAX_ID_PMC){
	// 			sscanf(ptr, "%lx", &sval);
	// 			for(k = 0; k < numCPU; k++){
	// 				if(current_config[k].pmcs[i].valid) current_config[k].pmcs[i].event = sval;
	// 			}
	// 			ptr = strtok(NULL, delim);
	// 			++i;
	// 		}
	// 		break;

	// 	case 's':
	// 		ptr = strtok(optarg, delim);
	// 		i = 0;
	// 		while(ptr != NULL && i < numCPU){
	// 			sscanf(ptr, "%lx", &sval);
	// 			for(k = 0; k < MAX_ID_PMC; k++){
	// 				current_config[i].pmcs[k].start_value = ~sval;
	// 			}
	// 			ptr = strtok(NULL, delim);
	// 			++i;
	// 		}
	// 		break;

	// 	case 'r':
	// 		ptr = strtok(optarg, delim);
	// 		i = 0;
	// 		while(ptr != NULL && i < numCPU){
	// 			sscanf(ptr, "%lx", &sval);
	// 			for(k = 0; k < MAX_ID_PMC; k++){
	// 				current_config[i].pmcs[k].reset_value = ~sval;
	// 			}
	// 			ptr = strtok(NULL, delim);
	// 			++i;
	// 		}
	// 		break;

	// 	case 'w':
	// 		sscanf(optarg, "%lx", &sval);
	// 		i = 0;
	// 		while(i < numCPU){
	// 			current_config[i].fixed_value = ~sval;
	// 			++i;
	// 		}
	// 		break;


	// 	case 'u':
	// 		len = strnlen(optarg, MAX_LEN);
	// 		for(i = 0; i < numCPU && i < len; i++){
	// 			char* c = convert_binary(toupper(optarg[i]));
	// 			for(k = 0; k < MAX_ID_PMC; k++){
	// 				current_config[i].pmcs[k].user = (c[MAX_ID_PMC-1-k] == '1')? 1 : 0;
	// 			}
	// 		}
	// 		break;

	// 	case 'k':
	// 		len = strnlen(optarg, MAX_LEN);
	// 		for(i = 0; i < numCPU && i < len; i++){
	// 			char* c = convert_binary(toupper(optarg[i]));
	// 			for(k = 0; k < MAX_ID_PMC; k++){
	// 				current_config[i].pmcs[k].kernel = (c[MAX_ID_PMC-1-k] == '1')? 1 : 0;
	// 			}
	// 		}
	// 		break;

	// 	case 'n':
	// 		if ((err = ioctl(fd, IME_SETUP_PMC, current_config)) < 0){
	// 			printf("IOCTL: IME_SETUP_PMC failed\n");
	// 			return err;
	// 		}
	// 		printf("IOCTL: IME_SETUP_PMC success\n");
	// 		break;

	// 	case 'f':
	// 		if ((err = ioctl(fd, IME_RESET_PMC, current_config)) < 0){
	// 			printf("IOCTL: IME_RESET_PMC failed\n");
	// 			return err;
	// 		}
	// 		printf("IOCTL: IME_RESET_PMC success\n");
	// 		break;

	// 	case 'd':				
	// 		if ((err = ioctl(fd, IME_READ_STATUS, current_config)) < 0){
	// 			printf("IOCTL: IME_READ_STATUS failed\n");
	// 			return err;
	// 		}
	// 		printf("IOCTL: IME_READ_STATUS success\n");
	// 		break;

	// 	case 't':	//read tuples	
	// 		if ((err = ioctl(fd, IME_SIZE_STATS, &size_stat)) < 0){
	// 			printf("IOCTL: IME_SIZE_STATS failed\n");
	// 			return err;
	// 		}
	// 		printf("IOCTL: IME_SIZE_STATS success ~~ size: %lu\n", size_stat);
	// 		if(size_stat == 0) break;
	// 		stat = (struct statistics_user*) calloc (size_stat, sizeof(struct statistics_user));
	// 		if ((err = ioctl(fd, IME_EVT_STATS, stat)) < 0){
	// 			printf("IOCTL: IME_EVT_STATS failed\n");
	// 			return err;
	// 		}
	// 		printf("IOCTL: IME_EVT_STATS success\n");

	// 		for(k = 0; k < size_stat; k++){
	// 			printf("FIXED0:\t%lx\n", stat[k].fixed0);
	// 			printf("FIXED1:\t%lx\n", stat[k].fixed1);
	// 			for(i = 0; i < MAX_ID_PMC; i++){
	// 				printf("PMC%d:\t%lx\n", i, stat[k].event[i].stat);
	// 			}
	// 		}
	// 		free(stat);
	// 		break;

	// 	case 'a':	//read tuples	
	// 		if ((err = ioctl(fd, IME_SIZE_STATS, &size_stat)) < 0){
	// 			printf("IOCTL: IME_SIZE_STATS failed\n");
	// 			return err;
	// 		}
	// 		printf("IOCTL: IME_SIZE_STATS success ~~ size: %lu\n", size_stat);
	// 		if(size_stat == 0) break;
	// 		stat = (struct statistics_user*) calloc (size_stat, sizeof(struct statistics_user));
	// 		if ((err = ioctl(fd, IME_EVT_STATS, stat)) < 0){
	// 			printf("IOCTL: IME_EVT_STATS failed\n");
	// 			return err;
	// 		}
	// 		printf("IOCTL: IME_EVT_STATS success\n");

	// 		FILE *f = fopen(optarg, "w");
	// 		if (f == NULL)
	// 		{
	// 			printf("Error opening file!\n");
	// 			exit(1);
	// 		}
	// 		fprintf(f, "#time\t#INSTR_RETIRED");
	// 		uint64_t time = 0;
	// 		for(i = 0; i < MAX_ID_PMC; i++){
	// 			if(!stat[0].event[i].event) continue;
	// 			fprintf(f, "\t0x%lx",stat[0].event[i].event);
	// 		}
	// 		fprintf(f, "\n");
	// 		for(k = 0; k < size_stat; k++){
	// 			time +=  stat[k].fixed1;
	// 			fprintf(f, "%lu\t%lu", time, stat[k].fixed0);
	// 			for(i = 0; i < MAX_ID_PMC; i++){
	// 				if(!stat[0].event[i].event) continue;
	// 				fprintf(f, "\t%lu",stat[k].event[i].stat);
	// 			}
	// 			fprintf(f, "\n");
	// 		}
	// 		fclose(f);
	// 		free(stat);
	// 		break;

	// 	case 'o':
	// 		read_buffer(fd);
	// 		break;

	// 	default:
	// 		break;
	// 	}
	// 	option = getopt(argc, argv, ARGS);
	// }

	close(fd);
	free(current_config);
end:
	return 0;
}// main