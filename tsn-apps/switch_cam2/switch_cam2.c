/***************************************************************
* Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
* Copyright (C) 2022 - 2023 Advanced Micro Devices, Inc. All rights reserved.
* SPDX-License-Identifier: MIT
***************************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#define ADD_CAM_ENTRY				0x18
#define DELETE_CAM_ENTRY			0x19
#define PORT_VLAN_MEM_CTRL			0x20
#define READ_CAM_ENTRY                          0x39

#define QCI

enum switch_port {
	PORT_EP = 1,
	PORT_MAC1 = 2,
	PORT_MAC2 = 4,
	PORT_EX_ONLY = 8,
	PORT_EX_EP = 16,
	PORT_PS = 24,
};

struct thershold {
	unsigned short t1;
	unsigned short t2;
};

struct static_cntr {
	unsigned int msb;
	unsigned int lsb;
};

struct mem_static_arr_cntr {
	struct static_cntr cam_lookup;
	struct static_cntr multicast_fr;
	struct static_cntr err_mac1;
	struct static_cntr err_mac2;
	struct static_cntr sc_mac1_ep;
	struct static_cntr res_mac1_ep;
	struct static_cntr be_mac1_ep;
	struct static_cntr err_sc_mac1_ep;
	struct static_cntr err_res_mac1_ep;
	struct static_cntr err_be_mac1_ep;
	struct static_cntr sc_mac2_ep;
	struct static_cntr res_mac2_ep;
	struct static_cntr be_mac2_ep;
	struct static_cntr err_sc_mac2_ep;
	struct static_cntr err_res_mac2_ep;
	struct static_cntr err_be_mac2_ep;
	struct static_cntr sc_ep_mac1;
	struct static_cntr res_ep_mac1;
	struct static_cntr be_ep_mac1;
	struct static_cntr err_sc_ep_mac1;
	struct static_cntr err_res_ep_mac1;
	struct static_cntr err_be_ep_mac1;
	struct static_cntr sc_mac2_mac1;
	struct static_cntr res_mac2_mac1;
	struct static_cntr be_mac2_mac1;
	struct static_cntr err_sc_mac2_mac1;
	struct static_cntr err_res_mac2_mac1;
	struct static_cntr err_be_mac2_mac1;
	struct static_cntr sc_ep_mac2;
	struct static_cntr res_ep_mac2;
	struct static_cntr be_ep_mac2;
	struct static_cntr err_sc_ep_mac2;
	struct static_cntr err_res_ep_mac2;
	struct static_cntr err_be_ep_mac2;
	struct static_cntr sc_mac1_mac2;
	struct static_cntr res_mac1_mac2;
	struct static_cntr be_mac1_mac2;
	struct static_cntr err_sc_mac1_mac2;
	struct static_cntr err_res_mac1_mac2;
	struct static_cntr err_be_mac1_mac2;
};

#define XAS_CAM_IPV_EN		(1 << 0)
#define XAS_CAM_VALID		(1 << 2)

#define XAS_CAM_TVMAC_EN	(1 << 0)
#define XAS_CAM_TVVLAN_EN	(1 << 1)
#define XAS_CAM_UNTAG_EN	(1 << 2)
#define XAS_CAM_TAG_EN		(1 << 3)

struct cam_struct {
	unsigned char src_addr[6];
	unsigned char dest_addr[6];
	unsigned short vlanid;
	unsigned short tv_vlanid;
	unsigned char fwd_port;
	unsigned char gate_id;
#ifdef QCI
	unsigned char ipv;
#endif
	unsigned int flags;
	unsigned char ep_port_act;
	unsigned char mac_port_act;
};

struct ff_type {
	unsigned short type1;
	unsigned short type2;
};

struct switch_data {
	unsigned int switch_status;
	unsigned int switch_ctrl;
	unsigned int switch_prt;
	unsigned char sw_mac_addr[6];
	/*0 - schedule, 1 - reserved, 2 - best effort queue*/
	struct thershold thld_ep_mac[3];
	struct thershold thld_mac_mac[3];
	unsigned int ep_vlan;
	unsigned int mac_vlan;
	unsigned int max_frame_sc_que;
	unsigned int max_frame_res_que;
	unsigned int max_frame_be_que;
	/* Memory counters */
	struct mem_static_arr_cntr mem_arr_cnt;
	/* CAM */
	struct cam_struct cam_data;
/*Frame Filtering Type Field Option */
	struct ff_type typefield;
/*MAC Port-1 Management Queueing Options */
	int mac1_config;
/*MAC Port-2 Management Queueing Options */
	int mac2_config;
/* Port VLAN Membership Registers */
	int port_vlan_mem_ctrl;
	char port_vlan_mem_data;
} data_val;

char dmac[30];
char port_s[8];
char tvmac[30] = "0";
unsigned int vlanid = 0;
unsigned int tv_vlanid = 0;
unsigned int gate_id = 0;
unsigned char ipv = 0;
int cam_mul_ports = 0;

int switch_ioctl(int cmd, struct switch_data *data)
{
	int fd;
	int ret;

	fd = open("/dev/switch", O_RDWR);	/* Driver open */
	if (fd == -1) {
		printf("open error...\n");
		return -1;
	}
	ret = ioctl(fd, cmd, data);
	close(fd);
	return ret;
}

void port_act_buf(unsigned char act, char *buf)
{
	memset(buf, 0, sizeof(buf));
	if(act & XAS_CAM_TVMAC_EN) sprintf(buf, "%s Translate MAC,", buf);
	if(act & XAS_CAM_TVVLAN_EN) sprintf(buf, "%s Translate VLAN ID,", buf);
	if(act & XAS_CAM_TAG_EN) sprintf(buf, "%s Tag VLAN ID,", buf);
	if(act & XAS_CAM_UNTAG_EN) sprintf(buf, "%s Untag VLAN ID,", buf);
	return;
}

int read_cam_entry(struct switch_data *data)
{
	int ret;
	struct cam_struct *cam;
	if (0 > switch_ioctl(READ_CAM_ENTRY, data)) {
		printf("\nswitch_cam read failed\n");
		return -1;
	}
	
	cam = (struct cam_struct *)data;

	if (cam->flags & XAS_CAM_VALID) {
		printf("CAM Entry exists\n");
		if(cam->fwd_port & PORT_EX_EP) {
			printf("Port: swpex0\n");
			printf("Port: swp0\n\tAction:\n");
			if(cam->ep_port_act & XAS_CAM_TVMAC_EN) printf("\t\tTranslate MAC\n");
			if(cam->ep_port_act & XAS_CAM_TVVLAN_EN) printf("\t\tTranslate VLAN ID\n");
			if(cam->ep_port_act & XAS_CAM_TAG_EN) printf("\t\tTag VLAN ID\n");
			if(cam->ep_port_act & XAS_CAM_UNTAG_EN) printf("\t\tUntag VLAN ID\n");
		} else	if(cam->fwd_port & PORT_EX_ONLY) {
			printf("Port: swpex0\n\tAction:\n");
			if(cam->ep_port_act & XAS_CAM_TVMAC_EN) printf("\t\tTranslate MAC\n");
			if(cam->ep_port_act & XAS_CAM_TVVLAN_EN) printf("\t\tTranslate VLAN ID\n");
			if(cam->ep_port_act & XAS_CAM_TAG_EN) printf("\t\tTag VLAN ID\n");
			if(cam->ep_port_act & XAS_CAM_UNTAG_EN) printf("\t\tUntag VLAN ID\n");
		} else	if(cam->fwd_port & PORT_EP) {
			printf("Port: swp0\n\tAction:\n");
			if(cam->ep_port_act & XAS_CAM_TVMAC_EN) printf("\t\tTranslate MAC\n");
			if(cam->ep_port_act & XAS_CAM_TVVLAN_EN) printf("\t\tTranslate VLAN ID\n");
			if(cam->ep_port_act & XAS_CAM_TAG_EN) printf("\t\tTag VLAN ID\n");
			if(cam->ep_port_act & XAS_CAM_UNTAG_EN) printf("\t\tUntag VLAN ID\n");
		}
			
		if(cam->fwd_port & PORT_MAC1 || cam->fwd_port & PORT_MAC2) {
			if(cam->fwd_port & PORT_MAC1) printf("Port: swp1\n");
			if(cam->fwd_port & PORT_MAC2) printf("Port: swp2\n");
			printf("\tAction:\n");
			if(cam->mac_port_act & XAS_CAM_TVMAC_EN) printf("\t\tTranslate MAC\n");
			if(cam->mac_port_act & XAS_CAM_TVVLAN_EN) printf("\t\tTranslate VLAN ID\n");
			if(cam->mac_port_act & XAS_CAM_TAG_EN) printf("\t\tTag VLAN ID\n");
			if(cam->mac_port_act & XAS_CAM_UNTAG_EN) printf("\t\tUntag VLAN ID\n");
		}
		printf("\n");
		printf("Gate ID: %d\n", cam->gate_id);
		if(cam->ep_port_act & XAS_CAM_TVMAC_EN || cam->mac_port_act & XAS_CAM_TVMAC_EN)
		printf("Translated MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
			cam->src_addr[0], cam->src_addr[1], cam->src_addr[2],
			cam->src_addr[3], cam->src_addr[4], cam->src_addr[5]);
		if(cam->ep_port_act & XAS_CAM_TVVLAN_EN || cam->ep_port_act & XAS_CAM_TAG_EN ||
		   cam->mac_port_act & XAS_CAM_TVVLAN_EN || cam->mac_port_act & XAS_CAM_TAG_EN)
		printf("Translated VlanID: %d\n", cam->tv_vlanid);
		if(cam->flags & XAS_CAM_IPV_EN) printf("IPV: %u\n", cam->ipv);
	} else {
		printf("CAM Entry Not Found\n");
	}
	return 0;
}

int add_cam_entry(struct switch_data *data)
{
	int ret =  switch_ioctl(ADD_CAM_ENTRY, data);
	return ret;
}

int delete_cam_entry(struct switch_data *data)
{
	int ret =  switch_ioctl(DELETE_CAM_ENTRY, data);
	return ret;
}

int port_vlan_mem_ctrl(struct switch_data *data)
{
	return switch_ioctl(PORT_VLAN_MEM_CTRL, data);
}

void usage() {
	printf("/******************************************/\n");
	printf("Usage: switch_cam2 [-edr] <arg1> <arg2> ...\n\n");
	printf("Extended CAM entry to add multiple ports actions:\n");
	printf("switch_cam2 -e --swp0|swpex0 =[dvtu] --swp1|swp2=[dvtu] -m <dest mac> -v <vlan id> -n <native vlan id> -g <gate id> -c <tv MAC> -t <tv vlan id> -i <ipv> -s <swp0 | swpex0>\n");
	printf("For deleting CAM entry :\n");
	printf("switch_cam2 -d -m <dest mac> -v <vlan id>\n");
	printf("For reading CAM entry :\n");
	printf("switch_cam2 -r -m <dest mac> -v <vlan id>\n");
	printf("options: can select any combinations of \"dvtu\" or without any option\n");
	printf("d: translate dest mac, v: translate vlan id\n");
	printf("t: tag vlan id, u: untag vlan id\n");
	printf("port: swp0 for endpoint, swp1 for Port1, swp2 for Port2 swpex0 for extended endpoint\n");
	printf("ipv : 0 to 7\n");
	printf("HELP:\n");
	printf("switch_cam2 -h\n");
	printf("/******************************************/\n");
	exit(1);
}

unsigned char port_act_list(char *arg)
{
	unsigned char act = 0;
	int i = 0;
	
	for (i = 0; i < strlen(arg); i++) {
		switch(arg[i]) {
			case 'd':
				act |= XAS_CAM_TVMAC_EN;
			break;
			case 'v':
				act |= XAS_CAM_TVVLAN_EN;
			break;
			case 't':
				act |= XAS_CAM_TAG_EN;
			break;
			case 'u':
				act |= XAS_CAM_UNTAG_EN;
			break;
			default:
				usage();
		}
	}
	return act;
}

struct option longopts[] = {
   { "swp0",	 	optional_argument, NULL, '0'},
   { "swp1",	 	optional_argument, NULL, '1'},
   { "swp2",	 	optional_argument, NULL, '2'},
   { "swpex0",	 	optional_argument, NULL, '3'},
   { "tvmac",	 	required_argument, NULL, 'c'},
   { "del",	 	no_argument,       NULL, 'd'},
   { "ext",	 	no_argument, 	   NULL, 'e'},
   { "gateid",	 	required_argument, NULL, 'g'},
   { "ipv",	 	required_argument, NULL, 'i'},
   { "dmac",	 	required_argument, NULL, 'm'},
   { "nvlanid",	 	required_argument, NULL, 'n'},
   { "read",	 	no_argument,       NULL, 'r'},
   { "sourcelist",	required_argument, NULL, 's'},
   { "tvvlanid", 	required_argument, NULL, 't'},
   { "vlanid",	 	required_argument, NULL, 'v'},
   { 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
struct switch_data data = {0};
int i, ret, opt, index, id = -1;
int cam_del = 0;
int cam_read = 0;
unsigned char dest_mac[6] = {0};
unsigned char tv_mac[6] = {0};
unsigned int port = 0;
unsigned int src_port = 0;
unsigned char en_ipv = 0;
char buf[256] = "\0";
char act_buf[128] = "\0";

if(argc < 4) {
	printf("ERROR : Not sufficient arguments\n");
	usage();
}
	

    while((opt = getopt_long(argc, argv, ":0:1:2:c:deg:i:m:n:rs:t:v:W;", longopts, NULL)) != -1) {
	switch(opt) {
	case 'd':
		printf("Deleting CAM entry\n");
		cam_del = 1;
	break;
	case 'r':
		printf("Reading CAM entry\n");
		cam_read = 1;
	break;
	case 'e':
		printf("Extended cam entry for multiple ports action\n");
		cam_mul_ports = 1;
	break;
	case 'g':
		sscanf(optarg, "%d", &gate_id);
		if (gate_id > 255) {
			printf("WARNING: gate id is %d, it must be in 0 - 255 range\n", gate_id);
		}
		sprintf(buf, "%s, %s- %d", buf, "gateid", gate_id);
	break;
	case 'c':
		sscanf(optarg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &tv_mac[0],
			&tv_mac[1], &tv_mac[2], &tv_mac[3], &tv_mac[4],
							&tv_mac[5]);
		sprintf(buf, "%s, %s- %x:%x:%x:%x:%x:%x", buf, "tv mac",
			tv_mac[0], tv_mac[1], tv_mac[2], tv_mac[3],
			tv_mac[4], tv_mac[5]);
		strcpy(tvmac, optarg);
	break;
	case 'm':
		sscanf(optarg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &dest_mac[0],
			&dest_mac[1], &dest_mac[2], &dest_mac[3], &dest_mac[4],
								&dest_mac[5]);
		sprintf(buf, "%s- %x:%x:%x:%x:%x:%x", "Dest mac", dest_mac[0],
			dest_mac[1], dest_mac[2], dest_mac[3], dest_mac[4],
			dest_mac[5]);
		strcpy(dmac, optarg);
	break;
	case 't':
		sscanf(optarg, "%d", &tv_vlanid);
		if (tv_vlanid == 0 || tv_vlanid > 4096) {
			printf("WARNING: tv vlan-id is %s, it must be in 1 - 4096 range\n", optarg);
		}
		sprintf(buf, "%s, %s- %d", buf, "tv vlanid", tv_vlanid);
	break;
	case 'v':
		sscanf(optarg, "%d", &vlanid);
		if (vlanid == 0 || vlanid > 4096) {
			printf("\n**WARNING: vlan-id is %s, it must be in 1 - 4096 range\n", optarg);
		}
		sprintf(buf, "%s, %s- %d", buf, "vlanid", vlanid);
	break;
	case 'n':
		sscanf(optarg, "%d", &vlanid);
		if (vlanid == 0 || vlanid > 4096) {
			printf("\n**WARNING: vlan-id is %s, it must be in 1 - 4096 range\n", optarg);
		}
		sprintf(buf, "%s, %s- %d", buf, "vlanid", vlanid);
	break;
	case 'i':
		sscanf(optarg, "%d", &ipv);
		if (ipv > 7) {
			printf("WARNING: ipv is %d, it must be in 0 - 7 range\n", ipv);
		}
			
		sprintf(buf, "%s, %s- %d", buf, "ipv", ipv);
		en_ipv = 1;
	break;
	case '0':
		port |= PORT_EP;
		strcpy(port_s, "swp0");
		sprintf(buf, "%s\n%s- %s", buf, "port", "swp0");
		if (optarg != NULL) {
			data.cam_data.ep_port_act = port_act_list(optarg);
			port_act_buf(data.cam_data.ep_port_act, act_buf);
			sprintf(buf, "%s %s", buf, act_buf);
		}
	break;
	case '1':
		port |= PORT_MAC1;
		strcpy(port_s, "swp1");
		sprintf(buf, "%s\n%s- %s", buf, "port", "swp1");
		if (optarg != NULL) {
			data.cam_data.mac_port_act = port_act_list(optarg);
			port_act_buf(data.cam_data.mac_port_act, act_buf);
			sprintf(buf, "%s %s", buf, act_buf);
		}
	break;
	case '2':
		port |= PORT_MAC2;
		strcpy(port_s, "swp2");
		sprintf(buf, "%s\n%s- %s", buf, "port", "swp2");
		if (optarg != NULL) {
			data.cam_data.mac_port_act = port_act_list(optarg);
			port_act_buf(data.cam_data.mac_port_act, act_buf);
			sprintf(buf, "%s %s", buf, act_buf);
		}
	break;
	case '3':
		port |= PORT_EX_ONLY;
		strcpy(port_s, "swpex0");
		sprintf(buf, "%s\n%s- %s", buf, "port", "swpex0");
		if (optarg != NULL) {
			data.cam_data.ep_port_act = port_act_list(optarg);
			port_act_buf(data.cam_data.ep_port_act, act_buf);
			sprintf(buf, "%s %s", buf, act_buf);
		}
	break;
	case 's':
		if (!strcmp(optarg, "swp0")) {
			src_port = PORT_EP; 
		} else if (!strcmp(optarg, "swpex0")) {
			src_port |= PORT_EX_ONLY; 
		}
		if (!src_port)
			usage();
		sprintf(buf, "%s, %s- %s", buf, " source port", optarg);
	break;
	case 'h':
	default:
		for (index = optind; index < argc; index++)
			printf ("Non-option argument %s\n", argv[index]);
		usage();
	break;
	}
    }
	if (!cam_del  && !cam_read && !cam_mul_ports)
		usage();

	if ((cam_del && argc != 6) || (cam_read && argc != 6)
		|| (cam_mul_ports && argc < 5))
		usage();
	if (src_port & PORT_EP) {
		if (!(port & PORT_EX_ONLY)) {
			printf("ERROR: it's an invalid scenario to have"
				" endpoint in source list and not have"
				" extended endpoint in port list\n");
			usage();
		}
		port |= PORT_PS;
	}
	if (src_port & PORT_EX_ONLY) {
		if (!(port & PORT_EP)) {
			printf("ERROR: it's an invalid scenario to have"
				" extended endpoint in source list and"
				" not have endpoint in port list\n");
			usage();
		}
		port |= PORT_PS;
	}

	if (!((port & PORT_EX_ONLY) && (port & PORT_EX_EP))) {
		if (port & PORT_EX_ONLY) {
			if (port & PORT_EP) {
				port = port & ~(PORT_EX_ONLY);
				port |= PORT_EX_EP;
			}
			else
				port |= PORT_EP;
		}
	} else {
		port |= PORT_EP;
	}

	printf("%s\n", buf);

	data.cam_data.src_addr[0] = tv_mac[0];
	data.cam_data.src_addr[1] = tv_mac[1];
	data.cam_data.src_addr[2] = tv_mac[2];
	data.cam_data.src_addr[3] = tv_mac[3];
	data.cam_data.src_addr[4] = tv_mac[4];
	data.cam_data.src_addr[5] = tv_mac[5];
	data.cam_data.dest_addr[0] = dest_mac[0];
	data.cam_data.dest_addr[1] = dest_mac[1];
	data.cam_data.dest_addr[2] = dest_mac[2];
	data.cam_data.dest_addr[3] = dest_mac[3];
	data.cam_data.dest_addr[4] = dest_mac[4];
	data.cam_data.dest_addr[5] = dest_mac[5];
	data.cam_data.vlanid = vlanid;
	data.cam_data.tv_vlanid = tv_vlanid;
	data.cam_data.fwd_port = port;
	data.cam_data.gate_id = gate_id;
	data.cam_data.ipv = ipv;
	if (en_ipv)
	data.cam_data.flags |= XAS_CAM_IPV_EN;

	if (cam_mul_ports)
		ret = add_cam_entry(&data);
	if (cam_del)
		ret = delete_cam_entry(&data);
	if (cam_read)
		ret = read_cam_entry(&data);

	if(ret)
		printf("Failed !!\n");

}
