#Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
#SPDX-License-Identifier: MIT

INSTALL_PROGRAM ?= install

bindir ?= /usr/bin

APP = qci_prog

# Add any other object files to this list below
APP_OBJS = qci_prog.o

.PHONY: all install clean

all: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

install: $(APP)
	$(INSTALL_PROGRAM) -d $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) -m 0755 $(APP) $(DESTDIR)$(bindir)/$(APP)

clean:
	-rm -f $(APP) *.elf *.gdb *.o


