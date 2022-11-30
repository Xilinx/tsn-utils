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
#include <linux/ethtool.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <linux/in.h>
#include <string.h>

static clockid_t get_clockid(int fd)
{
#define CLOCKFD 3
#define FD_TO_CLOCKID(fd)	((~(clockid_t) (fd) << 3) | CLOCKFD)

	return FD_TO_CLOCKID(fd);
}

int phc_fd;

int ptp_open(char *ifname)
{
	struct ifreq s;
	struct ethtool_ts_info info;
	struct ifreq ethtool_ifr;
	int ret;
	char phc[32];

	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	memset(&ethtool_ifr, 0, sizeof(ethtool_ifr));
	memset(&info, 0, sizeof(info));
	info.cmd = ETHTOOL_GET_TS_INFO;
	printf("interface name: %s\n",ifname);
	strncpy(ethtool_ifr.ifr_name, ifname, IFNAMSIZ - 1);
	ethtool_ifr.ifr_data = (char *) &info;
	ret = ioctl(fd, SIOCETHTOOL, &ethtool_ifr);

	if (ret < 0) {
		printf("ioctl SIOCETHTOOL failed : %m\n");
		close(fd);
		return -1;
	}

	if (info.phc_index >= 0) {
		printf("selected /dev/ptp%d as PTP clock\n", info.phc_index);
		snprintf(phc, sizeof(phc), "/dev/ptp%d", info.phc_index);
	}
	else {
		printf("phc index: %d\n",info.phc_index);
	}

	phc_fd = open(phc, O_RDWR);

	if (phc_fd < 0)
	{
		printf("phc open failed\n");
		close(fd);
		return -1;
	}
	return 0;
}

void usage(){
	printf("ptptime <interface name>\n");
	return 0;
}

int main(int argc, char **argv)
{
	struct timespec tmx;
	clockid_t clkid;
	char ifname[IFNAMSIZ];
	struct ifreq ethtool_ifr;

	if(argc != 2){
		usage();
		return 0;
	}

	strncpy(ifname, argv[1], IFNAMSIZ);

	if(ptp_open(ifname))
		return 0;

	clkid = get_clockid(phc_fd);

	while (1)
	{
		clock_gettime(clkid, &tmx);
	
		printf("ptp time: sec: %lx ns: %lx\n ", tmx.tv_sec, tmx.tv_nsec);

		usleep(100000);
	}
}
