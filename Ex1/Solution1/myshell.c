
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

void main(int argc, char *argv[])
{
    char *envPath = getenv("PATH");
    int i;
    for (i = 1; i < argc; i++)
    {
        envPath = strcat(envPath, ":");
        envPath = strcat(envPath, argv[i]);
    }
    setenv("PATH", envPath, 1);

    char history[101][101];
    pid_t pidHistroy[101];
    int num = 0;

    int condition = 1;

    pid_t mainPid = getpid();

    while (condition)
    {
        char arrayForCmd[101];
        char *arrayAfterStrtok[101];
        int numInAfter = 0;

        printf("$ ");
        fflush(stdout);
        fgets(arrayForCmd, sizeof(arrayForCmd), stdin);
        arrayForCmd[strlen(arrayForCmd) - 1] = '\0';
        if (arrayForCmd[0] == '\0')
        {
            continue;
        }

        strcpy(history[num], arrayForCmd);

        char *token;
        const char delimiter[2] = " ";
        token = strtok(arrayForCmd, delimiter);
        while (token != NULL)
        {
            arrayAfterStrtok[numInAfter] = (char *)malloc(strlen(token));
            if (arrayAfterStrtok[numInAfter] == NULL)
            {
                exit(-1);
            }
            strcpy(arrayAfterStrtok[numInAfter], token);
            token = strtok(NULL, delimiter);
            numInAfter++;
        }
        arrayAfterStrtok[numInAfter] = NULL;

        if (strcmp(arrayAfterStrtok[0], "exit") == 0)
        {
            pidHistroy[num] = mainPid;
            condition = 0;
        }
        else if (strcmp(arrayAfterStrtok[0], "cd") == 0)
        {
            pidHistroy[num] = mainPid;
            if (chdir(arrayAfterStrtok[1]) != 0)
            {
                perror("cd failed");
            }
        }
        else if (strcmp(arrayAfterStrtok[0], "history") == 0)
        {
            pidHistroy[num] = mainPid;
            for (i = 0; i <= num; i++)
            {
                printf("%d ", pidHistroy[i]);
                puts(history[i]);
            }
        }
        else
        {
            pid_t childPid;
            int status;
            if ((childPid = fork()) == 0)
            {
                execvp(arrayAfterStrtok[0], arrayAfterStrtok);
                perror("execvp failed");
                return;
            }
            else if (childPid < 0)
            {
                perror("fork failed");
            }
            pidHistroy[num] = childPid;
            wait(&status);
        }
        for (i = 0; arrayAfterStrtok[i] != NULL; i++)
        {
            free(arrayAfterStrtok[i]);
        }
        num++;
    }
}