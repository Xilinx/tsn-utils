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
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#ifdef GUI
#include "cJSON.h"
#endif

#define GET_STATUS_SWITCH			0x16
#define SET_STATUS_SWITCH			0x17
#define ADD_CAM_ENTRY				0x18
#define DELETE_CAM_ENTRY			0x19
#define PORT_VLAN_MEM_CTRL			0x20
#define SET_FRAME_TYPE_FIELD			0x21
#define SET_MAC1_MNGMNT_Q_CONFIG		0x22
#define SET_MAC2_MNGMNT_Q_CONFIG		0x23
#define READ_CAM_ENTRY                          0x39

#define QCI

enum switch_port {
	PORT_EP = 1,
	PORT_MAC1 = 2,
	PORT_MAC2 = 4,
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
#define XAS_CAM_EP_MGMTQ_EN	(1 << 1)
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
char port_s[16];
char tvmac[30] = "0";
unsigned int vlanid = 0;
unsigned int tv_vlanid = 0x12;
unsigned int gate_id = 0;
unsigned char ipv = 0;
#ifdef GUI
char *fname_untag = "/etc/switch_cam_untag.json";
char *fname_tag = "/etc/switch_cam_tag.json";
char *fname_cam = "/etc/switch_cam.json";
#endif
int cam_add = 0;
int cam_add_tag = 0;
int cam_add_untag = 0;
int cam_mul_ports = 0;

#ifdef GUI
int del_json_cam_file()
{
	FILE *fp = fopen (fname_cam, "r");
	cJSON *json, *fld, *e_dmac, *e_vlan;
	char *data, *out;
	int i, len, ret = 0;

	if (fp == NULL) {
        	return ret;
	} else {
		fseek(fp,0,SEEK_END);len=ftell(fp);fseek(fp,0,SEEK_SET);
		if (len == 0 ){
			fclose(fp);
			return ret;
		}
		data=(char*)malloc(len+1);fread(data,1,len,fp);fclose(fp);
		json=cJSON_Parse(data);
		free(data);

		fp = fopen(fname_cam, "w");
	}
	for (i = 0; i< cJSON_GetArraySize(json); i++) {
		fld = cJSON_GetArrayItem(json, i);
		e_dmac = cJSON_GetObjectItem(fld, "DMAC");
		e_vlan = cJSON_GetObjectItem(fld, "VLAN-ID");
		if (e_dmac && e_vlan) {	
			if ((!strcmp(e_dmac->valuestring, dmac)) && (e_vlan->valueint == vlanid)) {
				cJSON_DeleteItemFromArray(json, i);
				ret = 1;
				break;
			}
		}
	}
	out=cJSON_Print(json);
	cJSON_Delete(json);

	fwrite(out, strlen(out), 1, fp);
	fclose (fp);

	free(out);
	return ret;
}

int del_json_tag_file()
{
	FILE *fp = fopen (fname_tag, "r");
	cJSON *json, *fld, *e_dmac, *e_vlan;
	char *data, *out;
	int i, len, ret = 0;

	if (fp == NULL) {
        	return ret;
	} else {
		fseek(fp,0,SEEK_END);len=ftell(fp);fseek(fp,0,SEEK_SET);
		if (len == 0 ){
			fclose(fp);
			return ret;
		}
		data=(char*)malloc(len+1);fread(data,1,len,fp);fclose(fp);
		json=cJSON_Parse(data);
		free(data);

		fp = fopen(fname_tag, "w");
	}
	for (i = 0; i< cJSON_GetArraySize(json); i++) {
		fld = cJSON_GetArrayItem(json, i);
		e_dmac = cJSON_GetObjectItem(fld, "DMAC");
		e_vlan = cJSON_GetObjectItem(fld, "NVLAN-ID");
		if (e_dmac && e_vlan) {	
			if ((!strcmp(e_dmac->valuestring, dmac)) && (e_vlan->valueint == vlanid)) {
				cJSON_DeleteItemFromArray(json, i);
				ret = 1;
				break;
			}
		}
	}
	out=cJSON_Print(json);
	cJSON_Delete(json);

	fwrite(out, strlen(out), 1, fp);
	fclose (fp);

	free(out);
	return ret;
}

int del_json_untag_file()
{
	FILE *fp = fopen (fname_untag, "r");
	cJSON *json, *fld, *e_dmac, *e_vlan;
	char *data, *out;
	int i, len, ret = 0;

	if (fp == NULL) {
        	return ret;
	} else {
		fseek(fp,0,SEEK_END);len=ftell(fp);fseek(fp,0,SEEK_SET);
		if (len == 0 ){
			fclose(fp);
			return ret;
		}
		data=(char*)malloc(len+1);fread(data,1,len,fp);fclose(fp);
		json=cJSON_Parse(data);
		free(data);

		fp = fopen(fname_untag, "w");
	}
	for (i = 0; i< cJSON_GetArraySize(json); i++) {
		fld = cJSON_GetArrayItem(json, i);
		e_dmac = cJSON_GetObjectItem(fld, "DMAC");
		e_vlan = cJSON_GetObjectItem(fld, "VLAN-ID");
		if (e_dmac && e_vlan) {	
			if ((!strcmp(e_dmac->valuestring, dmac)) && (e_vlan->valueint == vlanid)) {
				cJSON_DeleteItemFromArray(json, i);
				ret = 1;
				break;
			}
		}
	}
	out=cJSON_Print(json);
	cJSON_Delete(json);

	fwrite(out, strlen(out), 1, fp);
	fclose (fp);

	free(out);
	return ret;
}

void del_json_file()
{
	if (del_json_cam_file()) {
		printf("Deleted cam entry\n");
		return;
	}
	if (del_json_untag_file()) {
		printf("Deleted untag entry\n");
		return;
	}
	if (del_json_tag_file()) {
		printf("Deleted tag entry\n");
		return;
	}
	printf("Not existed to delete\n");
	return;
}

void add_json_cam_file()
{
	FILE *fp = fopen (fname_cam, "r");
	cJSON *json, *fld, *e_dmac, *e_vlan;
	char *data, *out;
	int i, len;

	if (fp == NULL) {
		fp = fopen(fname_cam, "a+");
    		if (!fp) {
        		fprintf (stderr, "jsonout() error: file open failed '%s'.\n", 
                		fname_cam);
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
			e_dmac = cJSON_GetObjectItem(fld, "DMAC");
			e_vlan = cJSON_GetObjectItem(fld, "VLAN-ID");
			if (e_dmac && e_vlan) {	
				if ((!strcmp(e_dmac->valuestring, dmac)) && (e_vlan->valueint == vlanid)) {
					return;
				}
			}
		}
		fp = fopen(fname_cam, "w");
	}
	
	cJSON_AddItemToArray(json, fld = cJSON_CreateObject());
	cJSON_AddStringToObject(fld, "DMAC", dmac);
	cJSON_AddNumberToObject(fld, "VLAN-ID", vlanid);
	cJSON_AddStringToObject(fld, "PORT", port_s);
	cJSON_AddNumberToObject(fld, "GATE-ID", gate_id);
	cJSON_AddStringToObject(fld, "TV-MAC", tvmac);
	cJSON_AddNumberToObject(fld, "TV-VLAN-ID", tv_vlanid);
	cJSON_AddNumberToObject(fld, "IPV", ipv);

	out=cJSON_Print(json);
	cJSON_Delete(json);

	fwrite(out, strlen(out), 1, fp);
	fclose (fp);

	free(out);
}
void add_json_tag_file()
{
	FILE *fp = fopen (fname_tag, "r");
	cJSON *json, *fld, *e_dmac, *e_vlan;
	char *data, *out;
	int i, len;

	if (fp == NULL) {
		fp = fopen(fname_tag, "a+");
    		if (!fp) {
        		fprintf (stderr, "jsonout() error: file open failed '%s'.\n", 
                		fname_cam);
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
			e_dmac = cJSON_GetObjectItem(fld, "DMAC");
			e_vlan = cJSON_GetObjectItem(fld, "NVLAN-ID");
			if (e_dmac && e_vlan) {	
				if ((!strcmp(e_dmac->valuestring, dmac)) && (e_vlan->valueint == vlanid)) {
					return;
				}
			}
		}
		fp = fopen(fname_tag, "w");
	}
	
	cJSON_AddItemToArray(json, fld = cJSON_CreateObject());
	cJSON_AddStringToObject(fld, "DMAC", dmac);
	cJSON_AddNumberToObject(fld, "NVLAN-ID", vlanid);
	cJSON_AddStringToObject(fld, "PORT", port_s);
	cJSON_AddNumberToObject(fld, "TV-VLAN-ID", tv_vlanid);

	out=cJSON_Print(json);
	cJSON_Delete(json);

	fwrite(out, strlen(out), 1, fp);
	fclose (fp);

	free(out);
}

void add_json_untag_file()
{
	FILE *fp = fopen (fname_untag, "r");
	cJSON *json, *fld, *e_dmac, *e_vlan;
	char *data, *out;
	int i, len;

	if (fp == NULL) {
		fp = fopen(fname_untag, "a+");
    		if (!fp) {
        		fprintf (stderr, "jsonout() error: file open failed '%s'.\n", 
                		fname_untag);
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
			e_dmac = cJSON_GetObjectItem(fld, "DMAC");
			e_vlan = cJSON_GetObjectItem(fld, "VLAN-ID");
			if (e_dmac && e_vlan) {	
				if ((!strcmp(e_dmac->valuestring, dmac)) && (e_vlan->valueint == vlanid)) {
					return;
				}
			}
		}
		fp = fopen(fname_untag, "w");
	}
	
	cJSON_AddItemToArray(json, fld = cJSON_CreateObject());
	cJSON_AddStringToObject(fld, "DMAC", dmac);
	cJSON_AddNumberToObject(fld, "VLAN-ID", vlanid);
	cJSON_AddStringToObject(fld, "PORT", port_s);

	out=cJSON_Print(json);
	cJSON_Delete(json);

	fwrite(out, strlen(out), 1, fp);
	fclose (fp);

	free(out);
}
#endif
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
	if (0 != switch_ioctl(READ_CAM_ENTRY, data)) {
		printf("\nswitch_cam read failed\n");
		return -1;
	}
	
	cam = (struct cam_struct *)data;

	if (cam->flags & XAS_CAM_VALID) {
		printf("CAM Entry exists\n");
		if(cam->fwd_port & PORT_EP) {
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

int get_switch_status(struct switch_data *data)
{
	return switch_ioctl(GET_STATUS_SWITCH, data);
}

int set_switch_status(struct switch_data *data)
{
	return switch_ioctl(SET_STATUS_SWITCH, data);
}

int add_cam_entry(struct switch_data *data)
{
	int ret =  switch_ioctl(ADD_CAM_ENTRY, data);
#ifdef GUI
	if (ret == 0) {
		if (cam_add) {
			add_json_cam_file();
			return ret;
		}
		if (cam_add_tag) {
			add_json_tag_file();
			return ret;
		}
		if (cam_add_untag) {
			add_json_untag_file();
			return ret;
		}
	}
#endif
	return ret;
}

int delete_cam_entry(struct switch_data *data)
{
	int ret =  switch_ioctl(DELETE_CAM_ENTRY, data);
#ifdef GUI
	if (ret == 0) {
		del_json_file();
	}
#endif
	return ret;
}

int port_vlan_mem_ctrl(struct switch_data *data)
{
	return switch_ioctl(PORT_VLAN_MEM_CTRL, data);
}

int set_frame_filter_opt(struct switch_data *data)
{
	return switch_ioctl(SET_FRAME_TYPE_FIELD, data);
}

int set_mac1_mngmntq(struct switch_data *data)
{
	return switch_ioctl(SET_MAC1_MNGMNT_Q_CONFIG, data);
}

int set_mac2_mngmntq(struct switch_data *data)
{
	return switch_ioctl(SET_MAC2_MNGMNT_Q_CONFIG, data);
}

void usage() {
	printf("/******************************************/\n");
	printf("Usage: switch_cam [-atudr] <arg1> <arg2> ...\n\n");
	printf("For adding CAM entry :\n");
	printf("switch_cam -a <Dest MAC> <vlan id> <port> [gate id] [tv MAC] [tv vlan id] [ipv]\n");
	printf("For adding CAM entry to tag frame:\n");
	printf("switch_cam -t <Dest MAC> <native vlan id> <port> <tv vlan id> [gate id]\n");
	printf("For adding CAM entry to untag frame:\n");
	printf("switch_cam -u <Dest MAC> <vlan id> <port> [gate id]\n");
	printf("For deleting CAM entry :\n");
	printf("switch_cam -d <Dest MAC> <vlan id>\n");
	printf("For reading CAM entry :\n");
	printf("switch_cam -r <Dest MAC> <vlan id>\n");
	printf("Extended CAM entry to add multiple ports actions:\n");
	printf("switch_cam -e <Dest MAC> <vlan id> --swp0=[dvtu] --swp1|swp2=[dvtu] -g <gate id> -m <tv MAC> -v <tv vlan id> -i <ipv>\n");
	printf("options: can select any combinations of \"dvtu\" or without any option\n");
	printf("d: translate dest mac, v: translate vlan id\n");
	printf("t: tag vlan id, u: untag vlan id\n");
	printf("port: swp0 for endpoint, swp1 for Port1, swp2 for Port2\n");
	printf("ipv : 0 to 7\n");
	printf("[] arguments are optional\n");
	printf("HELP:\n");
	printf("switch_cam -h\n");
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
   { "swp0",	 optional_argument, NULL, '0'},
   { "swp1",	 optional_argument, NULL, '1'},
   { "swp2",	 optional_argument, NULL, '2'},
   { "gateid",	 required_argument, NULL, 'g'},
   { "tvmac",	 required_argument, NULL, 'm'},
   { "tvvlanid", required_argument, NULL, 'v'},
   { "ipv",	 required_argument, NULL, 'i'},
   { "ext",	 no_argument, 	    NULL, 'e'},
   { "add",	 no_argument,       NULL, 'a'},
   { "tag",	 no_argument,       NULL, 't'},
   { "untag",	 no_argument,       NULL, 'u'},
   { "del",	 no_argument,       NULL, 'd'},
   { "read",	 no_argument,       NULL, 'r'},
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
unsigned char en_ipv = 0;
char buf[256] = "\0";
char act_buf[128] = "\0";

if(argc < 4) {
	printf("ERROR : Not sufficient arguments\n");
	usage();
}
	sscanf(argv[2], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &dest_mac[0],
		&dest_mac[1], &dest_mac[2], &dest_mac[3], &dest_mac[4],
								&dest_mac[5]);
	sprintf(buf, "%s- %x:%x:%x:%x:%x:%x", "Dest mac", dest_mac[0],
		dest_mac[1], dest_mac[2], dest_mac[3], dest_mac[4],
		dest_mac[5]);
	strcpy(dmac, argv[2]);
	
	sscanf(argv[3], "%d", &vlanid);
	if (vlanid == 0 || vlanid > 4096) {
		printf("\n**WARNING: vlan-id is %s, it must be in 1 - 4096 range\n", argv[3]);
	}
	sprintf(buf, "%s, %s- %d", buf, "vlanid", vlanid);

    while((opt = getopt_long(argc, argv, ":atudreg:m:v:i:0:1:2:W;", longopts, NULL)) != -1) {
	switch(opt) {
	case 'a':
		printf("Adding CAM entry\n");
		cam_add = 1;
	break;
	case 't':
		printf("Adding CAM tag entry\n");
		cam_add_tag = 1;
	break;
	case 'u':
		printf("Adding CAM untag entry\n");
		cam_add_untag = 1;
	break;
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
	case 'm':
		sscanf(optarg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &tv_mac[0],
			&tv_mac[1], &tv_mac[2], &tv_mac[3], &tv_mac[4],
							&tv_mac[5]);
		sprintf(buf, "%s, %s- %x:%x:%x:%x:%x:%x", buf, "tv mac",
			tv_mac[0], tv_mac[1], tv_mac[2], tv_mac[3],
			tv_mac[4], tv_mac[5]);
		strcpy(tvmac, optarg);

	break;
	case 'v':
		sscanf(optarg, "%d", &tv_vlanid);
		if (tv_vlanid == 0 || tv_vlanid > 4096) {
			printf("WARNING: tv vlan-id is %s, it must be in 1 - 4096 range\n", optarg);
		}
		sprintf(buf, "%s, %s- %d", buf, "tv vlanid", tv_vlanid);
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
	case 'h':
	default:
		for (index = optind; index < argc; index++)
			printf ("Non-option argument %s\n", argv[index]);
		usage();
	break;
	}
    }

	if (!cam_del  && !cam_add && !cam_read && !cam_add_tag && !cam_add_untag && !cam_mul_ports)
		usage();

	if ((cam_del && argc < 4) || (cam_add && argc < 5) || (cam_read && argc < 4)
	    || (cam_add_tag && argc < 6) || (cam_add_untag && argc < 5)
	    || (cam_mul_ports && argc < 5))
		usage();

	if(cam_mul_ports) goto skip_argv;

	if (argc > 4) {
		if (strstr(argv[4], "swp0")) {
			port = PORT_EP; 
		}
		if (strstr(argv[4], "swp1")) {
			port |= PORT_MAC1; 
		}
		if (strstr(argv[4], "swp2")) {
			port |= PORT_MAC2; 
		}
		if (!port)
			usage();
		sprintf(buf, "%s, %s- %s", buf, "port", argv[4]);
		strncpy(port_s,argv[4],sizeof(port_s));
	}

	if (argc > 5) {
		if (cam_add || cam_add_untag) {
			sscanf(argv[5], "%d", &gate_id);
			if (gate_id > 255) {
				printf("WARNING: gate id is %d, it must be in 0 - 255 range\n", gate_id);
			}
			sprintf(buf, "%s, %s- %d", buf, "gateid", gate_id);
		} else if (cam_add_tag) {
			sscanf(argv[5], "%d", &tv_vlanid);
			if (tv_vlanid == 0 || tv_vlanid > 4096) {
				printf("WARNING: tv vlan-id is %s, it must be in 1-4096 range\n", argv[7]);
			}
			sprintf(buf, "%s, %s- %d", buf, "tv vlanid", tv_vlanid);
		} else {
			usage();
		}
	}
	if (argc > 6) {
		if (cam_add) {
		sscanf(argv[6], "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &tv_mac[0],
			&tv_mac[1], &tv_mac[2], &tv_mac[3], &tv_mac[4],
								&tv_mac[5]);
		sprintf(buf, "%s, %s- %x:%x:%x:%x:%x:%x", buf, "tv mac",
			tv_mac[0], tv_mac[1], tv_mac[2], tv_mac[3],
			tv_mac[4], tv_mac[5]);
		strcpy(tvmac, argv[6]);
		if (port & PORT_EP)
			data.cam_data.ep_port_act = XAS_CAM_TVMAC_EN;
		if (port & PORT_MAC1 || port & PORT_MAC2)
			data.cam_data.mac_port_act = XAS_CAM_TVMAC_EN;
		} else if (cam_add_tag) {
			sscanf(argv[6], "%d", &gate_id);
			if (gate_id > 255) {
				printf("WARNING: gate id is %d, it must be in 0 - 255 range\n", gate_id);
			}
			sprintf(buf, "%s, %s- %d", buf, "gateid", gate_id);
		}
	}
	if(argc > 7) {
		sscanf(argv[7], "%d", &tv_vlanid);
		if (tv_vlanid == 0 || tv_vlanid > 4096) {
			printf("WARNING: tv vlan-id is %s, it must be in 1-4096 range\n", argv[7]);
		}
		sprintf(buf, "%s, %s- %d", buf, "tv vlanid", tv_vlanid);
		if (port & PORT_EP)
			data.cam_data.ep_port_act |= XAS_CAM_TVVLAN_EN;
		if (port & PORT_MAC1 || port & PORT_MAC2)
			data.cam_data.mac_port_act |= XAS_CAM_TVVLAN_EN;
	}
	if(argc > 8) {
		sscanf(argv[8], "%d", &ipv);
		if (ipv > 7) {
			printf("WARNING: ipv is %d, it must be in 0 - 7 range\n", ipv);
		}
		sprintf(buf, "%s, %s- %d", buf, "ipv", ipv);
		en_ipv = 1;
	}
	if (cam_add_tag) {
		if (port & PORT_EP)
			data.cam_data.ep_port_act = XAS_CAM_TAG_EN;
		if (port & PORT_MAC1 || port & PORT_MAC2)
			data.cam_data.mac_port_act = XAS_CAM_TAG_EN;
	} else if (cam_add_untag) {
		if (port & PORT_EP)
			data.cam_data.ep_port_act = XAS_CAM_UNTAG_EN;
		if (port & PORT_MAC1 || port & PORT_MAC2)
			data.cam_data.mac_port_act = XAS_CAM_UNTAG_EN;
	}

skip_argv:
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

#if 0
	printf("dest => %0x\n", dest_mac[0]);
	printf("dest => %0x\n", dest_mac[1]);
	printf("dest => %0x\n", dest_mac[2]);
	printf("dest => %0x\n", dest_mac[3]);
	printf("dest => %0x\n", dest_mac[4]);
	printf("dest => %0x\n", dest_mac[5]);
	printf("vlan id ==> %0x\n", vlanid);
	printf("port ==> %0x\n", port);
	printf("tv => %0x\n", tv_mac[0]);
	printf("tv => %0x\n", tv_mac[1]);
	printf("tv => %0x\n", tv_mac[2]);
	printf("tv => %0x\n", tv_mac[3]);
	printf("tv => %0x\n", tv_mac[4]);
	printf("tv => %0x\n", tv_mac[5]);
	printf("tv vlan==> %0x\n", tv_vlanid);
	printf("gate_id==> %d\n", gate_id);
#endif
	if (cam_add || cam_add_tag || cam_add_untag || cam_mul_ports)
		ret = add_cam_entry(&data);
	if (cam_del)
		ret = delete_cam_entry(&data);
	if (cam_read)
		ret = read_cam_entry(&data);

	if(ret)
		printf("Failed !!\n");

}
