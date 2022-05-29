#include "exec.h"

int exec_command(int argc, char *argv[]) {
    if (argc < 1) {
        printf("command not found");
        exit(1);
    }

    int pid = getpid();

    // prepare command and arguments
    int exec_argc = argc + 1;
    char* exec_argv[exec_argc];
    for (int i = 0; i < exec_argc-1; i++)
        exec_argv[i] = argv[i];
    exec_argv[exec_argc-1] = NULL;

    // show command
    DEBUG_PRINT("run command: \n");
    for (int i = 0; i < exec_argc-1; i++)
        DEBUG_PRINT("   %s \n", exec_argv[i]);

    // execute command
    int exec_pid = vfork();
    if(exec_pid == 0) {  // child process
        // redirect stdout and stderr to /dev/null
        // int fd = open("/dev/null", O_WRONLY);
        // dup2(fd, 1);
        // dup2(fd, 2);
        // close(fd);

        DEBUG_PRINT("+ Before execvp: %lld\n", get_ns_timestamp());
        execvp(exec_argv[0], exec_argv);
    
    } else {  // parent process
        return exec_pid;
        // printf("Child  PID is %d\n", exec_pid);
        wait(NULL);  // wait for child process to finish
        DEBUG_PRINT("+ Child exited : %lld\n", get_ns_timestamp());
    }
    return 0;
}