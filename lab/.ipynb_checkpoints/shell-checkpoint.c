#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>

#define MAX_COMMANDS 64
#define MAX_ARGUMENTS 64

void handle_sigchld(int sig) {
    int saved_errno = errno;
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
    errno = saved_errno;
}

int main() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int status;
    pid_t pid;

    signal(SIGCHLD, handle_sigchld);

    printf("Welcome to MyShell!\n");

    while (1) {
        printf("MyShell $ ");
        if ((read = getline(&line, &len, stdin)) == -1) {
            break;
        }

        // Remove trailing newline character
        if (line[read - 1] == '\n') line[read - 1] = '\0';

        char *args[MAX_ARGUMENTS];
        char *token = strtok(line, " ");
        int i = 0;
        bool bg_process = false;

        while (token != NULL) {
            if (strcmp(token, "&") == 0) {
                bg_process = true;
                break;
            }
            args[i] = token;
            i++;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        if (strcmp(args[0], "DONE") == 0) {
            break;
        }

        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                fprintf(stderr, "MyShell: expected argument to \"cd\"\n");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("MyShell");
                }
            }
            continue;
        }

        char *commands[MAX_COMMANDS][MAX_ARGUMENTS];
        int cmd_num = 0, arg_num = 0;

        for (i = 0; i < MAX_ARGUMENTS && args[i] != NULL; i++) {
            if (strcmp(args[i], "|") == 0) {
                commands[cmd_num][arg_num] = NULL;
                cmd_num++;
                arg_num = 0;
            } else {
                commands[cmd_num][arg_num] = args[i];
                arg_num++;
            }
        }
        commands[cmd_num][arg_num] = NULL;

        int pipefd[2];
        int old_pipefd[2];

        for (i = 0; i <= cmd_num; i++) {
            pipe(pipefd);

            pid = fork();
            if (pid == 0) {  // Child process
                if (i > 0) {  // If this isn't the first command
                    dup2(old_pipefd[0], STDIN_FILENO);  // Get input from old pipe
                }
                if (i != cmd_num) {  // If this isn't the last command
                    dup2(pipefd[1], STDOUT_FILENO);  // Send output to new pipe
                }

                close(old_pipefd[0]);
                close(old_pipefd[1]);
                close(pipefd[0]);
                close(pipefd[1]);

                execvp(commands[i][0], commands[i]);
                exit(EXIT_FAILURE);
            } else {  // Parent process
                if (i > 0) {  // If this isn't the first command
                    close(old_pipefd[0]);  // Parent doesn't need old pipe
                    close(old_pipefd[1]);
                }

                old_pipefd[0] = pipefd[0];  // Save new pipe for next iteration
                old_pipefd[1] = pipefd[1];

                if (!bg_process && i == cmd_num) {  // If this is the last command and it's not running in background
                    waitpid(pid, &status, 0);
                }
            }
        }
        close(old_pipefd[0]);
        close(old_pipefd[1]);
    }

    free(line);
    return 0;
}