#include "exec.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

int exec_command(int argc, char *argv[], char* redict_out) {
    if (argc < 1) {
        printf("argc is zero, command must be specified");
        return 1;
    }

    /* prepare command and arguments */
    int exec_argc = argc + 1;
    char* exec_argv[exec_argc];
    for (int i = 0; i < exec_argc-1; i++)
        exec_argv[i] = argv[i];
    exec_argv[exec_argc-1] = NULL;

    /* show command need to run */
    printf("%-25s ", "run command:");
    for (int i = 0; i < exec_argc-1; i++) {
        printf(" %s", exec_argv[i]);
    }
    printf("\n");

    /* execute command */
    int exec_pid = fork();
    if(exec_pid == 0) {  // child process
        // redirect stdout and stderr to custom file
        if (redict_out) {
            int fd = open(redict_out, O_WRONLY|O_CREAT);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        execvp(exec_argv[0], exec_argv);
    }
    return exec_pid;
}