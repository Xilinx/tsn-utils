#Copyright (c) 2016-2022 Xilinx, Inc. All rights reserved.
#SPDX-License-Identifier: MIT
APP = tadma_prog

# Add any other object files to this list below
APP_OBJS = tadma_prog.o

all: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS) -lconfig

clean:
	-rm -f $(APP) *.elf *.gdb *.o


