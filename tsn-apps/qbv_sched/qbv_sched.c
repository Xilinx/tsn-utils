/***************************************************************
* Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
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
	int ret;

	if(!ifname)
		return;

	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

	s.ifr_data = (void *)prog;

	strcpy(s.ifr_name, ifname);

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

void get_schedule(unsigned char port, char *ifname)
{
	struct qbv_info qbv;
	struct ifreq s;
	int ret, fd, i;

	if(!ifname)
		return;

	qbv.port = port;
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

int get_interface(char *argv)
{	
        if ((!strcmp(argv, "eth1")))
                return 1;
        else if (!strcmp(argv, "eth2"))
                return 2;
        else if (!strcmp(argv, "ep"))
                return 0;
        else
		return -1;
}

void usage()
{
	printf("Usage:\n qbv_sched [-s|-g|-c] <interface> [off] [file path] -f\n");
	printf("interface : [ep/eth0/eth1/eth2]\n");
	printf("file path : location of Qbv schedule."
		"For example: /etc/qbv.cfg\n");
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
	clockid_t clkid;
	int phc_fd, i, j;
	char *port;
	char ifname[IFNAMSIZ];

	config_t cfg;
	config_setting_t *setting;
	char str[120];
	int ch, len = 0;
	char set_qbv = 0;
	char file_path = 0, off = 0;
	char *path = NULL;

	struct timespec tmx;


	if((argc < 2) || (argc > 5)) 
		usage();
	if(argc >= 4) {
		len = strlen(argv[3]);
		path = (char*)malloc((len+1)*sizeof(char));
		strcpy(path,argv[3]);
	}
	phc_fd = open( "/dev/ptp0", O_RDWR );
	if(phc_fd < 0)
		printf("/dev/ptp0 open failed\n");

	clkid = get_clockid(phc_fd);
	prog.force = 0;
	
	i = get_interface(argv[1]);

	if (i >= 0){
		strncpy(ifname, argv[1], IFNAMSIZ);
		if(argc == 3 && (strcmp(argv[2],"off") == 0)) {
			off = 1;
			prog.cycle_time = 0;
		}
		set_qbv = 1;
		goto set;	
	}
	else {
		if(argc == 2)
			usage();
	}

	while ((ch = getopt (argc, argv, "s:g:c:f")) != -1) {
                switch (ch)
                {
                        case 'g':
				if ((i = get_interface(optarg)) < 0) {
					close(phc_fd);
					usage();
				}
				strncpy(ifname, optarg, IFNAMSIZ);
				get_schedule(PORT_EP + i, ifname);
				goto out;
                        case 's':
				if ((i = get_interface(optarg)) < 0) {
					close(phc_fd);
					usage();
				}
				strncpy(ifname, optarg, IFNAMSIZ);
				set_qbv = 1;
				break;
			case 'c':
				if ((i = get_interface(optarg)) < 0) {
					close(phc_fd);
					usage();
				}
				strncpy(ifname, optarg, IFNAMSIZ);
				set_qbv = 1;
				file_path = 1;
				break;
                        case 'f':
				prog.force = 1;
				break;
			default:
				goto out;
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
		len = strlen("/etc/qbv.cfg");
		path = (char*)malloc((len+1)*sizeof(char));
		strcpy(path,"/etc/qbv.cfg");
	}
	if( !config_read_file(&cfg, path) )
	{
		config_destroy(&cfg);
		printf("Can't read %s\n", path);
		return(EXIT_FAILURE);
	}

	if (set_qbv) {
		port = port_names[i]; 
		if(off == 0) {
			sprintf(str, "qbv.%s.cycle_time", port);
			setting = config_lookup(&cfg, str);

			prog.cycle_time = config_setting_get_int(setting);
		}
		printf("Setting: %s :\n", port);

		printf("cycle_time: %d\n", prog.cycle_time);

		if(prog.cycle_time == 0)
			printf("Opening all gates\n");

		if(prog.cycle_time > MAX_CYCLE_TIME)
		{
			printf("Cycle time invalid, assuming default\n");
			prog.cycle_time = MAX_CYCLE_TIME;
		}

		prog.cycle_time = prog.cycle_time;

		prog.port = PORT_EP + i;

		sprintf(str, "qbv.%s.start_sec", port);
		setting = config_lookup(&cfg, str);
		prog.ptp_time_sec = config_setting_get_int(setting);

		sprintf(str, "qbv.%s.start_ns", port);
		setting = config_lookup(&cfg, str);
		prog.ptp_time_ns = config_setting_get_int(setting);

		if(prog.ptp_time_ns == 0 && prog.ptp_time_sec == 0)
		{
			clock_gettime(clkid, &tmx);
			prog.ptp_time_ns = 0;
			prog.ptp_time_sec = tmx.tv_sec + 1;
		}

		sprintf(str, "qbv.%s.gate_list", port);

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
	close(phc_fd);
}
	
