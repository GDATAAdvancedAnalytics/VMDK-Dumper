CC?=gcc
CFLAGS=-Wall -Wextra -Werror -O2
LIBS=-lz

SRCS=vmdk_dump.c compression_helper.c
EXECUTABLE?=vmdk_dump

.PHONY: all clean

all: $(EXECUTABLE)

$(EXECUTABLE): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(EXECUTABLE)
