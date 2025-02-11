CC=gcc

CFLAGS = -Wall -Werror -g -Wno-missing-braces
INCLUDES += -I .
INCLUDES += -I include/
INCLUDES += -I /lib/modules/$(shell uname -r)/build/include/

SRCS += $(wildcard src/*.c)
HSRCS += $(wildcard include/*.h)
OBJS = $(addsuffix .o, $(basename $(SRCS)))

TARGET=bin/pcie_test.exe
DEFINES = -DPROGNAME=\"$(notdir $(TARGET))\"
.PHONY: clean

$(TARGET): $(OBJS)
	@if [ ! -d "./bin" ]; then mkdir ./bin; fi
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c* $(HSRCS)
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c -o $@ $<

clean:
	$(RM) -rf **/*.o **/*.exe
