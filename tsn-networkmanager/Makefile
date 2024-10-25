#*****************************************************************************
#Copyright (c) 2022 Xilinx, Inc. All rights reserved.
#Copyright (C) 2022 - 2023 Advanced Micro Devices, Inc. All rights reserved.
#SPDX-License-Identifier: MIT
#*****************************************************************************

INSTALL_PROGRAM ?= install

bindir ?= /usr/bin/
DESTDIR_NETWORKMANAGER = $(bindir)/network-manager/
DESTDIR_PARSERS = $(DESTDIR_NETWORKMANAGER)/tsn-parsers
sysconfdir ?= /etc/
confdir = $(sysconfdir)/tsn-networkmanager/
DESTDIR_SAMPLES = $(confdir)/samples

SCRIPT = xcnc_setup fdb.py tftp_nm.py qbv.py
PARSERS = tsn-parsers/fdb_parser.py tsn-parsers/qbv-parser.py tsn-parsers/common.py tsn-parsers/xnm.sh tsn-parsers/board_config.sh tsn-parsers/mac_setup.sh
SAMPLES = samples/sample_qbv.json samples/sample_fdb.json samples/default_qbv.json


.PHONY: install

install: app parsers

app:
	$(INSTALL_PROGRAM) -d $(DESTDIR)$(DESTDIR_NETWORKMANAGER)
	$(INSTALL_PROGRAM) -d $(DESTDIR)$(DESTDIR_SAMPLES)
	$(INSTALL_PROGRAM) -m 0755 $(SCRIPT) $(DESTDIR)$(DESTDIR_NETWORKMANAGER)
	$(INSTALL_PROGRAM) -m 0644 $(SAMPLES) $(DESTDIR)$(DESTDIR_SAMPLES)

parsers:
	$(INSTALL_PROGRAM) -d $(DESTDIR)$(DESTDIR_PARSERS)
	$(INSTALL_PROGRAM) -m 0755 $(PARSERS) $(DESTDIR)$(DESTDIR_PARSERS)
