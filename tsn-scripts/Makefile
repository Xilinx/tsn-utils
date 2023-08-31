#*****************************************************************************
#Copyright (c) 2022 Xilinx, Inc. All rights reserved.
#Copyright (C) 2022 - 2023 Advanced Micro Devices, Inc. All rights reserved.
#SPDX-License-Identifier: MIT
#*****************************************************************************

INSTALL_PROGRAM ?= install

bindir ?= /usr/bin

SCRIPT = net_setup.sh start_ptp.sh

.PHONY: install

install: 
	$(INSTALL_PROGRAM) -d $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) -m 0644 $(SCRIPT) $(DESTDIR)$(bindir)
