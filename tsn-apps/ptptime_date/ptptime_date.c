/***************************************************************
* Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
* Copyright (C) 2022 - 2023 Advanced Micro Devices, Inc. All rights reserved.
* SPDX-License-Identifier: MIT
***************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <syscall.h>
#include <sys/ioctl.h>
#include <time.h>

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
	time_t current_time;
	char *c_time_string;

	ptp_open();

	clkid = get_clockid(phc_fd);

	clock_gettime(clkid, &tmx);

	current_time = tmx.tv_sec;
	
	c_time_string = ctime(&current_time);

	printf("current time is : %s\n", c_time_string);

}
