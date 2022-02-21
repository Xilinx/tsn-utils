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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <getopt.h>
#ifdef GUI
#include "cJSON.h"
#endif

#define SET_MAC_ADDR_LEARN_CONFIG               0x30
#define GET_MAC_ADDR_LEARN_CONFIG               0x31
#define GET_MAC_ADDR_LEARNT_LIST                0x32
#define SET_PORT_STATUS                         0x33
#define GET_PORT_STATUS                         0x34
#define ADD_PORT_VLAN                           0x35
#define DEL_PORT_VLAN                           0x36
#define SET_VLAN_MAC_ADDR_LEARN_CONFIG          0x37
#define GET_VLAN_MAC_ADDR_LEARN_CONFIG          0x38
#define SET_PORT_NATIVE_VLAN                    0x3A
#define GET_PORT_NATIVE_VLAN                    0x3B
#define GET_VLAN_MAC_ADDR_LEARN_CONFIG_VLANM    0x3C

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

#define no_argument 0
#define required_argument 1

int fd;

struct mac_addr_learn {
        bool aging;
        bool is_age;
        bool learning;
        bool is_learn;
        bool learn_untag;
        bool is_untag;
        u32 aging_time;
};

struct mac_learnt {
        u8 mac_addr[6];
        u16 vlan_id;
};

#define MAX_NUM_MAC_ENTRIES     2048
struct mac_addr_list {
        u8 port_num;
        u16 num_list;
        struct mac_learnt list[MAX_NUM_MAC_ENTRIES];
};

struct port_vlan {
        bool aging;
        bool is_age;
        bool learning;
        bool is_learn;
        bool is_mgmtq;
	bool en_ipv;
	bool en_port_status;
        u8 mgmt_ext_id;
        u8 port_num;
	u8 ipv;
	u8 port_status;
        u16 vlan_id;
        u32 aging_time;
};

struct port_status {
        u8 port_num;
        u8 port_status;
};

struct native_vlan {
	bool en_ipv;
        u8 port_num;
        u8 ipv;
        u16 vlan_id;
};

unsigned int vlanid = 0;
unsigned char port_s0 = 0;
unsigned char port_s1 = 0;
unsigned char port_s2 = 0;
unsigned char ipv = 0;
unsigned char vaging = 0;
unsigned char vlearning = 0;
unsigned int vaging_time = 0;
#ifdef GUI
char *fname_vlanm = "/etc/switch_vlanm.json";
char *fname_set_vlan = "/etc/set_switch_vlan_ale.json";

void set_vlan_json_prog_file()
{
	FILE *fp = fopen (fname_set_vlan, "r");
	cJSON *json, *fld, *e_vlan, *e_age, *e_learn, *e_time;
	char *data, *out;
	int i, len, addition = 1;

	if (fp == NULL) {
		fp = fopen(fname_set_vlan, "a+");
		if (!fp) {
			fprintf (stderr, "jsonout() error: file open failed '%s'.\n", 
					fname_set_vlan);
			return;
		}
		json=cJSON_CreateArray();
	} else {
		fseek(fp,0,SEEK_END);len=ftell(fp);fseek(fp,0,SEEK_SET);
		if (len == 0 ){
			json=cJSON_CreateArray();
			fclose(fp);
		} else {
			data=(char*)malloc(len+1);fread(data,1,len,fp);fclose(fp);
			json=cJSON_Parse(data);
			free(data);

		}
		for (i = 0; i< cJSON_GetArraySize(json); i++) {
			fld = cJSON_GetArrayItem(json, i);
			e_vlan = cJSON_GetObjectItem(fld, "VLAN-ID");
			e_age = cJSON_GetObjectItem(fld, "AGING");
			e_learn = cJSON_GetObjectItem(fld, "LEARNING");
			e_time = cJSON_GetObjectItem(fld, "AGING TIME");
			if (e_vlan, e_age, e_learn, e_time) {	
				if (e_vlan->valueint == vlanid) {
					if (vaging == 0 && vlearning == 0) {
						cJSON_DeleteItemFromArray(json, i);
						addition = 0;
						break;
					}
					if (e_age->valueint == vaging && e_learn->valueint == vlearning && e_time->valueint == vaging_time) { 
						return;
					}
					cJSON_DeleteItemFromArray(json, i);
				}
			}
		}
		fp = fopen(fname_set_vlan, "w");
	}
	if (vaging == 0 && vlearning == 0) {
		addition = 0;
	}
	if (addition) {
		cJSON_AddItemToArray(json, fld = cJSON_CreateObject());
		cJSON_AddNumberToObject(fld, "VLAN-ID", vlanid);
		cJSON_AddNumberToObject(fld, "LEARNING", vlearning);
		cJSON_AddNumberToObject(fld, "AGING", vaging);
		cJSON_AddNumberToObject(fld, "AGING TIME", vaging_time);
	}
	out=cJSON_Print(json);
	cJSON_Delete(json);

	fwrite(out, strlen(out), 1, fp);
	fclose (fp);
	free(out);
}

void vlanm_add_json_prog_file()
{
	FILE *fp = fopen (fname_vlanm, "r");
	cJSON *json, *fld, *e_port0, *e_port1, *e_port2, *e_vlan;
	char *data, *out;
	int i, len;

	if (fp == NULL) {
		fp = fopen(fname_vlanm, "a+");
		if (!fp) {
			fprintf (stderr, "jsonout() error: file open failed '%s'.\n", 
					fname_vlanm);
			return;
		}
		json=cJSON_CreateArray();
	} else {
		fseek(fp,0,SEEK_END);len=ftell(fp);fseek(fp,0,SEEK_SET);
		if (len == 0 ){
			json=cJSON_CreateArray();
			fclose(fp);
		} else {
			data=(char*)malloc(len+1);fread(data,1,len,fp);fclose(fp);
			json=cJSON_Parse(data);
			free(data);

		}
		for (i = 0; i< cJSON_GetArraySize(json); i++) {
			fld = cJSON_GetArrayItem(json, i);
			e_port0 = cJSON_GetObjectItem(fld, "SWP0");
			e_port1 = cJSON_GetObjectItem(fld, "SWP1");
			e_port2 = cJSON_GetObjectItem(fld, "SWP2");
			e_vlan = cJSON_GetObjectItem(fld, "VLAN-ID");
			if (e_port0 && e_port1 && e_port2 && e_vlan) {	
				if (e_vlan->valueint == vlanid) {
					if (e_port0->valueint == port_s0 && e_port1->valueint == port_s1 && e_port2->valueint == port_s2) { 
						return;
					}
					if (port_s0 == 1) {
						cJSON_DeleteItemFromArray(json, i);
						port_s1 = e_port1->valueint;
						port_s2 = e_port2->valueint;
						break;
					}
					if (port_s1 == 1) {
						cJSON_DeleteItemFromArray(json, i);
						port_s0 = e_port0->valueint;
						port_s2 = e_port2->valueint;
						break;
					}
					if (port_s2 == 1) {
						cJSON_DeleteItemFromArray(json, i);
						port_s1 = e_port1->valueint;
						port_s0 = e_port0->valueint;
						break;
					}
				}
			}
		}
		fp = fopen(fname_vlanm, "w");
	}	
	cJSON_AddItemToArray(json, fld = cJSON_CreateObject());
	cJSON_AddNumberToObject(fld, "VLAN-ID", vlanid);
	cJSON_AddNumberToObject(fld, "SWP0", port_s0);
	cJSON_AddNumberToObject(fld, "SWP1", port_s1);
	cJSON_AddNumberToObject(fld, "SWP2", port_s2);
	cJSON_AddNumberToObject(fld, "IPV", ipv);

	out=cJSON_Print(json);
	cJSON_Delete(json);

	fwrite(out, strlen(out), 1, fp);
	fclose (fp);

	free(out);
}
void vlanm_del_json_prog_file()
{
	FILE *fp = fopen (fname_vlanm, "r");
	cJSON *json, *fld, *e_port0, *e_port1, *e_port2, *e_vlan, *e_ipv;
	char *data, *out;
	int i, len, ret = 0;

	if (fp == NULL) {
        	return;
	} else {
		fseek(fp,0,SEEK_END);len=ftell(fp);fseek(fp,0,SEEK_SET);
		if (len == 0 ){
			fclose(fp);
			return;
		}
		data=(char*)malloc(len+1);fread(data,1,len,fp);fclose(fp);
		json=cJSON_Parse(data);
		free(data);
		fp = fopen(fname_vlanm, "w");
	}
	for (i = 0; i< cJSON_GetArraySize(json); i++) {
		fld = cJSON_GetArrayItem(json, i);
		e_port0 = cJSON_GetObjectItem(fld, "SWP0");
		e_port1 = cJSON_GetObjectItem(fld, "SWP1");
		e_port2 = cJSON_GetObjectItem(fld, "SWP2");
		e_vlan = cJSON_GetObjectItem(fld, "VLAN-ID");
		e_ipv = cJSON_GetObjectItem(fld, "IPV");
		if (e_port0 && e_port1 && e_port2 && e_vlan) {	
			if (e_vlan->valueint == vlanid) {
				if (port_s0 == 0) {
					e_port0->valueint = 0;
				}
				if (port_s1 == 0) {
					e_port1->valueint = 0;
				}
				if (port_s2 == 0) {
					e_port2->valueint = 0;
				}
				if (e_port0->valueint == 0 && e_port1->valueint == 0 && e_port2->valueint == 0)  {
					cJSON_DeleteItemFromArray(json, i);
					break;
				}
				cJSON_AddItemToArray(json, fld = cJSON_CreateObject());
				cJSON_AddNumberToObject(fld, "VLAN-ID", vlanid);
				cJSON_AddNumberToObject(fld, "SWP0", e_port0->valueint);
				cJSON_AddNumberToObject(fld, "SWP1", e_port1->valueint);
				cJSON_AddNumberToObject(fld, "SWP2", e_port2->valueint);
				cJSON_AddNumberToObject(fld, "IPV", e_ipv->valueint);
				cJSON_DeleteItemFromArray(json, i);
				break;
			}
		}
	}
	out=cJSON_Print(json);
	cJSON_Delete(json);

	fwrite(out, strlen(out), 1, fp);
	fclose (fp);

	free(out);
	return;
}
#endif
void get_port_state(struct port_status *sts)
{
	if (0 > ioctl(fd, GET_PORT_STATUS, sts)) {
		printf("\nswitch get port state failed\n");
		return;
	}
	printf("\n");
	switch(sts->port_status) {
		case 0:
			printf("Port State: %u-Disable/Initialization State\n", sts->port_status);
			break;
		case 1:
			printf("Port State: %u-Blocking State\n", sts->port_status);
			break;
		case 2:
			printf("Port State: %u-Listening State\n", sts->port_status);
			break;
		case 3:
			printf("Port State: %u-Learning State\n", sts->port_status);
			break;
		case 4:
			printf("Port State: %u-Forwarding State\n", sts->port_status);
			break;
		case 5:
			printf("Port State: %u-Flush State\n", sts->port_status);
			break;
		default:
			printf("Port State: %u-Unknown State\n", sts->port_status);
			break;
	}
	return;
}

void get_port_vlan(struct port_vlan *port)
{
	u8 i = 0;

	if (0 > ioctl(fd, GET_VLAN_MAC_ADDR_LEARN_CONFIG_VLANM, port)) {
		printf("\nswitch read vlanm port failed\n");
		return;
	}
	//printf("\nPort num: %u\n", port->port_num);
	printf("\nPort:\n");
	
	for (i = 0 ; i < 3; i++) {
		if (port->port_num & (1 << i)) {
			switch (i) {
				case 0:
					printf("\tswp0\n");
					break;
				case 1:
					printf("\tswp1\n");
					break;
				case 2:
					printf("\tswp2\n");
					break;
			}
		}
	}
	if(port->learning) {
		printf("Port Status: %u\n", port->port_status);
		printf("VLAN Based Port Status Enabled: %u\n", port->en_port_status);
	}
	else {	
		printf("IPV: %u\n", port->ipv);
		printf("IPV enabled: %u\n", port->en_ipv);
	}
	return;
}

void get_hw_learn_config(void)
{
	struct mac_addr_learn learn;
	
	if (0 > ioctl(fd, GET_MAC_ADDR_LEARN_CONFIG, &learn)) {
		printf("\nswitch get hw address learning config failed\n");
		return;
	}
	//printf("\n");
	printf("HW address learning: %u\n", !learn.learning);
	printf("HW address aging: %u\n", !learn.aging);
	printf("HW address aging time: %usecs\n", (u32)(learn.aging_time * 0.134));
	printf("HW address learning UNTAG packets: %u\n", learn.learn_untag);
	return;
}

void get_vlan_hw_learn_config(struct port_vlan *port)
{
	if (0 > ioctl(fd, GET_VLAN_MAC_ADDR_LEARN_CONFIG, port)) {
		printf("\nswitch get vlan hw address learning config failed\n");
		return;
	}
	printf("\n");
//	printf("HW address learning for vlan id: %u\n", port->vlan_id);
	printf("HW address learning: %u\n", port->learning);
	printf("HW address aging: %u\n", port->aging);
	printf("HW address aging time: %usecs\n", (u32)(port->aging_time * 0.134));
	//printf("Port list: %u\n", port->port_num);
	return;
}

void get_mac_list(struct mac_addr_list *mac)
{
	int i = 0;
	if (0 > ioctl(fd, GET_MAC_ADDR_LEARNT_LIST, mac)) {
		printf("\nswitch get hw address learnt entries failed\n");
		return;
	}
	printf("\nTotally learnt %d number of entries in port %s\n", mac->num_list,
	       (mac->port_num == 2) ? "swp1" : "swp2");

	for(i = 0; i< MAX_NUM_MAC_ENTRIES; i++) {
		if(mac->list[i].vlan_id) {
			printf("%u -- %02X:%02X:%02X:%02X:%02X:%02X\n",
			mac->list[i].vlan_id, mac->list[i].mac_addr[0],
			mac->list[i].mac_addr[1], mac->list[i].mac_addr[2],
			mac->list[i].mac_addr[3], mac->list[i].mac_addr[4],
			mac->list[i].mac_addr[5]);
		}
	}
	return;
}

void get_port_nlv(struct native_vlan *port)
{
	u8 i = 0;

	if (0 > ioctl(fd, GET_PORT_NATIVE_VLAN, port)) {
		printf("\nswitch get vlan port failed\n");
		return;
	}
	printf("\nNative vlan-id: %u\n", port->vlan_id);
	printf("Native ipv: %u\n", port->ipv);
	return;
}

void usage() {
	printf("/******************************************/\n\n");
	printf("Usage: switch_prog ale -s --vlanid=<0 to 4095> --learning=<0|1> --aging=<0|1> --time=<aging_time 0 to 1000000secs>\n"); 
	printf("By mentioning vlan_id the HW address learning and aging configuration"
	       " applies to the specific vlan_id, otherwise it sets as switch"
	       " global configuration\n");

	printf("Usage: switch_prog ale -s --learning=<0|1> --untag\n");
	printf("--untag: set --learning 0 or 1 to learn HW MAC address for untag packets\n");
	
	printf("Usage: switch_prog ale -g\n");
	printf("Shows the switch global HW address learning configuration\n");
	printf("Usage: switch_prog ale -g --vlanid=<0 to 4095>\n");
	printf("Shows the specific vlan id's HW address learning configuration\n");
	printf("Usage: switch_prog ale -g --list_ent=<swp1 | swp2>\n");
	printf("Lists all HW address learnt for specific port\n");
	printf("swp1 for TEMAC1, swp2 for TEMAC2\n");
	printf("/******************************************/\n\n");

	printf("Usage: switch_prog pst -s <swp0 | swp1 | swp2 | swpex0> --state=<state 0 to 5>\n"); 
	printf("Usage: switch_prog pst -g <swp0 | swp1 | swp2 | swpex0>\n"); 
	printf("Set/Get the port state\n");
	printf("0 - Dsiable/Initialization state\n1 - Blocking state\n"
	      	"2 - Listening state\n3 - Learning state\n"
		"4 - Forwarding state\n5 - Flush Entries\n");
	printf("swp0 for endpoint, swp1 for TEMAC1, swp2 for TEMAC2 swpex0 for extended endpoint\n");
	printf("/******************************************/\n\n");

	printf("Usage: switch_prog vlanm -a <swp0 | swp1 | swp2> --vlanid=<0 to 4095> --ipv=<0 to 7> --vpst=<0 to 5>\n");
	printf("Usage: switch_prog vlanm -d <swp0 | swp1 | swp2> --vlanid=<0 to 4095>\n");
	printf("Usage: switch_prog vlanm -r --vlanid=<0 to 4095>\n");
	printf("Add/Delete/Get port to the specific vlan id\n");
	printf("swp0 for endpoint, swp1 for TEMAC1, swp2 for TEMAC2\n");
	printf("/******************************************/\n\n");
	
	printf("Usage: switch_prog nvl -s <swp0 | swp1 | swp2> --vlanid=<0 to 4095> --ipv=<0 to 7>\n");
	printf("Usage: switch_prog nvl -g <swp0 | swp1 | swp2>\n");
	printf("HELP:\n");
	printf("switch_prog -h\n");
	printf("/******************************************/\n");

	exit(1);
}

struct option longopts_ale[] = {
   { "vlanid",   required_argument, NULL, 'v'},
   { "aging",    required_argument, NULL, 'a'},
   { "learning", required_argument, NULL, 'l'},
   { "time",     required_argument, NULL, 't'},
   { "list_ent", required_argument, NULL, 'e'},
   { "untag",    no_argument,       NULL, 'u'},
   { "help",     no_argument,       NULL, 'h'},
   { "set",      no_argument,       NULL, 's'},
   { "get",      no_argument,       NULL, 'g'},
   { 0, 0, 0, 0 }
};

int prog_ale(int argc, char *argv[])
{
	struct mac_addr_learn learn = {0};
	struct mac_addr_list *list = malloc (sizeof(struct mac_addr_list));;
	struct port_vlan port = {0};
	signed char c;
	u8 set = 0; 
	u8 get = 0;
	u8 set_vlan = 0;
	u8 is_age = 0;
	u8 is_learn = 0;
	u32 age_time = 0;
	int vlan_id, aging, learning, num_ent, port_num, untag = 0;

	if(argc < 3) {
		printf("ERROR : Not sufficient arguments\n");
		usage();
	}
	
        fd = open("/dev/switch", O_RDWR);       /* Driver open */
        if (fd == -1) {
                printf("open error...\n");
                return -1;
        }
	
	if (!strcmp(argv[2], "-s")) {
		set = 1;
	} else if (!strcmp(argv[2], "-g")) {
		get = 1;
		if(argc == 3) {
			get_hw_learn_config();
			goto end;
		}
	} else {
		close(fd);
		usage();
	}

	while ((c = getopt_long(argc, argv, ":sghuv:a:l:t:e:W;", longopts_ale, NULL)) != -1) {
		switch (c) {
			case 'v':
				vlan_id = atoi(optarg);
				vlanid = vlan_id;
				if((vlan_id < 0) || (vlan_id > 4095)) {
					close(fd);
					usage();
				}
				printf("vlanid-%d  ", vlan_id);
				port.vlan_id = vlan_id;
				if (get) {
					get_vlan_hw_learn_config(&port);
					goto end;
				}
				set_vlan = 1;
				break;
			case 'a':
				aging = atoi(optarg);
				vaging = aging;
				if(!(aging == 0 || aging == 1)) {
					close(fd);
					usage();
				}
				is_age = 1;
				printf("aging-%d ", aging);
				break;
			case 'l':
				learning = atoi(optarg);
				vlearning = learning;
				if(!(learning == 0 || learning == 1)) {
					close(fd);
					usage();
				}
				is_learn = 1;
				printf("learning-%d ", learning);
				break;
			case 't':
				age_time = atoi(optarg);
				vaging_time = age_time;
				printf("aging_time-%d ", age_time);
				if((age_time < 0) || (age_time > 1000000)) {
					close(fd);
					usage();
				}
				age_time = age_time/0.134;
				//printf("actual age_time %d ***", age_time);
				break;
			case 'e':
				if (!(strcmp(optarg, "swp1"))) {
					list->port_num = 2;
				} else if (!(strcmp(optarg, "swp2"))) { 
					list->port_num = 4;
				} else {
					close(fd);
					usage();
				}
				printf("port-%s ", optarg);
				get_mac_list(list);
				goto end;
				break;
			case 's':
			case 'g':
				break;
			case 'u':
				untag = 1;
				printf("untag ");
				break;
			case 'h':
				close(fd);
				usage();
				break;
			case ':':   /* missing option argument */
				printf("%s: option `-%c' requires an argument\n",
						argv[0], optopt);
				goto end;
				break;
			case '?':
			default:    /* invalid option */
				printf("%s: option `-%c' is invalid: ignored\n",
						argv[0], optopt);
				goto end;
				break;
		}
	}
	if (untag) {
		if (is_learn) {
			learn.is_untag = 1;
			learn.learn_untag = learning;
				if (0 > ioctl(fd, SET_MAC_ADDR_LEARN_CONFIG, &learn)) {
					printf("\nswitch hw address learning for untag prog failed\n");
				} else {
					printf("\nSuccessfully set the untag learning configuration\n");
				}
				goto end;
		}
		else {
			close(fd);
			usage();
		}
	}

	if (set_vlan && set) {
		if (is_learn) {port.is_learn = 1; port.learning = learning;}
		if (is_age) {port.is_age = 1; port.aging = aging;}
		if (age_time) {port.aging_time = age_time;}
		if (0 > ioctl(fd, SET_VLAN_MAC_ADDR_LEARN_CONFIG, &port)) {
			printf("\nswitch hw address learning config for vlan_id %d prog failed\n", vlan_id);
		} else {
#ifdef GUI
			set_vlan_json_prog_file();
#endif
			printf("\nSuccessfully set the vlan-id HW address learning configuration\n");
		}
	} else if (set){
		if (is_learn) {learn.is_learn = 1; learn.learning = learning;}
		if (is_age) {learn.is_age = 1; learn.aging = aging;}
		if (age_time) {learn.aging_time = age_time;}
		if (0 > ioctl(fd, SET_MAC_ADDR_LEARN_CONFIG, &learn)) {
			printf("\nswitch global hw address learning config prog failed\n");
		} else {
			printf("\nSuccessfully set the Global HW address learning configuration\n");
		}
	}
end:
	close(fd);
	return 0;
}

struct option longopts_pst_vlanm[] = {
   { "vlanid",  required_argument, NULL, 'v'},
   { "ipv",     required_argument, NULL, 'i'},
   { "vpst",    required_argument, NULL, 'p'},
   { "state",   required_argument, NULL, 't'},
   { "set",     required_argument, NULL, 's'},
   { "add",     required_argument, NULL, 'a'},
   { "del",     required_argument, NULL, 'd'},
   { "get",     required_argument, NULL, 'g'},
   { "read",    no_argument, 	   NULL, 'r'},
   { "help",    no_argument,       NULL, 'h'},
   { 0, 0, 0, 0 }
};

int prog_pst_vlam(int argc, char *argv[])
{
	struct port_status sts = {0};
	struct port_vlan port = {0};
	signed char c;
	u8 state = 0;
	u8 set = 0; 
	u8 get = 0;
	u8 add = 0;
	u8 del = 0;
	u8 vlan = 0;
	u8 read = 0;
	int value = 38;

	if(argc < 3) {
		printf("ERROR : Not sufficient arguments\n");
		usage();
	}
	
        fd = open("/dev/switch", O_RDWR);       /* Driver open */
        if (fd == -1) {
                printf("open error...\n");
                return -1;
        }
	
	while ((c = getopt_long(argc, argv, ":hrg:s:a:d:v:i:p:t:W;", longopts_pst_vlanm, NULL)) != -1) {
		switch (c) {
			case 'v':
				port.vlan_id = atoi(optarg);
				vlanid = port.vlan_id;
				printf("vlanid-%d ", port.vlan_id);
				if((port.vlan_id < 0) || (port.vlan_id > 4095)) {
					close(fd);
					usage();
				}
				vlan = 1;
				break;
			case 'i':
				port.ipv = atoi(optarg);
				ipv = port.ipv;
				port.en_ipv = 1;
				printf("ipv-%d ", port.ipv);
				if((port.ipv < 0) || (port.ipv > 7)) {
					close(fd);
					usage();
				}
				break;
			case 'p':
				port.port_status = atoi(optarg);
				port.en_port_status = 1;
				if (port.port_status < 0 || port.port_status > 4) {
					close(fd);
					usage();
				}
				printf("state-%d ", port.port_status);
				break;
			case 'a':
				if (!(strcmp(optarg, "swp0"))) {
					port_s0 = 1;
					port.port_num = 1;
				} else if (!(strcmp(optarg, "swp1"))) {
					port_s1 = 1;
					port.port_num = 2;
				} else if (!(strcmp(optarg, "swp2"))) { 
					port_s2 = 1;
					port.port_num = 4;
				} else {
					close(fd);
					usage();
				}
				add = 1;
				printf("port-%s ", optarg);
				break;
			case 'd':
				if (!(strcmp(optarg, "swp0"))) {
					port_s0 = 0;
					port_s1 = 1;
					port_s2 = 1;
					port.port_num = 1;
				} else if (!(strcmp(optarg, "swp1"))) {
					port_s1 = 0;
					port_s0 = 1;
					port_s2 = 1;
					port.port_num = 2;
				} else if (!(strcmp(optarg, "swp2"))) { 
					port_s2 = 0;
					port_s1 = 1;
					port_s0 = 1;
					port.port_num = 4;
				} else {
					close(fd);
					usage();
				}
				del = 1;
				printf("port-%s ", optarg);
				break;
			case 'r':
				read = 1;
				break;
			case 's':
				if (!(strcmp(optarg, "swp0"))) {
					sts.port_num = 1;
				} else if (!(strcmp(optarg, "swp1"))) {
					sts.port_num = 2;
				} else if (!(strcmp(optarg, "swp2"))) { 
					sts.port_num = 4;
				} else if (!(strcmp(optarg, "swpex0"))) { 
					sts.port_num = 8;
				} else {
					close(fd);
					usage();
				}
				set = 1;
				printf("port-%s ", optarg);
				break;
			case 'g':
				if (!(strcmp(optarg, "swp0"))) {
					sts.port_num = 1;
				} else if (!(strcmp(optarg, "swp1"))) {
					sts.port_num = 2;
				} else if (!(strcmp(optarg, "swp2"))) { 
					sts.port_num = 4;
				} else if (!(strcmp(optarg, "swpex0"))) { 
					sts.port_num = 8;
				} else {
					close(fd);
					usage();
				}
				printf("port-%s ", optarg);
				get = 1;
				break;

			case 't':
				sts.port_status = atoi(optarg);
				if (sts.port_status < 0 || sts.port_status > 5) {
					close(fd);
					usage();
				}
				printf("state-%d ", sts.port_status);
				state = 1;
				break;
			case 'h':
				usage();
				break;
			case ':':   /* missing option argument */
				printf("%s: option `-%c' requires an argument\n",
						argv[0], optopt);
				break;
			case '?':
			default:    /* invalid option */
				printf("%s: option `-%c' is invalid: ignored\n",
						argv[0], optopt);
				break;
		}
	}
	if (set && state) {
		if (0 > ioctl(fd, SET_PORT_STATUS, &sts)) {
			printf("\nswitch set port status failed\n");
			goto end;
		}
		printf("\nSuccessfully set the port state\n");
	}
	else if (get) {
		get_port_state(&sts);
	}
	else if (add) {
		if (0 > (value = ioctl(fd, ADD_PORT_VLAN, &port))) {
			printf("\nswitch add port to vlan failed\n");
			goto end;
		}
#ifdef GUI
		vlanm_add_json_prog_file();
#endif
		printf("\nSuccessfully added\n");
	}
	else if (del) {
		if (0 > ioctl(fd, DEL_PORT_VLAN, &port)) {
			printf("\nswitch del port to vlan failed\n");
			goto end;
		}
#ifdef GUI
		vlanm_del_json_prog_file();
#endif
		printf("\nSuccessfully deleted\n");
	}
	else if (read && vlan) {
		get_port_vlan(&port);
	}
	else {
		close(fd);
		usage();
	}
end:
	close(fd);
	return 0;
}

struct option longopts_nvl[] = {
   { "vlanid",  required_argument, NULL, 'v'},
   { "ipv",     required_argument, NULL, 'i'},
   { "set",     required_argument, NULL, 's'},
   { "get",     required_argument, NULL, 'g'},
   { "help",    no_argument,       NULL, 'h'},
   { 0, 0, 0, 0 }
};

int prog_nvl(int argc, char *argv[])
{
	struct native_vlan port = {0};
	signed char c;
	u8 set = 0; 
	u8 get = 0;

	if(argc < 4) {
		printf("ERROR : Not sufficient arguments\n");
		usage();
	}
	
        fd = open("/dev/switch", O_RDWR);       /* Driver open */
        if (fd == -1) {
                printf("open error...\n");
                return -1;
        }
	
	while ((c = getopt_long(argc, argv, ":hg:s:v:i:W;", longopts_nvl, NULL)) != -1) {
		switch (c) {
			case 'v':
				port.vlan_id = atoi(optarg);
				printf("vlanid-%d ", port.vlan_id);
				if((port.vlan_id < 0) || (port.vlan_id > 4095)) {
					close(fd);
					usage();
				}
				break;
			case 'i':
				port.ipv = atoi(optarg);
				port.en_ipv = 1;
				printf("ipv-%d ", port.ipv);
				if((port.ipv < 0) || (port.ipv > 7)) {
					close(fd);
					usage();
				}
				break;
			case 's':
				if (!(strcmp(optarg, "swp0"))) {
					port.port_num = 1;
				} else if (!(strcmp(optarg, "swp1"))) {
					port.port_num = 2;
				} else if (!(strcmp(optarg, "swp2"))) { 
					port.port_num = 4;
				} else {
					close(fd);
					usage();
				}
				set = 1;
				printf("port-%s ", optarg);
				break;
			case 'g':
				if (!(strcmp(optarg, "swp0"))) {
					port.port_num = 1;
				} else if (!(strcmp(optarg, "swp1"))) {
					port.port_num = 2;
				} else if (!(strcmp(optarg, "swp2"))) { 
					port.port_num = 4;
				} else {
					close(fd);
					usage();
				}
				get = 1;
				printf("port-%s ", optarg);
				break;
			case 'h':
				usage();
				break;
			case ':':   /* missing option argument */
				printf("%s: option `-%c' requires an argument\n",
						argv[0], optopt);
				break;
			case '?':
			default:    /* invalid option */
				printf("%s: option `-%c' is invalid: ignored\n",
						argv[0], optopt);
				break;
		}
	}
	if (set) {
		if (0 > ioctl(fd, SET_PORT_NATIVE_VLAN, &port)) {
			printf("\nswitch set port native vlan failed\n");
			goto end;
		}
		printf("\nSuccessfully set the native vlan\n");
	}
	else if (get) {
		get_port_nlv(&port);
	} else {
		close(fd);
		usage();
	}
end:
	close(fd);
	return 0;

}

int main(int argc, char *argv[])
{
	int ret = 0;

	if(argc < 3) {
		printf("ERROR : Not sufficient arguments\n");
		usage();
	}

	if (!strcmp(argv[1], "pst")) {
		ret = prog_pst_vlam(argc, argv);
	} else if (!strcmp(argv[1], "vlanm")) {
		ret = prog_pst_vlam(argc, argv);
	} else if (!strcmp(argv[1], "ale")) {
		ret = prog_ale(argc, argv);
	} else if (!strcmp(argv[1], "nvl")) {
		ret = prog_nvl(argc, argv);
	} else {
		usage();
	}
	return ret;
}
