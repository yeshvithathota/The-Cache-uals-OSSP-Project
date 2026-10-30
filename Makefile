CC ?= gcc
CFLAGS ?= -Wall -Wextra -Wpedantic -std=c11 -O2

.PHONY: all run trace clean

all: alarm_clock

alarm_clock: alarm_clock.c
	$(CC) $(CFLAGS) $< -o $@

run: alarm_clock
	./alarm_clock

trace: alarm_clock
	strace -f -o strace-run.txt ./alarm_clock

clean:
	rm -f alarm_clock strace-run.txt
