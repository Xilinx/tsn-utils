#Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
#SPDX-License-Identifier: MIT

.PHONY: all
all:
	$(MAKE) -C tsn-apps/br_prog all
	$(MAKE) -C tsn-apps/cb_prog all
	$(MAKE) -C tsn-apps/ipic_prog all
	$(MAKE) -C tsn-apps/pcp_prog all
	$(MAKE) -C tsn-apps/ptptime all
	$(MAKE) -C tsn-apps/ptptime_date all
	$(MAKE) -C tsn-apps/qbu_prog all
	$(MAKE) -C tsn-apps/qbv_sched all
	$(MAKE) -C tsn-apps/qci_prog all
	$(MAKE) -C tsn-apps/switch_cam all
	$(MAKE) -C tsn-apps/switch_cam2 all
	$(MAKE) -C tsn-apps/switch_prog all
	$(MAKE) -C tsn-apps/tadma_prog all

.PHONY: clean
clean:
	$(MAKE) -C tsn-apps/br_prog clean
	$(MAKE) -C tsn-apps/cb_prog clean
	$(MAKE) -C tsn-apps/ipic_prog clean
	$(MAKE) -C tsn-apps/pcp_prog clean
	$(MAKE) -C tsn-apps/ptptime clean
	$(MAKE) -C tsn-apps/ptptime_date clean
	$(MAKE) -C tsn-apps/qbu_prog clean
	$(MAKE) -C tsn-apps/qbv_sched clean
	$(MAKE) -C tsn-apps/qci_prog clean
	$(MAKE) -C tsn-apps/switch_cam clean
	$(MAKE) -C tsn-apps/switch_cam2 clean
	$(MAKE) -C tsn-apps/switch_prog clean
	$(MAKE) -C tsn-apps/tadma_prog clean
