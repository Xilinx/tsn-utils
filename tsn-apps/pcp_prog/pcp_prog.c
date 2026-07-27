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
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>


#define SET_PMAP_CONFIG   0x3D
struct pmap_data {
	int st_pcp_reg;
	int res_pcp_reg;
};
/* Resolved at runtime by resolve_param(): the TSN module name differs by
 * kernel (xilinx_tsn_ep pre xilinx-v2025.1, xilinx_emac_tsn from v2025.1). */
char *st_pcp_fname;
char *res_pcp_fname;
char *config_fname = "/etc/xilinx-tsn/pcp.cfg";
int set_pcp(int argc, char *argv[]);
int get_pcp();
void usage();

/* Return the sysfs path for a driver parameter, auto-detecting the TSN module
 * name across kernel versions. Returns a malloc'd string, or NULL if the
 * parameter is not present under any known module name. */
char *resolve_param(const char *param)
{
	const char *mods[] = { "xilinx_emac_tsn", "xilinx_tsn_ep", NULL };
	char path[128];
	int i;

	for (i = 0; mods[i]; i++) {
		snprintf(path, sizeof(path),
			 "/sys/module/%s/parameters/%s", mods[i], param);
		if (access(path, F_OK) == 0)
			return strdup(path);
	}
	return NULL;
}
int set_pcp(int argc, char *argv[]){
	
	int pcp;
	int st_pcp_reg = 0;
	int res_pcp_reg = 0;
	int i = 0;
	int len = 0;
	int first = 0;
	struct pmap_data pri_info;
	char st_pcp_str[20];
	char res_pcp_str[20];
	char st_temp[30];
	char res_temp[30];
	FILE *fp_st;
	FILE *fp_res;
	FILE *fp_cfg;
	FILE *fd;

	st_pcp_str[0] = '\0';
	res_pcp_str[0] = '\0';
	fp_cfg = fopen(config_fname,"r");
	if (!fp_cfg) {
		printf("file open failed : %s\n",config_fname);
		return -1;
	}
	fgets(st_temp,30,fp_cfg);
	len = strlen(st_temp);
	for (i = 0; i < len; i++) {
		if (!isdigit(st_temp[i]))
			continue;
		pcp = atoi(&st_temp[i]);
		if (pcp > 7) {
			printf("Invalid pcp value: %d\n",pcp);
			usage();
		}
		st_pcp_reg |= 0x1 << pcp;
		if (first != 0) {
			strncat(st_pcp_str,",",1);
		}
		strncat(st_pcp_str,&st_temp[i],1);
		first = 1;
	}
	fgets(res_temp,30,fp_cfg);
        len = strlen(res_temp);
	first = 0;
	for(i=0;i < len; i++){
		if (!isdigit(res_temp[i]))
			continue;
		pcp = atoi(&res_temp[i]);
		if (pcp > 7){
			printf("Invalid pcp value: %d\n",pcp);
			usage();
		}
                res_pcp_reg |= 0x1 << pcp;
		if (first != 0)
                        strncat(res_pcp_str,",",1);
                strncat(res_pcp_str,&res_temp[i],1);
		first = 1;
        }
	fclose(fp_cfg);
	if ((st_pcp_reg & res_pcp_reg & 0xFF) != 0){
		printf("ST and RES can't be mapped to the same pcp value\n");
		return -1;
	}
	pri_info.st_pcp_reg = st_pcp_reg;
	pri_info.res_pcp_reg = res_pcp_reg;
	fp_st = fopen(st_pcp_fname,"w");
	if (!fp_st) {
		printf("file open failed : %s\n",st_pcp_fname);
		return -1;
	}
	fp_res = fopen(res_pcp_fname,"w");
	if (!fp_res) {
                printf("file open failed : %s\n",res_pcp_fname);
                return -1;
        }
	if ( access( "/dev/switch", F_OK ) == 0 ) {
		fd = open("/dev/switch", O_RDWR);       /* Driver open */
		if (fd == -1) {
			printf("open error...\n");
			return -1;
		}
		if (0 > ioctl(fd, SET_PMAP_CONFIG, &pri_info)) {
			printf("\nswitch set pmap config failed\n");
			return -1;
		}
	}
	fprintf(fp_st,"%s",st_pcp_str);
	fprintf(fp_res,"%s",res_pcp_str);
	fclose(fp_st);
	fclose(fp_res);
	get_pcp();
	return 0;
}

int get_pcp(){
	FILE *fp;
	char st_str[20];
	char res_str[20];
	fp = fopen(st_pcp_fname,"r");
	if (!fp) {
                printf("file open failed : %s\n",st_pcp_fname);
                return -1;
        }
	fgets(st_str,20,fp);
	fclose(fp);
	fp = fopen(res_pcp_fname,"r");
        if (!fp) {
                printf("file open failed : %s\n",res_pcp_fname);
                return -1;
        }
        fgets(res_str,20,fp);
	fclose(fp);
        printf("Current configuration:\n");
        printf("PCP mapped to ST:%s\n",st_str);
	printf("PCP mapped to RES:%s\n",res_str);
	return 0;
}

void usage(){
	printf("/******************************************/\n");
        printf("Usage: pcp_prog [-sg]\n\n");
        printf("To set new configuration as specified in /etc/xilinx-tsn/pcp.cfg:\n");
        printf("pcp_prog -s\n");
        printf("For reading current configuration:\n");
        printf("pcp_prog -g\n");
	printf("HELP:\n");
        printf("pcp_prog -h\n");
        printf("/******************************************/\n");
        exit(1);
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;

	if (argc < 2) {
		usage();
		goto out;
	}

	st_pcp_fname = resolve_param("st_pcp");
	res_pcp_fname = resolve_param("res_pcp");
	if (!st_pcp_fname || !res_pcp_fname) {
		fprintf(stderr,
			"TSN driver st_pcp/res_pcp parameters not found under "
			"/sys/module/{xilinx_emac_tsn,xilinx_tsn_ep}/parameters; "
			"is the TSN driver loaded?\n");
		goto out;
	}

	if ((!strcmp(argv[1], "-s")) || (!strcmp(argv[1], "--set"))) {
		ret = set_pcp(argc, argv);
	} else if ((!strcmp(argv[1], "-g")) || (!strcmp(argv[1], "--get"))) {
		ret = get_pcp();
	} else {
		usage();
		goto out;
	}

	ret = (ret == 0) ? EXIT_SUCCESS : EXIT_FAILURE;

out:
	free(st_pcp_fname);
	free(res_pcp_fname);
	return ret;
}
