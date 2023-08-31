/***************************************************************
* Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
* Copyright (C) 2022 - 2023 Advanced Micro Devices, Inc. All rights reserved.
* SPDX-License-Identifier: MIT
***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/in.h>
#include <syscall.h>
#include <string.h>
#include <fcntl.h>
#include <linux/ethtool.h>
#include <sys/ioctl.h>
#include <libconfig.h>
#include <errno.h>

typedef  unsigned long long uint64_t;
typedef  unsigned int uint32_t;
typedef  unsigned char uint8_t;

#define ONE_MS 1000000
#define ONE_US 1000

#define MAX_CYCLE_TIME	1000000000
#define MAX_ENTRIES	256

#define SIOCETHTOOL     0x8946

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

enum hw_port
{
	PORT_EP = 0,
	PORT_TEMAC_1,
	PORT_TEMAC_2,
};

char *port_names[] = { "ep", "temac1", "temac2"};

struct qbv_info 
{
    uint8_t  port;
    uint8_t  force;
    uint32_t cycle_time;
    uint64_t ptp_time_sec;
    uint32_t ptp_time_ns;
    uint32_t list_length;
    uint32_t acl_gate_state[MAX_ENTRIES];
    uint32_t acl_gate_time[MAX_ENTRIES];
};

static clockid_t get_clockid(int fd)
{
#define CLOCKFD 3
#define FD_TO_CLOCKID(fd)	((~(clockid_t) (fd) << 3) | CLOCKFD)

	return FD_TO_CLOCKID(fd);
}

int set_schedule(struct qbv_info *prog, char *ifname)
{
	struct ifreq s;
	struct ethtool_ts_info info;
	struct ifreq ethtool_ifr;
	int ret;
	int phc_fd;
	char phc[32];
	clockid_t clkid;
	struct timespec tmx;

	if(!ifname)
		return -1;

	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if(prog->ptp_time_ns == 0 && prog->ptp_time_sec == 0){
		memset(&ethtool_ifr, 0, sizeof(ethtool_ifr));
		memset(&info, 0, sizeof(info));
		info.cmd = ETHTOOL_GET_TS_INFO;
		strncpy(ethtool_ifr.ifr_name, ifname, IFNAMSIZ - 1);
		ethtool_ifr.ifr_data = (char *) &info;
		ret = ioctl(fd, SIOCETHTOOL, &ethtool_ifr);
		if (ret < 0) {
			printf("ioctl SIOCETHTOOL failed: %m");
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
			return -1;

		clkid = get_clockid(phc_fd);
		clock_gettime(clkid, &tmx);
                prog->ptp_time_ns = 0;
                prog->ptp_time_sec = tmx.tv_sec + 1;
		close(phc_fd);
	}

	s.ifr_data = (void *)prog;

	strncpy(s.ifr_name, ifname,sizeof(s.ifr_name)-1);
	ret = ioctl(fd, SIOCDEVPRIVATE, &s);

	if(ret < 0)
	{
		printf("QBV prog failed\n");
		if(errno == EALREADY){
			printf("\nLast QBV schedule configuration is pending,"
			       " cannot configure new schedule.\n"
			       "use -f option to force the schedule\n");
		}

	}
	close(fd);
}

void get_schedule(char *ifname)
{
	struct qbv_info qbv;
	struct ifreq s;
	int ret, fd, i;

	if(!ifname)
		return;

	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

	s.ifr_data = (void *)&qbv;

	strcpy(s.ifr_name, ifname);

	ret = ioctl(fd, SIOC_GET_SCHED, &s);

	if(ret < 0)
	{
		printf("QBV get status failed\n");
		close(fd);
		return;
	}

	if (!qbv.cycle_time) {
		printf("Cycle time: %u\n", qbv.cycle_time);
		printf("QBV is not scheduled\n");
		close(fd);
		return;
	}

	printf("List length: %u\n", qbv.list_length);
	printf("Cycle time: %u\n", qbv.cycle_time);
	printf("Base time: %llus %uns\n", qbv.ptp_time_sec, qbv.ptp_time_ns);
	for (i = 0; i < qbv.list_length; i++) {
		printf("List %d: Gate State: %u Gate Time: %uns\n", i,
		       qbv.acl_gate_state[i], qbv.acl_gate_time[i] * 8);
	}
	close(fd);
	return;
}

void usage()
{
	printf("Usage:\n qbv_sched [-s|-g|-c] <interface> [off] [file path] -f\n");
	printf("interface : tsn interface name\n");
	printf("file path : location of Qbv schedule."
		"For example: /etc/xilinx-tsn/qbv.cfg\n");
	printf("-s : To set QBV schedule\n");
	printf("-g : To get operative QBV schedule\n");
	printf("-c : To use QBV schedule in the file at mentioned path\n");
	printf("-f : force to set QBV schedule\n");
	printf("off : to stop QBV schedule\n");
	exit(1);
}

int main(int argc, char **argv)
{
	struct qbv_info prog;
	int i, j;
	char *port;
	char ifname[IFNAMSIZ];

	config_t cfg;
	config_setting_t *setting;
	char str[120];
	int ch, len = 0;
	char set_qbv = 0;
	char file_path = 0, off = 0;
	char *path = NULL;

	if((argc < 2) || (argc > 5)) 
		usage();
	if(argc >= 4) {
		len = strlen(argv[3]);
		path = (char*)malloc((len+1)*sizeof(char));
		strcpy(path,argv[3]);
	}
	prog.force = 0;
	
	strncpy(ifname, argv[1], IFNAMSIZ);
	set_qbv=1;
	if(argc == 3 && (strcmp(argv[2],"off") == 0)) {
		off = 1;
		prog.cycle_time = 0;
	}

	while ((ch = getopt (argc, argv, "s:g:c:f")) != -1) {
                switch (ch)
                {
                        case 'g':
				strncpy(ifname, optarg, IFNAMSIZ);
				set_qbv = 0;
				get_schedule(ifname);
				goto out;
                        case 's':
				strncpy(ifname, optarg, IFNAMSIZ);
				set_qbv = 1;
				break;
			case 'c':
				strncpy(ifname, optarg, IFNAMSIZ);
				set_qbv = 1;
				file_path = 1;
				break;
                        case 'f':
				prog.force = 1;
				break;
			default:
				goto set;
		}
	}
set:
	config_init(&cfg);
	if(file_path) {
		if(len == 0)
			goto out;
	} else {
		if(path)
			free(path);
		len = strlen("/etc/xilinx-tsn/qbv.cfg");
		path = (char*)malloc((len+1)*sizeof(char));
		strcpy(path,"/etc/xilinx-tsn/qbv.cfg");
	}
	if( !config_read_file(&cfg, path) )
	{
		config_destroy(&cfg);
		printf("Can't read %s\n", path);
		return(EXIT_FAILURE);
	}

	if (set_qbv) {
		if(off == 0) {
			sprintf(str, "qbv.%s.cycle_time", ifname);
			setting = config_lookup(&cfg, str);
			if(!setting){
				printf("qbv.%s.cycle_time not found, please check your configuration\n", ifname);
				goto out;
			}
			prog.cycle_time = config_setting_get_int(setting);
		}
		printf("Setting: %s :\n", ifname);

		printf("cycle_time: %d\n", prog.cycle_time);

		if(prog.cycle_time == 0)
			printf("Opening all gates\n");

		if(prog.cycle_time > MAX_CYCLE_TIME)
		{
			printf("Cycle time invalid, assuming default\n");
			prog.cycle_time = MAX_CYCLE_TIME;
		}

		prog.cycle_time = prog.cycle_time;

		sprintf(str, "qbv.%s.start_sec", ifname);
		setting = config_lookup(&cfg, str);
		prog.ptp_time_sec = config_setting_get_int(setting);

		sprintf(str, "qbv.%s.start_ns", ifname);
		setting = config_lookup(&cfg, str);
		prog.ptp_time_ns = config_setting_get_int(setting);

		sprintf(str, "qbv.%s.gate_list", ifname);

		setting = config_lookup(&cfg, str);
		if(setting != NULL)
		{
			int count = config_setting_length(setting);

			if(count > MAX_ENTRIES)
			{
				printf("Invalid gate list length\n");
				goto out;
			}

			prog.list_length = count;

			for(j = 0; j < count; j++)
			{
				int state, time;

				config_setting_t *cs =
					config_setting_get_elem(setting, j);

				config_setting_lookup_int(cs, "state", &state);
				prog.acl_gate_state[j] = state;

				config_setting_lookup_int(cs, "time", &time);
				/* multiple of tick granularity = 8ns */
				prog.acl_gate_time[j] = (time)/8;
				if(prog.cycle_time != 0) {
					printf("list %d: gate_state: 0x%x gate_time: %d nS\n", 
					j, state, time);
				}
			}

			set_schedule(&prog, ifname);
		}
	}
out:
	free(path);
}
	
