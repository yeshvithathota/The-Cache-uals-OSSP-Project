CC = gcc
CFLAGS = -Wall -Wextra

all:
	$(CC) $(CFLAGS) alarm_clock.c -o alarm_clock

run:
	./alarm_clock

trace:
	strace ./alarm_clock 2> strace.txt

clean:
	rm -f alarm_clock strace.txt output.txt
