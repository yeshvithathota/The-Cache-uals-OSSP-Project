#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_ALARMS 10

typedef struct {
    pid_t pid;
    int delay;
    int active;
} Alarm;

static volatile sig_atomic_t alarm_triggered;
static volatile sig_atomic_t alarm_cancelled;

static void alarm_handler(int signal_number) {
    static const char message[] = "\n====================================\n          ALARM RINGING!\n====================================\n";
    (void)signal_number;
    alarm_triggered = 1;
    (void)write(STDOUT_FILENO, message, sizeof(message) - 1);
}

static void cancel_handler(int signal_number) {
    static const char message[] = "\n[Child] Alarm cancelled.\n";
    (void)signal_number;
    alarm_cancelled = 1;
    (void)write(STDOUT_FILENO, message, sizeof(message) - 1);
}

static int install_handler(int signal_number, void (*handler)(int)) {
    struct sigaction action = {0};
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    return sigaction(signal_number, &action, NULL);
}

static void alarm_process(int delay) {
    if (install_handler(SIGALRM, alarm_handler) == -1 ||
        install_handler(SIGTERM, cancel_handler) == -1) {
        perror("sigaction");
        _exit(EXIT_FAILURE);
    }

    sigset_t watched, previous_mask;
    sigemptyset(&watched);
    sigaddset(&watched, SIGALRM);
    sigaddset(&watched, SIGTERM);

    /* Block first so a signal arriving before the wait cannot be lost. */
    if (sigprocmask(SIG_BLOCK, &watched, &previous_mask) == -1) {
        perror("sigprocmask");
        _exit(EXIT_FAILURE);
    }

    printf("\n[Child Process]\nChild PID: %ld\nAlarm delay: %d second(s)\nChild process is suspended using sigsuspend()...\n",
           (long)getpid(), delay);
    fflush(stdout);
    alarm((unsigned int)delay);

    while (!alarm_triggered && !alarm_cancelled)
        (void)sigsuspend(&previous_mask);

    (void)sigprocmask(SIG_SETMASK, &previous_mask, NULL);
    puts(alarm_triggered ? "[Child] Alarm event handled successfully." :
                            "[Child] Alarm process terminated after cancellation.");
    fflush(stdout);
    _exit(EXIT_SUCCESS);
}

static void reap_finished_alarms(Alarm alarms[]) {
    for (int i = 0; i < MAX_ALARMS; ++i) {
        if (!alarms[i].active) continue;
        int status;
        pid_t result;
        do {
            result = waitpid(alarms[i].pid, &status, WNOHANG);
        } while (result == -1 && errno == EINTR);
        if (result == alarms[i].pid || (result == -1 && errno == ECHILD))
            alarms[i].active = 0;
    }
}

static int find_alarm(const Alarm alarms[], pid_t pid) {
    for (int i = 0; i < MAX_ALARMS; ++i)
        if (alarms[i].active && alarms[i].pid == pid) return i;
    return -1;
}

static int read_integer(const char *prompt, int *value) {
    char line[128], extra;
    fputs(prompt, stdout);
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) return 0;
    if (sscanf(line, " %d %c", value, &extra) != 1) return -1;
    return 1;
}

static void wait_for_child(pid_t pid) {
    while (waitpid(pid, NULL, 0) == -1) {
        if (errno != EINTR && errno != ECHILD) {
            perror("waitpid");
            break;
        }
        if (errno == ECHILD) break;
    }
}

int main(void) {
    Alarm alarms[MAX_ALARMS] = {0};
    puts("============================================\n         LINUX ALARM CLOCK SYSTEM\n============================================");
    puts("OS concepts: fork()  alarm()  SIGALRM  sigsuspend()  SIGTERM  waitpid()");

    for (;;) {
        reap_finished_alarms(alarms);
        puts("\n1. Add Alarm\n2. Cancel Alarm\n3. List Active Alarms\n4. Exit");
        int choice;
        int input = read_integer("Enter choice: ", &choice);
        if (input == 0) choice = 4;
        else if (input < 0) { puts("Invalid input. Enter a number from 1 to 4."); continue; }

        if (choice == 1) {
            int slot = -1;
            for (int i = 0; i < MAX_ALARMS; ++i)
                if (!alarms[i].active) { slot = i; break; }
            if (slot == -1) { puts("Maximum number of active alarms reached."); continue; }
            int delay;
            input = read_integer("Enter alarm delay in seconds: ", &delay);
            if (input <= 0 || delay <= 0) { puts("Invalid alarm delay. Enter a positive whole number."); continue; }
            pid_t pid = fork();
            if (pid == -1) { perror("fork"); continue; }
            if (pid == 0) alarm_process(delay);
            alarms[slot] = (Alarm){.pid = pid, .delay = delay, .active = 1};
            printf("Alarm created. Child PID: %ld\n", (long)pid);
        } else if (choice == 2) {
            int pid_value;
            input = read_integer("Enter Child PID to cancel: ", &pid_value);
            if (input <= 0 || pid_value <= 0) { puts("Invalid PID."); continue; }
            int index = find_alarm(alarms, (pid_t)pid_value);
            if (index == -1) { printf("No active alarm found for PID %d.\n", pid_value); continue; }
            if (kill(alarms[index].pid, SIGTERM) == -1 && errno != ESRCH) perror("kill");
            wait_for_child(alarms[index].pid);
            alarms[index].active = 0;
            printf("Alarm with PID %d cancelled.\n", pid_value);
        } else if (choice == 3) {
            int found = 0;
            puts("\nActive Alarms:");
            for (int i = 0; i < MAX_ALARMS; ++i) {
                if (alarms[i].active) {
                    printf("PID: %ld | Delay: %d second(s)\n", (long)alarms[i].pid, alarms[i].delay);
                    found = 1;
                }
            }
            if (!found) puts("No active alarms.");
        } else if (choice == 4) {
            puts("\nShutting down Alarm Clock System...");
            for (int i = 0; i < MAX_ALARMS; ++i)
                if (alarms[i].active && kill(alarms[i].pid, SIGTERM) == -1 && errno != ESRCH)
                    perror("kill");
            for (int i = 0; i < MAX_ALARMS; ++i)
                if (alarms[i].active) { wait_for_child(alarms[i].pid); alarms[i].active = 0; }
            puts("All alarm processes terminated. Exiting successfully.");
            break;
        } else puts("Invalid choice. Please enter 1-4.");
    }
    return EXIT_SUCCESS;
}
