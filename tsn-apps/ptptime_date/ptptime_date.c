/* SPDX-License-Identifier: GPL-2.0 */
/*
 * TSN Configuration utility
 *
 * (C) Copyright 2017 - 2021, Xilinx, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
*/
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
