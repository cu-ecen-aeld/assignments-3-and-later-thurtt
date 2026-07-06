#include "defs.h"


int daemon_runner()
{
    switch(fork())
    {
      case -1:
        return -1;
      case 0:
        // close our existing file descriptors
        close(STDIN_FILENO);

        int fd = open("/dev/null", O_RDWR);
        if(fd != STDIN_FILENO)
            return -1;
        if(dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
            return -2;
        if(dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
            return -3;

        // save pid to the run file
        pid_t pid = getpid();
        FILE * hFile = fopen(PID_FILE, "wt");
        char s_pid[10];
        snprintf(s_pid, 10, "%d", pid);
        fwrite(s_pid, sizeof(char), strnlen(s_pid, 10), hFile);
        fclose(hFile);
        break;
      default:
        exit(0);
    }
    return 0;
}
