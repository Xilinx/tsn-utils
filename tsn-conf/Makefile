#Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
#SPDX-License-Identifier: MIT

INSTALL_PROGRAM ?= install

bindir ?= /usr/bin
sysconfdir ?= /etc/xilinx-tsn

.PHONY: install

install:
	$(INSTALL_PROGRAM) -d $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) -m 0644 *.conf $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) -d $(DESTDIR)$(sysconfdir)
	$(INSTALL_PROGRAM) -m 0644 *.cfg $(DESTDIR)$(sysconfdir)
