#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

#include "common.h"
#include "utils.h"

int exec_command(int argc, char *argv[]);