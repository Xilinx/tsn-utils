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
#include <sys/ioctl.h>
#include <libconfig.h>
#include <errno.h>

enum axienet_tsn_ioctl {
	SIOCCHIOCTL = SIOCDEVPRIVATE,
	SIOC_GET_SCHED,
	SIOC_PREEMPTION_CFG,
	SIOC_PREEMPTION_CTRL,
	SIOC_PREEMPTION_STS,
	SIOC_PREEMPTION_COUNTER,
	SIOC_QBU_USER_OVERRIDE,
	SIOC_QBU_STS,
	SIOC_TADMA_STR_ADD,
	SIOC_TADMA_PROG_ALL,
	SIOC_TADMA_STR_FLUSH,
	SIOC_PREEMPTION_RECEIVE,
	SIOC_TADMA_OFF,
};

struct tadma_stream {
	unsigned char dmac[6];
	unsigned short vid;
	unsigned int trigger;
	unsigned int count;
	unsigned char qno;
};

int change_to_continuous(char *ifname)
{
	struct ifreq s;
	int ret;

	if(!ifname)
		return -1;

	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

	strcpy(s.ifr_name, ifname);

	ret = ioctl(fd, SIOC_TADMA_OFF, &s);

	if(ret < 0)
	{
		perror("Cannot be changed to continuous mode");
	}
	close(fd);
}

int flush_stream(char *ifname)
{
	struct ifreq s;
	int ret;

	if(!ifname)
		return -1;

	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

	strcpy(s.ifr_name, ifname);

	ret = ioctl(fd, SIOC_TADMA_STR_FLUSH, &s);

	if(ret < 0)
	{
		perror("TADMA stream flush failed");
	}
	close(fd);
	return ret;
}
int add_stream(struct tadma_stream *stream, char *ifname)
{
	struct ifreq s;
	int ret;

	if(!ifname)
		return -1;

	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

	s.ifr_data = (void *)stream;

	strcpy(s.ifr_name, ifname);

	ret = ioctl(fd, SIOC_TADMA_STR_ADD, &s);

	if(ret < 0)
	{
		perror("TADMA stream add failed");
	}
	close(fd);
	return ret;
}

int program_all_streams(char *ifname)
{
	struct ifreq s;
	int ret;

	if(!ifname)
		return -1;

	int fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);

	s.ifr_data = (void *)NULL;

	strcpy(s.ifr_name, ifname);

	ret = ioctl(fd, SIOC_TADMA_PROG_ALL, &s);

	if(ret < 0)
	{
		perror("TADMA stream program failed");
	}
	close(fd);
}

int sortbytrigger(const void *i1, const void *i2)  {
    struct tadma_stream **a = (struct tadma_stream **)i1;
    struct tadma_stream **b = (struct tadma_stream **)i2;
    return ((*a)->trigger - (*b)->trigger);
}

void usage()
{
	printf("Usage-1 :\n tadma_prog <interface>\n");
	printf("	Default configuration of streams is in /etc/xilinx-tsn/streams.cfg\n");
	printf("Usage-2 :\n tadma_prog -c <interface> [file name]\n");
	printf("	file name : user defined streams's configuration. Ex: streams_01.cfg\n");
	printf("Usage-3 :\n tadma_prog <interface> off\n");
	printf("	Programs TADMA back to continuous mode\n");
	printf("-c : To use the configuration of streams in the file at mentioned path\n");
	exit(1);
}

int main(int argc, char **argv)
{
	config_t cfg;
	config_setting_t *setting;
	char str[120];
	char ifname[IFNAMSIZ];
	int i;
	int ch;
	struct tadma_stream **stream;
	char *path;
	int len;

	config_init(&cfg);
	if((argc < 2) || (argc > 4)) 
		usage();
	if(argc == 4 && (strcmp(argv[1],"-c") == 0)) {
		len = strlen(argv[3]);
		path = (char*)malloc((len+1)*sizeof(char));
		strcpy(path,argv[3]);
		strncpy(ifname, argv[2], IFNAMSIZ);

	} else if (argc ==3 && (strcmp(argv[2], "off") == 0)) {
		strncpy(ifname, argv[1], IFNAMSIZ);
		flush_stream(ifname);
		change_to_continuous(ifname);
		return 0;
	} else {
		if(argc == 3 || argc == 4) 
			usage();
		len = strlen("/etc/xilinx-tsn/streams.cfg");
		path = (char*)malloc((len+1)*sizeof(char));
		strcpy(path,"/etc/xilinx-tsn/streams.cfg");
		strncpy(ifname, argv[1], IFNAMSIZ);
	}
	if( !config_read_file(&cfg, path) )
	{
		config_destroy(&cfg);
		printf("Can't read %s\n", path);
		return(EXIT_FAILURE);
	}

	sprintf(str, "streams");

	setting = config_lookup(&cfg, str);
	if(setting != NULL)
	{
		int loop = config_setting_length(setting);
		if (loop == 0) {
			printf("Number of stream entries cannot be zero\n");
			usage();	
		}
		
		stream = malloc(sizeof(struct tadma_stream*) * loop);
		for(i = 0; i < loop; i++)
		stream[i] = malloc(sizeof(struct tadma_stream));
		/* TODO */
		/* if( count > stream_count) */
		for(i = 0; i < loop; i++)
		{
			const char *mac_buf;
			int vid, trigger, count, qno = -1;
			unsigned char mac[6];


			config_setting_t *cs =
				config_setting_get_elem(setting, i);

			if(!config_setting_lookup_string(cs, "dest", &mac_buf))
				continue;

			sscanf(mac_buf, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0],
			&mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);

			config_setting_lookup_int(cs, "vid", &vid);
			config_setting_lookup_int(cs, "qno", &qno);
			config_setting_lookup_int(cs, "trigger", &trigger);
			config_setting_lookup_int(cs, "count", &count);

			if(count > 4) {
				printf("Error: count value should not be greater than 4\n");
				goto end;
			}

			if (qno == -1) {
				printf("Warning: Queue number is missing. It must be specified"
				       " if number of TCs exceeds three.\n");
			}

			stream[i]->vid = (unsigned short)vid;
			stream[i]->trigger = (unsigned int)trigger;
			stream[i]->count = (unsigned int)count;
			stream[i]->qno = (unsigned char)qno;

			memcpy(stream[i]->dmac, mac, 6);

			printf("%s vid: %d, qno: %d, trigger: %d\n", mac_buf,
			       stream[i]->vid, stream[i]->qno,
			       stream[i]->trigger);
		}
		qsort(stream, loop, sizeof(struct tadma_stream*), sortbytrigger);
		for (i = 1; i < loop; i++) {
			if (stream[i]->trigger == stream[i - 1]->trigger) {
				printf("Error: Multiple streams with same trigger time %d is not supported\n",
				       stream[i]->trigger);
				goto end;
			}
		}

		flush_stream(ifname);
		for(i = 0; i < loop; i++)
		{
			//printf("vid: %d trigger: %d\n",
			//		stream[i]->vid, stream[i]->trigger);
			add_stream(stream[i], ifname);
		}
		program_all_streams(ifname);
end:
		for(i = 0; i < loop; i++)
		free(stream[i]);
		
		free(stream);
	}
}
