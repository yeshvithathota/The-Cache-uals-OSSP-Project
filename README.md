# Alarm Clock System

## Objective
To implement a Linux-based alarm clock using C and POSIX system programming concepts.

## OS Concepts Used
- Process creation using fork()
- Signals
- SIGALRM
- Signal handling
- Process suspension using pause()
- Process termination using kill()
- Parent-child synchronization using waitpid()
- Linux system calls
- Process monitoring

## Compilation
gcc alarm_clock.c -o alarm_clock

## Execution
./alarm_clock

## System Call Tracing
strace ./alarm_clock 2> strace.txt

## Functionality
1. Accepts an alarm delay from the user.
2. Creates a child process to manage the alarm.
3. Uses alarm() to schedule the alarm.
4. Uses SIGALRM to generate the alarm event.
5. Suspends the child process using pause().
6. Displays an alarm notification.
7. Allows the parent to cancel the alarm.
8. Uses waitpid() to synchronize with the child.
