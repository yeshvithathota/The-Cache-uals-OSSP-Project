# Tempo — Linux Alarm Clock System

Tempo pairs a small browser alarm dashboard with a Linux systems-programming project. The C program demonstrates one child process per alarm, Linux signals, efficient process suspension, cancellation, and parent-child synchronization.

## Browser dashboard

Open `index.html` in a modern browser. Alarms are saved in that browser's local storage and run while the page is open. When an alarm rings, its sound continues until you cancel or snooze it. Snooze silences the current ring and rings again after five minutes. Browser notifications appear when permission is granted. This front end is a browser demo; it does not communicate with the C process manager.

For a local server, run:

```bash
python3 -m http.server 8000
```

Then visit `http://localhost:8000`.

## Linux C program

Build and run on Linux with GCC:

```bash
make
./alarm_clock
```

Trace its system calls with `strace` installed:

```bash
make trace
```

Clean generated files with `make clean`.

## OS concepts shown

- `fork()` creates a child process for every alarm.
- `alarm()` asks the kernel to deliver `SIGALRM` after the requested delay.
- `sigaction()` installs handlers for alarm and cancellation signals.
- `sigprocmask()` and `sigsuspend()` make the wait race-free: signals are blocked while checking state, then atomically unblocked while the child sleeps.
- `waitpid()` collects child processes and avoids zombies.
- `kill()` sends `SIGTERM` to cancel alarms.
