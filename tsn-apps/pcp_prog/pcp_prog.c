/* SPDX-License-Identifier: GPL-2.0 */
/*
 * TSN Configuration utility
 *
 * (C) Copyright 2021, Xilinx, Inc.
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
#include <ctype.h>


#define SET_PMAP_CONFIG   0x3D
struct pmap_data {
	int st_pcp_reg;
	int res_pcp_reg;
};
char *st_pcp_fname = "/sys/module/xilinx_tsn_ep/parameters/st_pcp";
char *res_pcp_fname = "/sys/module/xilinx_tsn_ep/parameters/res_pcp";
char *config_fname = "/etc/pcp.cfg";
int set_pcp(int argc, char *argv[]);
int get_pcp();
void usage();
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
        printf("To set new configuration as specified in /etc/pcp.cfg:\n");
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
        int ret = 0;

	if (argc < 2){
		printf("Insufficient arguments\n");
		usage();
	}

        if ((!strcmp(argv[1], "-s")) || (!strcmp(argv[1], "--set"))) {
                ret = set_pcp(argc, argv);
        } else if ((!strcmp(argv[1], "-g")) || (!strcmp(argv[1], "--get"))) {
                ret = get_pcp();
        } else if ((!strcmp(argv[1], "-h")) || (!strcmp(argv[1], "--help"))) {
                usage();
        } else {
		printf("Invalid arguments: %s\n", argv[1]);
		usage();
	}
        return ret;
}
