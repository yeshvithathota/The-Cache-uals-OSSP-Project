#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MAX_ALARMS 10

typedef struct
{
    pid_t pid;
    int delay;
    int active;
} Alarm;

volatile sig_atomic_t alarm_triggered = 0;
volatile sig_atomic_t alarm_cancelled = 0;

/* Handler for alarm signal */
void alarm_handler(int sig)
{
    const char message[] =
        "\n====================================\n"
        "          ALARM RINGING! 🔔\n"
        "====================================\n";

    (void)sig;

    alarm_triggered = 1;

    write(STDOUT_FILENO, message, sizeof(message) - 1);
}

/* Handler for cancellation signal */
void cancel_handler(int sig)
{
    const char message[] =
        "\n[Child] Alarm cancelled.\n";

    (void)sig;

    alarm_cancelled = 1;

    write(STDOUT_FILENO, message, sizeof(message) - 1);
}

/* Check for finished child processes */
void reap_finished_alarms(Alarm alarms[])
{
    int status;

    for (int i = 0; i < MAX_ALARMS; i++)
    {
        if (alarms[i].active)
        {
            pid_t result =
                waitpid(alarms[i].pid, &status, WNOHANG);

            if (result == alarms[i].pid)
            {
                alarms[i].active = 0;
            }
        }
    }
}

/* Find alarm using child PID */
int find_alarm(Alarm alarms[], pid_t pid)
{
    for (int i = 0; i < MAX_ALARMS; i++)
    {
        if (alarms[i].active &&
            alarms[i].pid == pid)
        {
            return i;
        }
    }

    return -1;
}

/* Code executed by every alarm child process */
void alarm_process(int delay)
{
    struct sigaction alarm_action = {0};
    struct sigaction cancel_action = {0};

    /* Register SIGALRM handler */
    alarm_action.sa_handler = alarm_handler;
    sigemptyset(&alarm_action.sa_mask);

    sigaction(SIGALRM, &alarm_action, NULL);

    /* Register SIGTERM handler */
    cancel_action.sa_handler = cancel_handler;
    sigemptyset(&cancel_action.sa_mask);

    sigaction(SIGTERM, &cancel_action, NULL);

    printf("\n[Child Process]\n");
    printf("Child PID: %d\n", getpid());
    printf("Alarm delay: %d second(s)\n", delay);
    printf("Child process is suspended using pause()...\n");

    fflush(stdout);

    /* Start timer */
    alarm((unsigned int)delay);

    /*
       The child sleeps here until a signal arrives.
       No continuous busy waiting is performed.
    */
    while (!alarm_triggered && !alarm_cancelled)
    {
        pause();
    }

    if (alarm_triggered)
    {
        printf("[Child] Alarm event handled successfully.\n");
    }
    else if (alarm_cancelled)
    {
        printf("[Child] Alarm process terminated after cancellation.\n");
    }

    fflush(stdout);

    _exit(0);
}

int main(void)
{
    Alarm alarms[MAX_ALARMS] = {0};

    int choice;

    printf("============================================\n");
    printf("         LINUX ALARM CLOCK SYSTEM\n");
    printf("============================================\n");

    printf("OS Concepts Used:\n");
    printf("fork()  alarm()  SIGALRM  pause()\n");
    printf("kill()  SIGTERM  waitpid()\n");

    while (1)
    {
        /* Reap completed alarm processes */
        reap_finished_alarms(alarms);

        printf("\n============================================\n");
        printf("                 MENU\n");
        printf("============================================\n");

        printf("1. Add Alarm\n");
        printf("2. Cancel Alarm\n");
        printf("3. List Active Alarms\n");
        printf("4. Exit\n");

        printf("============================================\n");
        printf("Enter choice: ");
        fflush(stdout);

        if (scanf("%d", &choice) != 1)
        {
            printf("Invalid input.\n");

            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF)
            {
            }

            continue;
        }

        /* =========================================
           ADD ALARM
           ========================================= */

        if (choice == 1)
        {
            int delay;
            int slot = -1;

            /* Find empty alarm slot */
            for (int i = 0; i < MAX_ALARMS; i++)
            {
                if (!alarms[i].active)
                {
                    slot = i;
                    break;
                }
            }

            if (slot == -1)
            {
                printf("Maximum number of active alarms reached.\n");
                continue;
            }

            printf("Enter alarm delay in seconds: ");
            fflush(stdout);

            if (scanf("%d", &delay) != 1 || delay <= 0)
            {
                printf("Invalid alarm delay.\n");

                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF)
                {
                }

                continue;
            }

            /*
               Create a child process to manage
               this alarm.
            */

            pid_t pid = fork();

            if (pid < 0)
            {
                perror("fork");
                continue;
            }

            /* Child process */
            if (pid == 0)
            {
                alarm_process(delay);
            }

            /* Parent process */
            else
            {
                alarms[slot].pid = pid;
                alarms[slot].delay = delay;
                alarms[slot].active = 1;

                printf("\n[Parent Process]\n");
                printf("Parent PID: %d\n", getpid());
                printf("Alarm child PID: %d\n", pid);
                printf("Alarm created successfully.\n");
            }
        }

        /* =========================================
           CANCEL ALARM
           ========================================= */

        else if (choice == 2)
        {
            int pid;

            printf("Enter Child PID to cancel: ");
            fflush(stdout);

            if (scanf("%d", &pid) != 1)
            {
                printf("Invalid PID.\n");
                continue;
            }

            int index =
                find_alarm(alarms, (pid_t)pid);

            if (index == -1)
            {
                printf("No active alarm found for PID %d.\n", pid);
            }
            else
            {
                if (kill(alarms[index].pid, SIGTERM) == -1)
                {
                    perror("kill");
                }
                else
                {
                    waitpid(alarms[index].pid, NULL, 0);

                    alarms[index].active = 0;

                    printf(
                        "Alarm with PID %d cancelled successfully.\n",
                        pid
                    );
                }
            }
        }

        /* =========================================
           LIST ACTIVE ALARMS
           ========================================= */

        else if (choice == 3)
        {
            int found = 0;

            printf("\nActive Alarms:\n");
            printf("--------------------------------------------\n");

            for (int i = 0; i < MAX_ALARMS; i++)
            {
                if (alarms[i].active)
                {
                    printf(
                        "PID: %d | Delay: %d second(s)\n",
                        alarms[i].pid,
                        alarms[i].delay
                    );

                    found = 1;
                }
            }

            if (!found)
            {
                printf("No active alarms.\n");
            }
        }

        /* =========================================
           EXIT
           ========================================= */

        else if (choice == 4)
        {
            printf("\nShutting down Alarm Clock System...\n");

            /*
               Cancel all remaining alarms.
            */

            for (int i = 0; i < MAX_ALARMS; i++)
            {
                if (alarms[i].active)
                {
                    kill(alarms[i].pid, SIGTERM);
                }
            }

            /*
               Wait for all child processes.
            */

            for (int i = 0; i < MAX_ALARMS; i++)
            {
                if (alarms[i].active)
                {
                    waitpid(alarms[i].pid, NULL, 0);

                    alarms[i].active = 0;
                }
            }

            printf("All alarm processes terminated.\n");
            printf("Alarm Clock System exited successfully.\n");

            break;
        }

        else
        {
            printf("Invalid choice. Please enter 1-4.\n");
        }
    }

    return 0;
}
