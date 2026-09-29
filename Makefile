CC = gcc
CFLAGS = -Wall -Wextra -std=c11

all: alarm_clock

alarm_clock: alarm_clock.c
	$(CC) $(CFLAGS) alarm_clock.c -o alarm_clock

run: alarm_clock
	./alarm_clock

trace: alarm_clock
	strace -f -o strace.txt ./alarm_clock

clean:
	rm -f alarm_clock strace.txt
