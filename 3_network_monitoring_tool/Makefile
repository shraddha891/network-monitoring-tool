CC     = gcc
CFLAGS = -Wall -Wextra -g
TARGET = netmon
SRCS   = src/netmon.c src/main.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)

.PHONY: all clean
