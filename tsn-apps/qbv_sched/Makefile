#SPDX-License-Identifier: GPL-2.0
APP = qbv_sched

# Add any other object files to this list below
APP_OBJS = qbv_sched.o

all: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

clean:
	-rm -f $(APP) *.elf *.gdb *.o


