/***************************************************************
* Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
* Copyright (C) 2022 - 2023 Advanced Micro Devices, Inc. All rights reserved.
* SPDX-License-Identifier: MIT
***************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>


#define SET_IPV4_TSN_INTERCEPT 0x100
#define DEL_IPV4_TSN_INTERCEPT 0x101
#define FLUSH_IPV4_TSN_INTERCEPT 0x102
struct tsn_stream
{
	uint32_t src;
	uint32_t dst;
	uint16_t sport;
	uint16_t dport;
	uint8_t proto;
	uint8_t dscp;
	
	uint16_t vid;
	uint8_t pcp;
	uint8_t smac[6];
	uint8_t dmac[6];
}__attribute__((__packed__));


#if 0
struct tsn_stream test = 
{
	.src = htonl(inet_aton("10.0.0.1")),
	.dst = htonl(inet_aton("10.0.0.2")),
	.sport = htons(1024),
	.dport = htons(23),
	.proto = IPPROTO_UDP,
	.dscp  = 0,
	.vid   = 200,
	.pcp   = 4,
	.mac   = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 },
};
#endif

int main(int argc, char *argv[])
{
	int fd;
	struct tsn_stream test;
	struct in_addr ipaddr;
	unsigned char dscp;

	if ((argc != 12) && (argc !=2)) {
                fprintf(stderr, "%s: Usage \"%s <add|del|flush> <src_ip> <dest_ip> <protocol> <dscp> <src_port> <dest_port> <src_mac> <dest_mac> <vlanid> <pcp>\n<protocol> : ex:- udp=17, tcp=6\n<dscp>: 0 to 63\n",
			argv[0], argv[0]);
		return -1;
            }
	fd = open("/dev/ipic", O_RDWR);
	if (fd == -1) {
		printf("open error...\n");
		return -1;
	}
	
	if (argc == 2) {
		if (!strncmp(argv[1], "flush", 5)) {
			ioctl(fd, FLUSH_IPV4_TSN_INTERCEPT, &test);
			printf("Flushed TSN Intercept entries\n");
		} else {
			fprintf(stderr, "%s: Usage \"%s <add|del|flush> <src_ip> <dest_ip> <protocol> <dscp> <src_port> <dest_port> <src_mac> <dest_mac> <vlanid> <pcp>\n<protocol> : ex:- udp=17, tcp=6\n<dscp>: 0 to 63\n",
				argv[0], argv[0]);
			goto out;
		}

	} else {
		inet_aton(argv[2], &ipaddr);

		test.src = ipaddr.s_addr;

		inet_aton(argv[3], &ipaddr);

		test.dst = ipaddr.s_addr;

		test.proto = atoi(argv[4]) & 0xFF;

		dscp = atoi(argv[5]);
		if ((dscp >= 0) && (dscp <= 63)) {
			test.dscp = dscp & 0x3F;
		} else {
			fprintf(stderr, "%s: Usage \"%s <add|del|flush> <src_ip> <dest_ip> <protocol> <dscp> <src_port> <dest_port> <src_mac> <dest_mac> <vlanid> <pcp>\n<protocol> : ex:- udp=17, tcp=6\n<dscp>: 0 to 63\n",
					argv[0], argv[0]);
			goto out;
		}

		test.sport = htons(atoi(argv[6]));
		test.dport = htons(atoi(argv[7]));

		if (!strchr(argv[8], '-') || !strchr(argv[9], '-')) {
			fprintf(stderr, "ERROR: Separate MAC address bytes by using '-'\n For ex: 01-02-03-04-05-06\n\n");
			goto out;
		}
			
		sscanf(argv[8], "%hhx-%hhx-%hhx-%hhx-%hhx-%hhx", &test.smac[0], &test.smac[1], &test.smac[2], &test.smac[3], &test.smac[4], &test.smac[5]);
		sscanf(argv[9], "%hhx-%hhx-%hhx-%hhx-%hhx-%hhx", &test.dmac[0], &test.dmac[1], &test.dmac[2], &test.dmac[3], &test.dmac[4], &test.dmac[5]);

		test.vid = atoi(argv[10]);
		test.pcp = atoi(argv[11]);

		if (!strncmp(argv[1], "add", 3)) {
			ioctl(fd, SET_IPV4_TSN_INTERCEPT, &test);
			printf("Added TSN Intercept\n");
		} else if (!strncmp(argv[1], "del", 3)) {
			ioctl(fd, DEL_IPV4_TSN_INTERCEPT, &test);
			printf("Deleted TSN Intercept\n");
		} else {
			fprintf(stderr, "%s: Usage \"%s <add|del|flush> <src_ip> <dest_ip> <protocol> <dscp> <src_port> <dest_port> <src_mac> <dest_mac> <vlanid> <pcp>\n<protocol> : ex:- udp=17, tcp=6\n<dscp>: 0 to 63\n",
					argv[0], argv[0]);
		}
	}
out:
	close(fd);
	return 0;

}
