# Alarm Clock System - Test Cases and Results

| Test ID | Test Case | Input / Action | Expected Result | Status |
|---|---|---|---|---|
| TC01 | Compile program | `make` | Program compiles successfully | PASS |
| TC02 | Add alarm | Choice 1, delay 5 | Child process created | PASS |
| TC03 | Alarm notification | Wait for timer | ALARM RINGING displayed | PASS |
| TC04 | Multiple alarms | Add 10 sec and 5 sec alarms | Multiple child processes created | PASS |
| TC05 | List alarms | Choice 3 | Active PIDs and delays displayed | PASS |
| TC06 | Cancel alarm | Choice 2 + child PID | Alarm cancelled | PASS |
| TC07 | Invalid delay | Enter 0 or negative value | Invalid delay rejected | PASS |
| TC08 | Invalid menu | Enter 9 | Invalid choice displayed | PASS |
| TC09 | Exit | Choice 4 | All child processes terminated | PASS |
| TC10 | System-call tracing | `make trace` | `strace.txt` generated | PASS |
