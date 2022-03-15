/***************************************************************
* Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
* SPDX-License-Identifier: MIT
***************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#ifdef GUI
#include "cJSON.h"
#endif

#define CONFIG_MEMBER_MEM			0x2A
#define CONFIG_INGRESS_FLTR			0x2B
#define FRER_CONTROL				0x2C
#define GET_STATIC_FRER_COUNTER			0x2D
#define GET_MEMBER_REG				0x2E
#define GET_INGRESS_FLTR			0x2F

#define WR_EN_FRER_MASK				0x3
#define WR_EN_FRER_INGRESS_MASK			0x2
#define WR_EN_MEMB_MASK				0x1
#define WR_EN_ALL				0x0

#define WRITE_MASK				0x1
#define READ_MASK				0x0

struct static_cntr {
	unsigned int msb;
	unsigned int lsb;
};

/* CB Structures */
struct frer_ctrl {
	unsigned char gate_id;
	unsigned char memb_id;
	bool seq_reset;
	bool gate_state;
	bool rcvry_tmout;
	bool frer_valid;
	unsigned char wr_op_type;
	bool op_type;
};

struct frer_memb_config {
	unsigned char seq_rec_hist_len;
	unsigned char split_strm_egport_id;
	unsigned short split_strm_vlan_id;
	unsigned short rem_ticks;
};

struct in_fltr {
	unsigned char in_port_id;
	unsigned short max_seq_id;
};

/* FRER Static counter*/
struct frer_static_counter {
	struct static_cntr frer_fr_count;
	struct static_cntr disc_frames_in_portid;
	struct static_cntr pass_frames_seq_recv;
	struct static_cntr disc_frames_seq_recv;
	struct static_cntr rogue_frames_seq_recv;
	struct static_cntr pass_frames_ind_recv;
	struct static_cntr disc_frames_ind_recv;
	struct static_cntr seq_recv_rst;
	unsigned char num;
};

/* CB Core stuctures */
struct cb {
	struct frer_ctrl frer_ctrl_data;
	struct in_fltr in_fltr_data;
	struct frer_memb_config frer_memb_config_data;
	struct frer_static_counter frer_counter_data;
};

int switch_cb_ioctl(int cmd, struct cb *data)
{
	int fd;

	fd = open("/dev/switch", O_RDWR);	/* Driver open */
	if (fd == -1) {
		printf("ERROR: open error...\n");
	return -1;
	}
	ioctl(fd, cmd, data);
	close(fd);
	return 0;
}

int frer_control(struct cb *data)
{
	return switch_cb_ioctl(FRER_CONTROL, data);
}

int config_all(struct cb data)
{
	if (switch_cb_ioctl(CONFIG_INGRESS_FLTR, &data)) {
		printf("ERROR: write error to stream filter register...\n");
		return -1;
	}
	if (switch_cb_ioctl(CONFIG_MEMBER_MEM, &data)) {
		printf("ERROR: write error to stream filter register...\n");
		return -1;
	}
	data.frer_ctrl_data.wr_op_type = WR_EN_ALL;
	data.frer_ctrl_data.op_type = WRITE_MASK;
	return frer_control(&data);
}

char port_s[8];
#ifdef GUI
char *fname_ingress = "/etc/cb_ingress.json";
char *fname_egress = "/etc/cb_egress.json";
void add_ingress_json(struct cb *cb)
{
	FILE *fp = fopen (fname_ingress, "r"); /* open file for reading */
	cJSON *json, *fld, *e_gate;
	char *data, *out;
	int i, len;

	if (fp == NULL) {
		fp = fopen(fname_ingress, "a+");
    		if (!fp) {
        		fprintf (stderr, "jsonout() error: file open failed '%s'.\n", 
                		fname_ingress);
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
			e_gate = cJSON_GetObjectItem(fld, "GATE-ID");
			if (e_gate && (e_gate->valueint == cb->frer_ctrl_data.gate_id)) {
					cJSON_DeleteItemFromArray(json, i);
			}
		}
		fp = fopen(fname_ingress, "w");
	}
	
	cJSON_AddItemToArray(json, fld = cJSON_CreateObject());
	cJSON_AddNumberToObject(fld, "GATE-ID", cb->frer_ctrl_data.gate_id);
	cJSON_AddStringToObject(fld, "PORT", port_s);
	cJSON_AddNumberToObject(fld, "MEMBER-ID", cb->frer_ctrl_data.memb_id);
	cJSON_AddNumberToObject(fld, "SEQ-REC-LEN", cb->frer_memb_config_data.seq_rec_hist_len);
	cJSON_AddNumberToObject(fld, "REM-TICKS", cb->frer_memb_config_data.rem_ticks);
	cJSON_AddNumberToObject(fld, "SEQ-RST", cb->frer_ctrl_data.seq_reset);
	cJSON_AddNumberToObject(fld, "REC-TIMOUT-VALID", cb->frer_ctrl_data.rcvry_tmout);
	cJSON_AddNumberToObject(fld, "GATE-STATE", cb->frer_ctrl_data.gate_state);
	cJSON_AddNumberToObject(fld, "FRER-VALID", cb->frer_ctrl_data.frer_valid);

	out=cJSON_Print(json);
	cJSON_Delete(json);

	fwrite(out, strlen(out), 1, fp);
	fclose (fp);

	free(out);
}
#endif
int config_ingress_filter(unsigned char in_port_pid, unsigned char seq_rec_hist_len,
	unsigned char gate_id, unsigned char memb_id, unsigned short rem_ticks,
	bool seq_reset, bool rcvry_tmout, bool gate_state, bool frer_valid)
{
	struct cb data = {0};
	int ret = 0;

	data.in_fltr_data.in_port_id = in_port_pid;
	data.frer_memb_config_data.seq_rec_hist_len = seq_rec_hist_len;
	data.frer_memb_config_data.rem_ticks = rem_ticks;
	if (switch_cb_ioctl(CONFIG_INGRESS_FLTR, &data)) {
		printf("ERROR: write error to stream filter register...\n");
		return -1;
	}

	/* program FRER for write */
	data.frer_ctrl_data.gate_id = gate_id;
	data.frer_ctrl_data.memb_id = memb_id;
	data.frer_ctrl_data.seq_reset = seq_reset;
	data.frer_ctrl_data.rcvry_tmout = rcvry_tmout;
	data.frer_ctrl_data.gate_state = gate_state;
	data.frer_ctrl_data.frer_valid = frer_valid;
	data.frer_ctrl_data.wr_op_type = WR_EN_ALL;
	data.frer_ctrl_data.op_type = WRITE_MASK;

	ret = frer_control(&data);
#ifdef GUI
	if (!ret) {
		add_ingress_json(&data);
	}
#endif
	return ret;
}

#ifdef GUI
void add_egress_json(struct cb *cb)
{
	FILE *fp = fopen (fname_egress, "r"); /* open file for reading */
	cJSON *json, *fld, *e_gate;
	char *data, *out;
	int i, len;

	if (fp == NULL) {
		fp = fopen(fname_egress, "a+");
    		if (!fp) {
        		fprintf (stderr, "jsonout() error: file open failed '%s'.\n", 
                		fname_egress);
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
			e_gate = cJSON_GetObjectItem(fld, "GATE-ID");
			if (e_gate) {
				cJSON_DeleteItemFromArray(json, i);
			}
		}
		fp = fopen(fname_egress, "w");
	}
	
	cJSON_AddItemToArray(json, fld = cJSON_CreateObject());
	cJSON_AddStringToObject(fld, "PORT", port_s);
	cJSON_AddNumberToObject(fld, "MAX-SEQ-ID", cb->in_fltr_data.max_seq_id);
	cJSON_AddNumberToObject(fld, "VLAN-ID", cb->frer_memb_config_data.split_strm_vlan_id);
	cJSON_AddNumberToObject(fld, "GATE-ID", cb->frer_ctrl_data.gate_id);
	cJSON_AddNumberToObject(fld, "MEMBER-ID", cb->frer_ctrl_data.memb_id);
	cJSON_AddNumberToObject(fld, "GATE-STATE", cb->frer_ctrl_data.gate_state);
	cJSON_AddNumberToObject(fld, "FRER-VALID", cb->frer_ctrl_data.frer_valid);

	out=cJSON_Print(json);
	cJSON_Delete(json);

	fwrite(out, strlen(out), 1, fp);
	fclose (fp);

	free(out);
}
#endif

int program_member_reg(unsigned int max_seq_id, unsigned char split_strm_egport_id,
			unsigned short split_strm_vlan_id, unsigned char gate_id,
			int memb_id, bool gate_state, bool frer_valid)
{
	struct cb data = {0};
	int ret = 0;

	data.frer_memb_config_data.split_strm_egport_id = split_strm_egport_id;
	data.in_fltr_data.max_seq_id = max_seq_id;
	data.frer_memb_config_data.split_strm_vlan_id = split_strm_vlan_id;

	if (switch_cb_ioctl(CONFIG_MEMBER_MEM, &data)) {
		printf("ERROR: write error to stream filter register...\n");
		return -1;
	}

	/* program FRER for write */
	data.frer_ctrl_data.gate_id = gate_id;
	data.frer_ctrl_data.memb_id = memb_id;
	data.frer_ctrl_data.gate_state = gate_state;
	data.frer_ctrl_data.frer_valid = frer_valid;
	data.frer_ctrl_data.wr_op_type = WR_EN_ALL;
	data.frer_ctrl_data.op_type = WRITE_MASK;

	ret = frer_control(&data);
#ifdef GUI
	if (!ret) {
		add_egress_json(&data);
	}
#endif
	return ret;
}

int get_member_reg(struct cb *data, unsigned char memb_id)
{
	data->frer_ctrl_data.memb_id = memb_id;
	data->frer_ctrl_data.op_type = READ_MASK;

	if (frer_control(data)) {
		printf("ERROR: programming FRER register for read op failed\n");
		return -1;
	}

	return switch_cb_ioctl(GET_MEMBER_REG, data);
}

int get_ingress_filter_config(struct cb *data, unsigned char gate_id)
{
	data->frer_ctrl_data.gate_id = gate_id;
	data->frer_ctrl_data.op_type = READ_MASK;

	if (frer_control(data)) {
		printf("ERROR: programming FRER register for read op failed\n");
		return -1;
	}

	return switch_cb_ioctl(GET_INGRESS_FLTR, data);
}

int get_frer_counter(struct cb *data, unsigned char num)
{

	data->frer_counter_data.num = num;
	return switch_cb_ioctl(GET_STATIC_FRER_COUNTER, data);
}

#define ST_ARG_NUM			4
#define MEMB_ARG_NUM			5
#define ALL_ARG_NUM			8
#define FRER_ARG_NUM			4

void usage() {
	printf("\n/******************************************/\n");
	printf("Usage: cb_prog [-ima] <arg1> <arg2> ...\n\n");
	printf("For programing ingress filter memory :\n");
	printf("cb_prog -i <ingress port> <seq rec history len> <gate id> <member id> <remaining ticks> <seq reset> <recv timout valid> <gate state> <frer valid>\n");
	printf("(last 5 args are optional here, default value will be 1)\n\n");
	printf("For programing member memory :\n");
	printf("cb_prog -m <max seq id> <EgressPort> <VLAN ID> <gate id> <member id> <gate state> <frer valid>\n\n");
	printf("(last 2 args are optional here, default value will be 1)\n\n");
	printf("For both memory in one shot:\n");
	printf("cb_prog -a <ingress port> <max seq id> <seq rec history len>"	\
	" <EgressPort> <VLAN ID> <Remaining Ticks>"		\
	" <gate id> <member id> <seq reset> <recv timout valid> <gate state> <frer valid> \n");
	printf("(last 4 args are optional here, default value will be 0)\n\n");
	printf("For reading ingress filter memory:\n");
	printf("cb_prog -r ingress <gate id>\n\n");
	printf("For reading member memory:\n");
	printf("cb_prog -r member <member id>\n\n");
	printf("For reading FRER statistic counters:\n");
	printf("cb_prog -r counter <gate id>\n\n");
	printf("HELP:\n");
	printf("cb_prog -h\n\n");
	printf("All values in decimals, except ingress/egress port - swp1 | swp2\n");
	printf("/******************************************/\n");
	exit(1);
}

int main(int argc, char **argv)
{

struct cb data = {0};
int i, ret, opt, index, id = -1;
int prog_ingress_fltr =  0;
int prog_member_mem = 0;
int prog_all = 0;
int in_fltr[9] = {0, 0, 0, 0, 0, 0, 0, 1, 1};
unsigned int member[7] = {0, 0, 0, 0, 0, 1, 1};
unsigned int all[13] = {0};
unsigned char *cvalue = NULL;
unsigned long value;

if(argc < 2) {
	printf("ERROR : Not sufficient arguments\n");
	usage();
}

while((opt = getopt(argc, argv, "hi:m:c:a:r:")) != -1)
{
	switch(opt)
	{
	case 'i':
		i = 0;
		for (index = optind - 1; index < argc; index++) {
			if(argv[index][0] == '-')
				break;
			if (index == 2) {
				if (!strcmp(argv[index], "swp1")) {
					in_fltr[i++] = 1;
					strcpy(port_s, argv[index]);
				} else if (!strcmp(argv[index], "swp2")) {
					in_fltr[i++] = 2;
					strcpy(port_s, argv[index]);
				} else if (!strcmp(argv[index], "noport")) {
					in_fltr[i++] = 3;
					strcpy(port_s, argv[index]);
				} else {
					printf("ERROR : port name is invalid\n");
					usage();
				}
			} else {
				in_fltr[i++] = atoi(argv[index]);
			}
		}
		prog_ingress_fltr = i;
	break;
	case 'm':
		i = 0;
		for (index = optind - 1; index < argc; index++) {
			if(argv[index][0] == '-')
				break;
			if (index == 3) {
				if (!strcmp(argv[index], "swp1")) {
					member[i++] = 1;
					strcpy(port_s, argv[index]);
				} else if (!strcmp(argv[index], "swp2")) {
					member[i++] = 2;
					strcpy(port_s, argv[index]);
				} else if (!strcmp(argv[index], "noport")) {
					member[i++] = 3;
					strcpy(port_s, argv[index]);
				} else {
					printf("ERROR : port name is invalid\n");
					usage();
				}
			} else {
				member[i++] = atoi(argv[index]);
			}
		}
		prog_member_mem = i;
	break;
	case 'a':
		i = 0;
		for (index = optind - 1; index < argc; index++) {
			if(argv[index][0] == '-')
				break;
			if (index == 2 || index == 5) {
				if (!strcmp(argv[index], "swp1")) {
					all[i++] = 1;
				} else if (!strcmp(argv[index], "swp2")) {
					all[i++] = 2;
				} else if (!strcmp(argv[index], "noport")) {
					all[i++] = 3;
				} else {
					printf("ERROR : port name is invalid\n");
					usage();
				}
			} else {
				all[i++] = atoi(argv[index]);
			}
		}
		prog_all = i;
	break;
	case 'r':
		cvalue = optarg;
		if(argv[optind])
			id = atoi(argv[optind]);
	break;
	
	case 'h':
	default:
		for (index = optind; index < argc; index++)
			printf ("Non-option argument %s\n", argv[index]);
		usage();
	break;
			
	}
}

if (prog_ingress_fltr) {
	if((prog_ingress_fltr < ST_ARG_NUM) || (prog_ingress_fltr > ST_ARG_NUM + 5)) {
		printf("ERROR: Wrong number of arguments for"	\
			" programming ingress filter memory !\n");
		usage();
	}
	ret = !config_ingress_filter(in_fltr[0], in_fltr[1], in_fltr[2],
		in_fltr[3], in_fltr[4], in_fltr[5], in_fltr[6], in_fltr[7], in_fltr[8]);
	if (ret)
		printf("Gate ID: %d Config Ingress Filter Memory Success !!\n", in_fltr[2]);
	else
		printf("Gate ID: %d Config Ingress Filter Memory Failed !!\n", in_fltr[2]);
}

if (prog_member_mem) {
	if((prog_member_mem < MEMB_ARG_NUM) || (prog_member_mem > MEMB_ARG_NUM + 2)) {
		printf("ERROR: Wrong number of arguments for "	\
				"programming member memory !\n");
		usage();
	}
	ret = !program_member_reg(member[0], member[1], member[2], member[3],
					member[4], member[5], member[6]);
	if (ret)
		printf("Config Member Memory for Egress Success !!\n");
	else
		printf("Config Member Memory for Egress Failed !!\n");
}

if (prog_all) {
	if((prog_all < ALL_ARG_NUM) || (prog_all > ALL_ARG_NUM + 4)) {
		printf("ERROR: Wrong number of arguments for "	\
				"programming all memory !\n");
		usage();
	}
	data.in_fltr_data.in_port_id = all[0];
	data.in_fltr_data.max_seq_id = all[1];
	data.frer_memb_config_data.seq_rec_hist_len = all[2];
	data.frer_memb_config_data.split_strm_egport_id = all[3];
	data.frer_memb_config_data.split_strm_vlan_id = all[4];
	data.frer_memb_config_data.rem_ticks = all[5];
	data.frer_ctrl_data.gate_id = all[6];
	data.frer_ctrl_data.memb_id = all[7];
	data.frer_ctrl_data.seq_reset = all[8];
	data.frer_ctrl_data.rcvry_tmout = all[9];
	data.frer_ctrl_data.gate_state = all[10];
	data.frer_ctrl_data.frer_valid = all[11];
	ret = !config_all(data);
	if (ret)
		printf("Config All Success !!\n");
	else
		printf("Config All Failed !!\n");
}

if (cvalue) {
	if (id == -1) {
		printf ("Required id value for reading\n");
		usage();
	}
	if(!strcmp(cvalue, "ingress")) {
		printf("Reading ingress filter memory\n");
		get_ingress_filter_config(&data, id);
		printf("Ingress port : %d\n", data.in_fltr_data.in_port_id);
		printf("Max Sequence ID : %d\n", data.in_fltr_data.max_seq_id);
	}
	else if(!strcmp(cvalue, "member")) {
		printf("Reading Member memory\n");
		get_member_reg(&data, id);
		printf("Sequence Recovery History Length : %u\n", data.frer_memb_config_data.seq_rec_hist_len);
		printf("Split Stream Egress Port ID : %u\n", data.frer_memb_config_data.split_strm_egport_id);
		printf("Split Stream VLAN ID : %u\n", data.frer_memb_config_data.split_strm_vlan_id);
		printf("Remaining Ticks : %u\n", data.frer_memb_config_data.rem_ticks);
	}
	else if(!strcmp(cvalue, "counter")) {
		printf("Reading Counters value of gate-id: %d\n", id);
		get_frer_counter(&data, id);
		value = (data.frer_counter_data.frer_fr_count.msb << 16 | data.frer_counter_data.frer_fr_count.lsb) ;
		printf("Frame Count : %lu\n", value);
		value = (data.frer_counter_data.disc_frames_in_portid.msb << 16 | data.frer_counter_data.disc_frames_in_portid.lsb);
		printf("FRER Discarded Frames by Ingress Filter : %lu\n", value);
		value = (data.frer_counter_data.seq_recv_rst.msb << 16 | data.frer_counter_data.seq_recv_rst.lsb);
		printf("Sequence Recovery Resets : %lu\n", value);
		value = (data.frer_counter_data.pass_frames_ind_recv.msb << 16 | data.frer_counter_data.pass_frames_ind_recv.lsb);
		printf("FRER Passed Frames [Individual Recovery] : %lu\n", value);
		value = (data.frer_counter_data.disc_frames_ind_recv.msb << 16 | data.frer_counter_data.disc_frames_ind_recv.lsb);
		printf("FRER Discarded Frames [Individual Recovery]: %lu\n", value);
		value = (data.frer_counter_data.pass_frames_seq_recv.msb << 16 | data.frer_counter_data.pass_frames_seq_recv.lsb);
		printf("FRER Passed Frames [Sequence Recovery]: %lu\n", value);
		value = (data.frer_counter_data.disc_frames_seq_recv.msb << 16 | data.frer_counter_data.disc_frames_seq_recv.lsb);
		printf("FRER Discarded Frames [Sequence Recovery]: %lu\n", value);
		value = (data.frer_counter_data.rogue_frames_seq_recv.msb << 16 | data.frer_counter_data.rogue_frames_seq_recv.lsb);
		printf("FRER Rogue Frames [Sequence Recovery]: %lu\n", value);
	}
	else {
		printf("Wrong argument value\n");
		usage();
	}
}
return 0;

}
