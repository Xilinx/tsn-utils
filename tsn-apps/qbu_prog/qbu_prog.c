/***************************************************************
* Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
* SPDX-License-Identifier: MIT
***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/in.h>
#include <syscall.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <unistd.h>

typedef  unsigned int uint32_t;
typedef  unsigned short uint16_t;
typedef  unsigned char uint8_t;

enum axienet_tsn_ioctl {
        SIOCCHIOCTL = SIOCDEVPRIVATE,
        SIOC_GET_SCHED,
        SIOC_PREEMPTION_CFG,
        SIOC_PREEMPTION_CTRL,
        SIOC_PREEMPTION_STS,
        SIOC_PREEMPTION_COUNTER,
        SIOC_QBU_USER_OVERRIDE,
        SIOC_QBU_STS,
};

struct qbu_prog_override {
        uint8_t enable_value:1;
        uint16_t user_hold_time:9;
        uint8_t user_rel_time:6;
        uint8_t guard_band:1;
        uint8_t hold_rel_window:1;
        uint8_t hold_time_override:1;
        uint8_t rel_time_override:1;
}__attribute__((packed));

struct qbu_prog {
        struct qbu_prog_override user;
        uint8_t set;
};

#define BIT(x) (1 << (x))
#define QBU_WINDOW BIT(0)
#define QBU_GUARD_BAND BIT(1)
#define QBU_HOLD_TIME BIT(2)
#define QBU_REL_TIME BIT(3)

struct qbu_core_status {
        uint16_t hold_time;
        uint8_t rel_time;
        uint8_t hold_rel_en:1;
        uint8_t pmac_hold_req:1;
}__attribute__((packed));

struct qbu_all_status {
	struct qbu_prog_override prog;
	struct qbu_core_status core;
};

void qbu_status(struct qbu_all_status *sts)
{
	printf("Hold-Release Window: %d\n", sts->prog.hold_rel_window);
	printf("Guard Band Overrun Counters increment: %d\n", sts->prog.guard_band);
	printf("User Hold Time in Bytes: %d\n", sts->prog.user_hold_time);
	printf("User Release Time in Bytes: %d\n", sts->prog.user_rel_time);
	printf("Hold-Release Enablement: %d\n", sts->core.hold_rel_en);
	printf("Hold Time(Status) in Bytes: %d\n", sts->core.hold_time);
	printf("Release Time(Status) in Bytes: %d\n", sts->core.rel_time);
	printf("PMAC is under Hold Operation: %d\n", sts->core.pmac_hold_req);
	return;
}

void usage()
{
	printf("Usage:\n qbu_prog <interface> <status> -w <0|1> -g <0|1> -h <hold_time> -r <rel_time>\n");
	printf("\tOR\n");
	printf("Usage:\n qbu_prog <interface> <status>\n");
	printf("interface : [eth1|eth2]\n");
	printf("status: Display the Qbu Core status\n");
	printf("Optional parameters:\n");
	printf("\t-w : Hold-Release Window ON/OFF\n");
	printf("\t-g : Guard Band Overrun Counters increment ON/OFF\n");
	printf("\t-h : User Hold time in bytes 0 to 511, 0- Disable the User Hold time Override\n");
	printf("\t-r : User Release time in bytes 0 t0 63, 0- Disable the User Release time Override\n");
}

int main(int argc, char **argv)
{
	char ifname[IFNAMSIZ];
	struct qbu_prog qbu;
	struct qbu_all_status sts;
	int fd;
	int ch;
	struct ifreq s;

	opterr = 0;
	if (argc < 3 || argc > 10) {
		usage();
		return -1;
	}

	strncpy(ifname, argv[1], IFNAMSIZ);
	if ((strcmp(argv[1], "eth1")) && (strcmp(argv[1], "eth2"))) {
		usage();
		return -1;
	}
	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	strcpy(s.ifr_name, ifname);
	
	printf("Interface: %s\n", ifname);
	if (!strcmp(argv[2], "status")) {
		s.ifr_data = (void *)&sts;
		if (ioctl(fd, SIOC_QBU_STS, &s) < 0) {
			printf("Get Qbu Core status failed\n");
			goto out;
		}
		qbu_status(&sts);
		goto out;
	}

	memset(&qbu, 0, sizeof(struct qbu_prog));
	while ((ch = getopt (argc, argv, "w:g:h:r:")) != -1)
		switch (ch)
		{
			case 'w':
				qbu.set |= QBU_WINDOW;
				qbu.user.hold_rel_window = atoi(optarg)? 1: 0;
				break;
			case 'g':
				qbu.set |= QBU_GUARD_BAND;
				qbu.user.guard_band = atoi(optarg)? 1: 0;
				break;
			case 'h':
				qbu.set |= QBU_HOLD_TIME;
				qbu.user.hold_time_override = atoi(optarg)? 1: 0;
				qbu.user.user_hold_time = atoi(optarg);
				break;
			case 'r':
				qbu.set |= QBU_REL_TIME;
				qbu.user.rel_time_override = atoi(optarg)? 1: 0;
				qbu.user.user_rel_time = atoi(optarg);
				break;
			case '?':
				if (optopt == 'w' || optopt == 'g' || optopt == 'h' || optopt == 'r')
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr,
							"Unknown option character `\\x%x'.\n",
							optopt);
				return 1;
			default:
				usage ();
				goto out;
		}
	printf("Set: 0x%x window: %d, GB: %d, Hold_time: %d-%d Rel_time: %d-%d\n", qbu.set,
			qbu.user.hold_rel_window, qbu.user.guard_band, qbu.user.hold_time_override,
			qbu.user.user_hold_time, qbu.user.rel_time_override, qbu.user.user_rel_time);


	s.ifr_data = (void *)&qbu;
	if (ioctl(fd, SIOC_QBU_USER_OVERRIDE, &s) < 0) {
		printf("Set Qbu User Override failed\n");
	}
out:
	close(fd);
	return 0;
}	
