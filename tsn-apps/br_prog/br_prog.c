/***************************************************************
* Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
* Copyright (C) 2022 - 2023 Advanced Micro Devices, Inc. All rights reserved.
* SPDX-License-Identifier: MIT
***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/in.h>
#include <syscall.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

typedef  unsigned long long uint64_t;
typedef  unsigned int uint32_t;
typedef  unsigned short uint16_t;
typedef  unsigned char uint8_t;

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

struct cnt_64 {
	unsigned int msb;
	unsigned int lsb;
};
typedef union static_cntr {
	struct cnt_64 word;
	uint64_t cnt;
}counter;

struct mac_merge_counters {
	counter tx_hold_cnt;
	counter tx_frag_cnt;
	counter rx_assembly_ok_cnt;
	counter rx_assembly_err_cnt;
	counter rx_smd_err_cnt;
	counter rx_frag_cnt;
};

struct statistics_counters {
	counter rx_bytes_cnt;
	counter tx_bytes_cnt;
	counter undersize_frames_cnt;
	counter frag_frames_cnt;
	counter rx_64_bytes_frames_cnt;
	counter rx_65_127_bytes_frames_cnt;
	counter rx_128_255_bytes_frames_cnt;
	counter rx_256_511_bytes_frames_cnt;
	counter rx_512_1023_bytes_frames_cnt;
	counter rx_1024_max_frames_cnt;
	counter rx_oversize_frames_cnt;
	counter tx_64_bytes_frames_cnt;
	counter tx_65_127_bytes_frames_cnt;
	counter tx_128_255_bytes_frames_cnt;
	counter tx_256_511_bytes_frames_cnt;
	counter tx_512_1023_bytes_frames_cnt;
	counter tx_1024_max_frames_cnt;
	counter tx_oversize_frames_cnt;
	counter rx_good_frames_cnt;
	counter rx_fcs_err_cnt;
	counter rx_good_broadcast_frames_cnt;
	counter rx_good_multicast_frames_cnt;
	counter rx_good_control_frames_cnt;
	counter rx_out_of_range_err_cnt;
	counter rx_good_vlan_frames_cnt;
	counter rx_good_pause_frames_cnt;
	counter rx_bad_opcode_frames_cnt;
	counter tx_good_frames_cnt;
	counter tx_good_broadcast_frames_cnt;
	counter tx_good_multicast_frames_cnt;
	counter tx_underrun_err_cnt;
	counter tx_good_control_frames_cnt;
	counter tx_good_vlan_frames_cnt;
	counter tx_good_pause_frames_cnt;
};

struct pmac_counters {
	struct statistics_counters sts;
	struct mac_merge_counters merge;
};

struct emac_pmac_stats {
	uint8_t preemp_en;
	struct statistics_counters emac;
	struct pmac_counters pmac;
};

struct preempt_ctrl_sts {
        uint8_t tx_preemp_sts;
        uint8_t mac_tx_verify_sts;
        uint8_t verify_timer_value;
        uint8_t additional_frag_size;
        uint8_t disable_preemp_verify;
};

struct preempt_status {
	uint8_t preemp_en;
	struct preempt_ctrl_sts ctrl;
};

struct qbu_user_override {
        uint8_t enable_value:1;
        uint16_t user_hold_time:9;
        uint8_t user_rel_time:6;
        uint8_t guard_band:1;
        uint8_t hold_rel_window:1;
        uint8_t hold_time_override:1;
        uint8_t rel_time_override:1;
}__attribute__((packed));

struct qbu_user {
        struct qbu_user_override user;
        uint8_t set;
};

#define QBU_WINDOW BIT(0)
#define QBU_GUARD_BAND BIT(1)
#define QBU_HOLD_TIME BIT(2)
#define QBU_REL_TIME BIT(3)

struct qbu_core_status {
        uint16_t hold_time;
        uint8_t rel_time;
        uint8_t hold_rel_en:1;
        uint8_t pmac_hold_req:1;
}__attribute__((packed));

void pmac_merge_stats(struct mac_merge_counters *st)
{
	printf("TX MAC Merge Hold Count\t\t\t: %llu\n", st->tx_hold_cnt);
	printf("TX MAC Merge Fragment Count\t\t: %llu\n", st->tx_frag_cnt);
	printf("RX MAC Merge Assembly OK Count\t\t: %llu\n", st->rx_assembly_ok_cnt);
	printf("RX MAC Merge Assembly Error Count\t: %llu\n", st->rx_assembly_err_cnt);
	printf("RX MAC Merge SMD Error Count\t\t: %llu\n", st->rx_smd_err_cnt);
	printf("RX MAC Merge Fragment Count\t\t: %llu\n", st->rx_frag_cnt);
	return;
}
#if 1
void preemp_stats(struct statistics_counters *st)
{
	printf("Rx Bytes Counter\t\t\t: %llu\n", st->rx_bytes_cnt);
	printf("Tx Bytes Counter\t\t\t: %llu\n", st->tx_bytes_cnt);
	printf("Undersize Frames Counter\t\t: %llu\n", st->undersize_frames_cnt);
	printf("Fragment Frames Counter\t\t\t: %llu\n", st->frag_frames_cnt);
	printf("Rx 64-Byte Frames Counter\t\t: %llu\n", st->rx_64_bytes_frames_cnt);
	printf("Rx 65-127-Byte Frames Counter\t\t: %llu\n", st->rx_65_127_bytes_frames_cnt);
	printf("Rx 128-255-Byte Frames Counter\t\t: %llu\n", st->rx_128_255_bytes_frames_cnt);
	printf("Rx 256-511-Byte Frames Counter\t\t: %llu\n", st->rx_256_511_bytes_frames_cnt);
	printf("Rx 512-1023-Byte Frames Counter\t\t: %llu\n", st->rx_512_1023_bytes_frames_cnt);
	printf("Rx 1024-Max Frames Counter\t\t: %llu\n", st->rx_1024_max_frames_cnt);
	printf("Rx Oversize Frames Counter\t\t: %llu\n", st->rx_oversize_frames_cnt);
	printf("Tx 64-Byte Frames Counter\t\t: %llu\n", st->tx_64_bytes_frames_cnt);
	printf("Tx 65-127-Byte Frames Counter\t\t: %llu\n", st->tx_65_127_bytes_frames_cnt);
	printf("Tx 128-255-Byte Frames Counter\t\t: %llu\n", st->tx_128_255_bytes_frames_cnt);
	printf("Tx 256-511-Byte Frames Counter\t\t: %llu\n", st->tx_256_511_bytes_frames_cnt);
	printf("Tx 512-1023-Byte Frames Counter\t\t: %llu\n", st->tx_512_1023_bytes_frames_cnt);
	printf("Tx 1024-Max Frames Counter\t\t: %llu\n", st->tx_1024_max_frames_cnt);
	printf("Tx Oversize Frames Counter\t\t: %llu\n", st->tx_oversize_frames_cnt);
	printf("Rx Good Frames Counter\t\t\t: %llu\n", st->rx_good_frames_cnt);
	printf("Rx FCS Error Counter\t\t\t: %llu\n", st->rx_fcs_err_cnt);
	printf("Rx Good Broadcast Frames Counter\t: %llu\n", st->rx_good_broadcast_frames_cnt);
	printf("Rx Good Multicast Frames Counter\t: %llu\n", st->rx_good_multicast_frames_cnt);
	printf("Rx Good Control Frames Counter\t\t: %llu\n", st->rx_good_control_frames_cnt);
	printf("Rx OUT OF Range Error Counter\t\t: %llu\n", st->rx_out_of_range_err_cnt);
	printf("Rx Good VLAN Frames Counter\t\t: %llu\n", st->rx_good_vlan_frames_cnt);
	printf("Rx Good Pause Frames Counter\t\t: %llu\n", st->rx_good_pause_frames_cnt);
	printf("Rx Bad Opcode Frame Counter\t\t: %llu\n", st->rx_bad_opcode_frames_cnt);
	printf("Tx Good Frames Counter\t\t\t: %llu\n", st->tx_good_frames_cnt);
	printf("Tx Good Broadcast Frames Counter\t: %llu\n", st->tx_good_broadcast_frames_cnt);
	printf("Tx Good Multicast Frames Counter\t: %llu\n", st->tx_good_multicast_frames_cnt);
	printf("Tx Underrun Error Counter\t\t: %llu\n", st->tx_underrun_err_cnt);
	printf("Tx Good Control Frames Counter\t\t: %llu\n", st->tx_good_control_frames_cnt);
	printf("Tx Good VLAN Frames Counter\t\t: %llu\n", st->tx_good_vlan_frames_cnt);
	printf("Tx Good Pause Frames Counter\t\t: %llu\n", st->tx_good_pause_frames_cnt);
	return;
}
#endif
void preemp_status(struct preempt_status *sts)
{
	printf("Preemption Status: %s\n", sts->preemp_en ? "Enable" : "Disable");
	printf("TX Preemption Status: %s\n", sts->ctrl.tx_preemp_sts ? "Active" : "Inactive");
	switch (sts->ctrl.mac_tx_verify_sts){
		case 0:
			printf("MAC Merge TX Verify Status: Initial\n");
			break;
		case 1:
			printf("MAC Merge TX Verify Status: Verifying\n");
			break;
		case 2:
			printf("MAC Merge TX Verify Status: Succeeded\n");
			break;
		case 3:
			printf("MAC Merge TX Verify Status: Failed\n");
			break;
		case 4:
			printf("MAC Merge TX Verify Status: Verification disabled\n");
			break;
	}
	printf("Verify Timer Value: %d\n", sts->ctrl.verify_timer_value);
	switch (sts->ctrl.additional_frag_size){
		case 0:
			printf("Additional Fragment Size: 64Bytes\n");
			break;
		case 1:
			printf("Additional Fragment Size: 128Bytes\n");
			break;
		case 2:
			printf("Additional Fragment Size: 192Bytes\n");
			break;
		case 3:
			printf("Additional Fragment Size: 256Bytes\n");
			break;
	}
	printf("Disable Preemption Verification: %d\n", sts->ctrl.disable_preemp_verify);
	return;
}

void usage()
{
	printf("Usage:\n br_prog <interface> <frame_preemption> <verify_timer_value> <additional_frag_size> <disable_preemp_verify>\n");
	printf("tsn interface name\n");
	printf("frame_preemption : [enable|disable|status|stats]\n");
	printf("Optional parameters:\n");
	printf("\tverify_timer_value : [0 to 127]\n");
	printf("\tadditional_frag_size : [64|128|192|256]\n");
	printf("\tdisable_preemp_verify : [1|0]\n");
}

int main(int argc, char **argv)
{
	char ifname[IFNAMSIZ];
	uint8_t	preemp = 0;
	struct preempt_status sts;
	struct preempt_ctrl_sts ctrl;
	struct emac_pmac_stats stats;
	int fd;
	struct ifreq s;
	uint16_t frag_size = 0;

	if((argc != 3) && (argc != 6)) {
		usage();
		return -1;
	}

	strncpy(ifname, argv[1], IFNAMSIZ);

	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	strcpy(s.ifr_name, ifname);
	
	printf("Interface: %s\n", ifname);
	if (!strcmp(argv[2], "enable")) {
		preemp = 1;
		s.ifr_data = (void *)&preemp;
		if (ioctl(fd, SIOC_PREEMPTION_CFG, &s) < 0) {
			printf("br prog failed\n");
		} else {
			printf("Interface : %s Preemption is %sd successfully\n", argv[1], argv[2]);
		}
	} else if (!strcmp(argv[2], "disable")) {
		preemp = 0;
		s.ifr_data = (void *)&preemp;
		if (ioctl(fd, SIOC_PREEMPTION_CFG, &s) < 0) {
			printf("br prog failed\n");
		}
	} else if (!strcmp(argv[2], "status")) {
		s.ifr_data = (void *)&sts;
		if (ioctl(fd, SIOC_PREEMPTION_STS, &s) < 0) {
			printf("br get status failed\n");
			goto out;
		}
		preemp_status(&sts);
		goto out;
	} else if (!strcmp(argv[2], "stats")) {
		s.ifr_data = (void *)&stats;
		if (ioctl(fd, SIOC_PREEMPTION_COUNTER, &s) < 0) {
			printf("br get statistics failed\n");
			goto out;
		}
		printf("\t\t-----------------------\n");
		printf("\t\tStatistics Counter EMAC\n");
		printf("\t\t-----------------------\n");
		preemp_stats(&(stats.emac));
		if (stats.preemp_en) {
			printf("\t\t-----------------------\n");
			printf("\t\tStatistics Counter PMAC\n");
			printf("\t\t-----------------------\n");
			preemp_stats(&(stats.pmac.sts));
			printf("\t\t-----------------------\n");
			printf("\t\tPMAC MAC Merge Counters\n");
			printf("\t\t-----------------------\n");
			pmac_merge_stats(&(stats.pmac.merge));
		} else {
			printf("Preemption is not enabled\n");
		}

		goto out;

	} else {
		usage();
		goto out;
	}

	if (argc == 6) {
		ctrl.verify_timer_value = atoi(argv[3]);
		frag_size = atoi(argv[4]);
		ctrl.disable_preemp_verify = atoi(argv[5]);
		if ((ctrl.verify_timer_value < 0) || (ctrl.verify_timer_value > 127)) {
			usage();
			printf("verify_timer_value: %d\n", ctrl.verify_timer_value);
			goto out;
		}
		if ((frag_size != 64) && (frag_size != 128) && (frag_size != 192) && (frag_size != 256)) {
			usage();
			printf("additional_frag_size: %d\n", frag_size);
			goto out;
		} else {
			switch (frag_size){
				case 64:
					ctrl.additional_frag_size = 0;
					break;
				case 128:
					ctrl.additional_frag_size = 1;
					break;
				case 192:
					ctrl.additional_frag_size = 2;
					break;
				case 256:
					ctrl.additional_frag_size = 3;
					break;
			}
		}
		if ((ctrl.disable_preemp_verify != 0) && (ctrl.disable_preemp_verify != 1)) {
			usage();
			printf("disable_preemp_verify: %d\n", ctrl.disable_preemp_verify);
			goto out;
		}
		s.ifr_data = (void *)&ctrl;
		if (ioctl(fd, SIOC_PREEMPTION_CTRL, &s) < 0) {
			printf("br set control failed\n");
		} else {
			printf("verify_timer_value: %d\n", ctrl.verify_timer_value);
			printf("additional_frag_size: %d\n", frag_size);
			printf("disable_preemp_verify: %d\n", ctrl.disable_preemp_verify);
			printf("Configured Successfully\n");
		}
	}
out:
	close(fd);
	return 0;
}	
