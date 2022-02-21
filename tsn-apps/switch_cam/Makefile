#SPDX-License-Identifier: GPL-2.0
APP = switch_cam

# Add any other object files to this list below
APP_OBJS = switch_cam.o

all: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

clean:
	-rm -f $(APP) *.elf *.gdb *.o


