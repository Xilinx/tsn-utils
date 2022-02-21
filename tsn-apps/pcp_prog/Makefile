#SPDX-License-Identifier: GPL-2.0
APP = pcp_prog

# Add any other object files to this list below
APP_OBJS = pcp_prog.o

all: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

clean:
	-rm -f $(APP) *.elf *.gdb *.o


