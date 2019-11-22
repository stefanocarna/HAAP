#include <stdlib.h>     /* exit func */
#include <stdio.h>      /* printf func */
#include <fcntl.h>      /* open syscall */
#include <getopt.h>     /* args utility */
#include <sys/ioctl.h>  /* ioctl syscall*/
#include <unistd.h>	/* close syscall */
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/wait.h>

#include "ime-ioctl.h"
#include "ime.h"

const char * device = "/dev/ime/pmc";
static int fd = 0;

int open_file(){
	fd = open(device, 0666);
	if (fd < 0) {
		printf("Error, cannot open %s\n", device);
		return -1;
	}
	int err = 0;
	if ((err = ioctl(fd, JOE_ADD_TID, getpid())) < 0){
        printf("IOCTL: JOE_ADD_TID failed\n");
        return err;
    }
	if ((err = ioctl(fd, JOE_PROFILER_ON)) < 0){
        printf("IOCTL: JOE_PROFILER_ON failed\n");
        return err;
    }
	return fd;
}

void close_file(){
	int err = 0;
	if ((err = ioctl(fd, JOE_PROFILER_OFF)) < 0){
        printf("IOCTL: JOE_PROFILER_OFF failed\n");
        return;
    }
	close(fd);
}

int ime_on(){
	int err = 0;
	if ((err = ioctl(fd, IME_PROFILER_ON)) < 0){
        printf("IOCTL: IME_PROFILER_ON failed\n");
        return err;
    }
    return err;
}

int ime_off(){
	int err = 0;
	if ((err = ioctl(fd, IME_PROFILER_OFF)) < 0){
        printf("IOCTL: IME_PROFILER_OFF failed\n");
        return err;
    }
    return err;
}