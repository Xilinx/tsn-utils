#SPDX-License-Identifier: GPL-2.0
APP = ptptime_date

# Add any other object files to this list below
APP_OBJS = ptptime_date.o

all: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

clean:
	-rm -f $(APP) *.elf *.gdb *.o


