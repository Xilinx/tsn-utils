#SPDX-License-Identifier: GPL-2.0
APP = qci_prog

# Add any other object files to this list below
APP_OBJS = qci_prog.o

all: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

clean:
	-rm -f $(APP) *.elf *.gdb *.o


