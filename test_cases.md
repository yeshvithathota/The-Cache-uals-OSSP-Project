# Alarm Clock System — Manual Checks

| ID | Check | Expected result |
|---|---|---|
| 1 | Build with `make` on Linux | C program compiles without warnings |
| 2 | Add a positive delay | A child process is created and reports its PID |
| 3 | Wait for an alarm | Child wakes on `SIGALRM` and exits |
| 4 | Add multiple alarms | Each alarm has an independent child process |
| 5 | List active alarms | Parent lists only alarms that have not exited |
| 6 | Cancel an alarm by PID | Parent sends `SIGTERM` and reaps the child |
| 7 | Enter malformed menu input | Input is rejected without getting stuck |
| 8 | Exit with active alarms | Children are cancelled and collected |
| 9 | Create an alarm in the browser | Alarm appears and remains after page reload |
| 10 | Keep browser page open through alarm time | Browser alarm alert and tone are triggered |
| 11 | Run `make trace` | A fresh `strace-run.txt` is generated |
