#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>

volatile sig_atomic_t alarm_triggered = 0;

void alarm_handler(int sig)
{
    alarm_triggered = 1;

    printf("\n=================================\n");
    printf("        ALARM RINGING! 🔔\n");
    printf("=================================\n");
    printf("\a");
    fflush(stdout);
}

int main()
{
    int choice;
    int seconds;
    pid_t pid;

    printf("====================================\n");
    printf("       LINUX ALARM CLOCK SYSTEM\n");
    printf("====================================\n");

    printf("Enter alarm delay in seconds: ");
    scanf("%d", &seconds);

    if (seconds <= 0)
    {
        printf("Invalid alarm time.\n");
        return 1;
    }

    signal(SIGALRM, alarm_handler);

    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        printf("\n[Child Process]\n");
        printf("Alarm process created.\n");
        printf("Child PID: %d\n", getpid());
        printf("Waiting for %d seconds...\n", seconds);

        alarm(seconds);

        while (!alarm_triggered)
        {
            pause();
        }

        printf("Alarm process completed.\n");
        exit(0);
    }
    else
    {
        printf("\n[Parent Process]\n");
        printf("Parent PID: %d\n", getpid());
        printf("Child PID : %d\n", pid);
        printf("Parent continues other activities...\n");

        printf("\nEnter 1 to cancel alarm or 0 to wait: ");
        scanf("%d", &choice);

        if (choice == 1)
        {
            kill(pid, SIGTERM);
            printf("Alarm cancelled successfully.\n");
        }

        waitpid(pid, NULL, 0);

        printf("\nParent process finished.\n");
    }

    return 0;
}
