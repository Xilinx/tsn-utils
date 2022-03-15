/***************************************************************
* Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
* SPDX-License-Identifier: MIT
***************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <syscall.h>
#include <sys/ioctl.h>

static clockid_t get_clockid(int fd)
{
#define CLOCKFD 3
#define FD_TO_CLOCKID(fd)	((~(clockid_t) (fd) << 3) | CLOCKFD)

	return FD_TO_CLOCKID(fd);
}

int phc_fd;

void ptp_open()
{
	phc_fd = open( "/dev/ptp0", O_RDWR );
	if(phc_fd < 0)
		printf("ptp open failed\n");
}

int main()
{
	struct timespec tmx;
	clockid_t clkid;

	ptp_open();

	clkid = get_clockid(phc_fd);

	while (1)
	{
		clock_gettime(clkid, &tmx);
	
		printf("ptp time: sec: %lx ns: %lx\n ", tmx.tv_sec, tmx.tv_nsec);

		usleep(100000);
	}
}
