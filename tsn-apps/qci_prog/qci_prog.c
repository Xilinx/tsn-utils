/***************************************************************
* Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
* SPDX-License-Identifier: MIT
***************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#ifdef GUI
#include "cJSON.h"
#endif

#define CONFIG_METER_MEM			0x24
#define CONFIG_GATE_MEM				0x25
#define PSFP_CONTROL				0x26
#define GET_STATIC_PSFP_COUNTER			0x27
#define GET_METER_REG				0x28
#define GET_STREAM_FLTR_CONFIG			0x29

#define WR_EN_PSFP_MASK				0x3
#define WR_EN_PSFP_FILTER_MASK			0x2
#define WR_EN_METER_MASK			0x1
#define WR_EN_ALL				0x0

#define WRITE_MASK				0x1
#define READ_MASK				0x0

typedef unsigned int u32;

struct static_cntr {
	unsigned int msb;
	unsigned int lsb;
};

/* QCI Structures */
struct psfp_config {
	unsigned char gate_id;
	unsigned char meter_id;
	bool en_meter;
	bool allow_stream;
	bool en_psfp;
	unsigned char wr_op_type;
	bool op_type;
};

struct meter_config {
	unsigned int cir;
	unsigned int eir;
	unsigned int cbr;
	unsigned int ebr;
	unsigned char mode;
};

struct stream_filter {
	unsigned char in_pid; /* ingress port id*/
	unsigned short max_fr_size; /* max frame size*/
};

/* PSFP Static counter*/
struct psfp_static_counter {
	struct static_cntr psfp_fr_count;
	struct static_cntr err_filter_ins_port;
	struct static_cntr err_filtr_sdu;
	struct static_cntr err_meter;
	unsigned char num;
};

/* QCI Core stuctures */
struct qci {
	struct meter_config meter_conf_data;
	struct stream_filter stream_conf_data;
	struct psfp_config psfp_conf_data;
	struct psfp_static_counter psfp_counter_data;
};

int switch_qci_ioctl(int cmd, struct qci *data)
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

#if 0
int enable_metering (bool state) {
	struct qci data = {0};

	/* program PSFP for write */
	data.psfp_conf_data.en_meter = state;
	data.psfp_conf_data.wr_op_type = WR_EN_PSFP_MASK;
	data.psfp_conf_data.op_type = WRITE_MASK;
	return psfp_control(&data);
}

int enable_streaming (bool state) {
	struct qci data = {0};

	/* program PSFP for write */
	data.psfp_conf_data.allow_stream = state;
	data.psfp_conf_data.wr_op_type = WR_EN_PSFP_MASK;
	data.psfp_conf_data.op_type = WRITE_MASK;
	return psfp_control(&data);
}

int enable_streaming (bool state) {
	struct qci data = {0};

	/* program PSFP for write */
	data.psfp_conf_data.en_psfp = state;
	data.psfp_conf_data.wr_op_type = WR_EN_PSFP_MASK;
	data.psfp_conf_data.op_type = WRITE_MASK;
	return psfp_control(&data);
}
#endif

int psfp_control(struct qci *data)
{
	return switch_qci_ioctl(PSFP_CONTROL, data);
}

int config_all(struct qci data)
{
	if (switch_qci_ioctl(CONFIG_GATE_MEM, &data)) {
		printf("ERROR: write error to stream filter register...\n");
		return -1;
	}
	if (switch_qci_ioctl(CONFIG_METER_MEM, &data)) {
		printf("ERROR: write error to stream filter register...\n");
		return -1;
	}
	data.psfp_conf_data.wr_op_type = WR_EN_ALL;
	data.psfp_conf_data.op_type = WRITE_MASK;
	return psfp_control(&data);
}

char port_s[8];
#ifdef GUI
char *fname_filter = "/etc/qci_filtering.json";
char *fname_meter = "/etc/qci_metering.json";

void add_filter_json(struct qci *qci)
{
	FILE *fp = fopen (fname_filter, "r");
	cJSON *json, *fld, *e_gate;
	char *data, *out;
	int i, len;

	if (fp == NULL) {
		fp = fopen(fname_filter, "a+");
    		if (!fp) {
        		fprintf (stderr, "jsonout() error: file open failed '%s'.\n", 
                		fname_filter);
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
			if (e_gate && (e_gate->valueint == qci->psfp_conf_data.gate_id)) {
					cJSON_DeleteItemFromArray(json, i);
			}
		}
		fp = fopen(fname_filter, "w");
	}
	
	cJSON_AddItemToArray(json, fld = cJSON_CreateObject());
	cJSON_AddNumberToObject(fld, "GATE-ID", qci->psfp_conf_data.gate_id);
	cJSON_AddStringToObject(fld, "PORT", port_s);
	cJSON_AddNumberToObject(fld, "MAX-FS", qci->stream_conf_data.max_fr_size);
	cJSON_AddNumberToObject(fld, "METER-EN", qci->psfp_conf_data.en_meter);
	cJSON_AddNumberToObject(fld, "FILTER-EN", qci->psfp_conf_data.allow_stream);
	cJSON_AddNumberToObject(fld, "PSFP-EN", qci->psfp_conf_data.en_psfp);

	out=cJSON_Print(json);
	cJSON_Delete(json);

	fwrite(out, strlen(out), 1, fp);
	fclose (fp);

	free(out);

}

void add_meter_json(struct qci *qci)
{
	FILE *fp = fopen (fname_meter, "r");
	cJSON *json, *fld, *e_meter;
	char *data, *out;
	int i, len;
	unsigned int k = 0;

	if (fp == NULL) {
		fp = fopen(fname_meter, "a+");
    		if (!fp) {
        		fprintf (stderr, "jsonout() error: file open failed '%s'.\n", 
                		fname_meter);
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
			e_meter = cJSON_GetObjectItem(fld, "METER-ID");
			if (e_meter && (e_meter->valueint == qci->psfp_conf_data.meter_id)) {
					cJSON_DeleteItemFromArray(json, i);
			}
		}
		fp = fopen(fname_meter, "w");
	}
	
	cJSON_AddItemToArray(json, fld = cJSON_CreateObject());
	cJSON_AddNumberToObject(fld, "METER-ID", qci->psfp_conf_data.meter_id);
	cJSON_AddNumberToObject(fld, "GATE-ID", qci->psfp_conf_data.gate_id);
	k = round(qci->meter_conf_data.cir / 4294967.295);
	cJSON_AddNumberToObject(fld, "CIR", k);
	k = round(qci->meter_conf_data.eir / 4294967.295);
	cJSON_AddNumberToObject(fld, "EIR", k);
	cJSON_AddNumberToObject(fld, "CBR", qci->meter_conf_data.cbr);
	cJSON_AddNumberToObject(fld, "EBR", qci->meter_conf_data.ebr);
	cJSON_AddNumberToObject(fld, "MODE", qci->meter_conf_data.mode);

	out=cJSON_Print(json);
	cJSON_Delete(json);

	fwrite(out, strlen(out), 1, fp);
	fclose (fp);

	free(out);

}
#endif
int config_stream_filter(unsigned char in_pid, unsigned int max_fr_size,
			unsigned char gate_id, bool meter_state,
			bool fl_state, bool psfp_state)
{
	struct qci data = {0};
	int ret = 0;

	data.stream_conf_data.in_pid = in_pid;
	data.stream_conf_data.max_fr_size = max_fr_size;
	if (switch_qci_ioctl(CONFIG_GATE_MEM, &data)) {
		printf("ERROR: write error to stream filter register...\n");
		return -1;
	}

	/* program PSFP for write */
	data.psfp_conf_data.en_meter = meter_state;
	data.psfp_conf_data.allow_stream = fl_state;
	data.psfp_conf_data.en_psfp = psfp_state;
	data.psfp_conf_data.gate_id = gate_id;
	data.psfp_conf_data.wr_op_type = WR_EN_PSFP_FILTER_MASK;
	data.psfp_conf_data.op_type = WRITE_MASK;

	ret = psfp_control(&data);
#ifdef GUI
	if (!ret) {
		add_filter_json(&data);
	}
#endif
	return ret;
}

int program_meter_reg(unsigned int cir, unsigned int eir, unsigned int cbr,
		unsigned int ebr, unsigned char mode, int meter_id, int gate_id)
{
	struct qci data = {0};
	int ret = 0;

	data.meter_conf_data.cir = cir;
	data.meter_conf_data.eir = eir;
	data.meter_conf_data.cbr = cbr;
	data.meter_conf_data.ebr = ebr;
	data.meter_conf_data.mode = mode;

	if (switch_qci_ioctl(CONFIG_METER_MEM, &data)) {
		printf("ERROR: write error to stream filter register...\n");
		return -1;
	}

	/* program PSFP for write */
	data.psfp_conf_data.meter_id = meter_id;
	data.psfp_conf_data.wr_op_type = WR_EN_METER_MASK;
	data.psfp_conf_data.op_type = WRITE_MASK;
	data.psfp_conf_data.gate_id = gate_id;

	ret = psfp_control(&data);
#ifdef GUI
	if (!ret) {
		add_meter_json(&data);
	}
#endif

	return ret;
}

int get_meter_reg(struct qci *data, unsigned char meter_id)
{
	data->psfp_conf_data.meter_id = meter_id;
	data->psfp_conf_data.op_type = READ_MASK;

	if (psfp_control(data)) {
		printf("ERROR: programming psfp register for read op failed\n");
		return -1;
	}

	return switch_qci_ioctl(GET_METER_REG, data);
}

int get_stream_filter_config(struct qci *data, unsigned char gate_id)
{
	data->psfp_conf_data.gate_id = gate_id;
	data->psfp_conf_data.op_type = READ_MASK;

	if (psfp_control(data)) {
		printf("ERROR: programming psfp register for read op failed\n");
		return -1;
	}

	return switch_qci_ioctl(GET_STREAM_FLTR_CONFIG, data);
}

int get_psfp_counter(struct qci *data, unsigned char num)
{

	data->psfp_counter_data.num = num;
	return switch_qci_ioctl(GET_STATIC_PSFP_COUNTER, data);
}

#define ST_ARG_NUM			3
#define MTR_ARG_NUM			7
#define ALL_ARG_NUM			9
#define PSFP_ARG_NUM			4

void usage() {
	printf("/******************************************/\n");
	printf("Usage: qci_prog [-sma] <arg1> <arg2> ...\n\n");
	printf("For programing streaming filter memory :\n");
	printf("qci_prog -s <ingress port> <max frame size> <gate id> <meter en> <stream en> <psfp en>\n");
	printf("(meter en, stream en and psfp en are optional here, default value will be 0)\n\n");
	printf("For programing meter memory :\n");
	printf("qci_prog -m <CIR> <EIR> <CBR> <EBR> <mode> <meter id> <gate id>\n\n");
	printf("For both memory in one shot:\n");
	printf("qci_prog -a <ingress port> <max frame size> <CIR> <EIR> <CBR>" \
			" <EBR> <mode> <gate id> <meter id> "	\
			"<meter gate state> <filtering gate stream> "	\
			"<psfp valid>\n");
	printf("(last 3 options are optional here, default value will be 0)\n\n");
	printf("For reading stream filter memory:\n");
	printf("qci_prog -r stream <gate id>\n\n");
	printf("For reading meter memory:\n");
	printf("qci_prog -r meter <meter id>\n\n");
	printf("For reading PSFP statistic counters:\n");
	printf("qci_prog -r counter <gate id>\n\n");
	printf("HELP:\n");
	printf("qci_prog -h\n\n");
	printf("All values in decimals, except ingress port - swp1 | swp2\n");
	printf("/******************************************/\n");
	exit(1);
}

int main(int argc, char **argv)
{

struct qci data = {0};
int i, ret, opt, index, id = -1;
int prog_strm_fltr =  0;
int prog_meter_mem = 0;
int prog_all = 0;
int st_fltr[6] = {0x3, 0x7D0, 0, 0, 0, 0};
unsigned int meter[7] = {0};
unsigned int all[12] = {0};
unsigned char *cvalue = NULL;
unsigned long value;

if(argc < 2) {
	printf("ERROR : Not sufficient arguments\n");
	usage();
}

while((opt = getopt(argc, argv, "hs:m:c:a:r:")) != -1)
{
	switch(opt)
	{
	case 's':
		i = 0;
		for (index = optind - 1; index < argc; index++) {
			if(argv[index][0] == '-')
				break;
			if (index == 2) {
				if (!strcmp(argv[index], "swp1")) {
					st_fltr[i++] = 1;
					strcpy(port_s, argv[index]);
				} else if (!strcmp(argv[index], "swp2")) {
					st_fltr[i++] = 2;
					strcpy(port_s, argv[index]);
				} else {
					printf("ERROR : port name is invalid\n");
					usage();
				}
			} else {
				st_fltr[i++] = atoi(argv[index]);
			}
		}
		prog_strm_fltr = i;
	break;
	case 'm':
		i = 0;
		for (index = optind - 1; index < argc; index++) {
			if(argv[index][0] == '-')
				break;
			meter[i++] = strtoul(argv[index], NULL, 10);
		}
		prog_meter_mem = i;
	break;
	case 'a':
		i = 0;
		for (index = optind - 1; index < argc; index++) {
			if(argv[index][0] == '-')
				break;
			 if (index == 2) {
                                if (!strcmp(argv[index], "swp1")) {
                                        all[i++] = 1;
                                } else if (!strcmp(argv[index], "swp2")) {
                                        all[i++] = 2;
                                } else {
                                        printf("ERROR : port name is invalid\n");
                                        usage();
                                }
                        } else {
				all[i++] = strtoul(argv[index], NULL, 10);
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

if (prog_strm_fltr) {
	if(prog_strm_fltr < ST_ARG_NUM || (prog_strm_fltr > ST_ARG_NUM + 3)) {
		printf("ERROR: Wrong number of arguments for"	\
			" programming stream filter memory !\n");
		usage();
	}
	ret = !config_stream_filter(st_fltr[0], st_fltr[1], st_fltr[2],
				st_fltr[3], st_fltr[4], st_fltr[5]);
	if (ret)
		printf("Gate ID: %d Config Stream Filter Memory Success !!\n", st_fltr[2]);
	else
		printf("Gate ID: %d Config Stream Filter Memory Failed !!\n", st_fltr[2]);
}

if (prog_meter_mem) {
	if(prog_meter_mem != MTR_ARG_NUM) {
		printf("ERROR: Wrong number of arguments for "	\
				"programming meter memory !\n");
		usage();
	}
	ret = !program_meter_reg(meter[0], meter[1], meter[2], meter[3],
						meter[4], meter[5], meter[6]);
	if (ret)
		printf("Meter ID: %d Config Meter Memory Success !!\n", meter[5]);
	else
		printf("Meter ID: %d Config Meter Memory Failed !!\n", meter[5]);
}

if (prog_all) {
	if(prog_all < ALL_ARG_NUM) {
		printf("ERROR: Wrong number of arguments for "	\
				"programming all memory !\n");
		usage();
	}
	data.stream_conf_data.in_pid = all[0];
	data.stream_conf_data.max_fr_size = all[1];
	data.meter_conf_data.cir = all[2];
	data.meter_conf_data.eir = all[3];
	data.meter_conf_data.cbr = all[4];
	data.meter_conf_data.ebr = all[5];
	data.meter_conf_data.mode = all[6];
	data.psfp_conf_data.gate_id = all[7];
	data.psfp_conf_data.meter_id = all[8];
	data.psfp_conf_data.en_meter = all[9];
	data.psfp_conf_data.allow_stream = all[10];
	data.psfp_conf_data.en_psfp = all[11];
	ret = !config_all(data);
	if (ret)
		printf("Config All Success !!\n");
	else
		printf("Config All Failed !!\n");
}

#if 0
if(en_psfp_option)
{
	if(en_psfp_option != PSFP_ARG_NUM) {
		printf("ERROR: Wrong number of arguments for "	\
				"programming psfp options !\n");
		usage();
	}

	data.psfp_conf_data.en_meter = psfp[0];
	data.psfp_conf_data.allow_stream = psfp[1];
	data.psfp_conf_data.en_psfp = psfp[2];
	data.psfp_conf_data.gate_id = psfp[3];
	data.psfp_conf_data.wr_op_type = WR_EN_PSFP_MASK;

	ret = !psfp_control(&data);
	if (ret)
		printf("Enabling PSFP options Success !!\n");
	else
		printf("Enabling PSFP options Failed!!\n");
}
#endif

if (cvalue) {
	if (id == -1) {
		printf ("Required id value for reading\n");
		usage();
	}
	if(!strcmp(cvalue, "stream")) {
		printf("Reading stream filter memory\n");
		get_stream_filter_config(&data, id);
		printf("Ingress port : %d\n", data.stream_conf_data.in_pid);
		printf("Max Frame Size : %d\n", data.stream_conf_data.max_fr_size);
	}
	else if(!strcmp(cvalue, "meter")) {
		printf("Reading Meter memory\n");
		get_meter_reg(&data, id);
		printf("CIR : %lu\n", data.meter_conf_data.cir);
		printf("EIR : %lu\n", data.meter_conf_data.eir);
		printf("CBR : %lu\n", data.meter_conf_data.cbr);
		printf("EBR : %lu\n", data.meter_conf_data.ebr);
		printf("Mode : %lu\n", data.meter_conf_data.mode);
	}
	else if(!strcmp(cvalue, "counter")) {
		printf("Reading Counters value of gate-id: %d\n", id);
		get_psfp_counter(&data, id);
		value = (data.psfp_counter_data.psfp_fr_count.msb << 16 | data.psfp_counter_data.psfp_fr_count.lsb) ;
		printf("Frame Count : %lu\n", value);
		value = (data.psfp_counter_data.err_filter_ins_port.msb << 16 | data.psfp_counter_data.err_filter_ins_port.lsb);
		printf("Filter Ingress port error : %lu\n", value);
		value = (data.psfp_counter_data.err_filtr_sdu.msb << 16 | data.psfp_counter_data.err_filtr_sdu.lsb);
		printf("Filter SDU error : %lu\n", value);
		value = (data.psfp_counter_data.err_meter.msb << 16 | data.psfp_counter_data.err_meter.lsb);
		printf("Meter Error : %lu\n", value);
	}
	else {
		printf("Wrong argument value\n");
		usage();
	}
}
return 0;

}
