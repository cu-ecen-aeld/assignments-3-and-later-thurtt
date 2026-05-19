#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "systemcalls.h"


bool wait_for_complete(int pid)
{
    bool result = false;

    // wait for the child process to exit and return true if everything is good
    int stat = 0;
    pid_t p_rc = waitpid(pid, &stat, 0);
    bool exited = WIFEXITED(stat);
    printf("Exited state for pid %d: %d\n", pid, exited);
    int exit_status = WEXITSTATUS(stat);
    printf("Exit status for pid %d: %d\n", pid, exit_status);
    bool signaled = WIFSIGNALED(stat);
    printf("Signaled status for pid %d: %d\n", pid, signaled);
    if( p_rc == pid
        && exited == true
        && exit_status == 0
        && signaled == false )
    {
        result = true;
    }
    return result;
}

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    bool result = false;
    int rc = system(cmd);

    if( rc == 0 )
    {
        result = true;
    }

    return result;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];

    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = (char *)NULL;

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    bool result = false;

    // spawn a child process and execute the passed in command
    pid_t pid = fork();

    if( pid == 0 )
    {
        execv(command[0], command);
        printf("execv call failed!\n");
        printf("Error no %d, message: %s\n", errno, strerror(errno));
        exit(1);
    }
    if( pid > 0 )
    {
        // If we're the parent, wait for the child to complete
        result = wait_for_complete(pid);
    }

    va_end(args);

    return result;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];

    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
// Source - https://stackoverflow.com/a/13784315
// Posted by tmyklebu, modified by community. See post 'Timeline' for change history
// Retrieved 2026-05-15, License - CC BY-SA 3.0

    bool result = false;
    int dfd = 0;

    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
    if( fd != -1 )
    {
        int pid = fork();
        switch( pid )
        {
        case 0:
            dfd = dup2(fd, 1);
            if( dfd == -1 )
            {
                return false;
            }
            close(fd);
            execv(command[0], command);
            printf("execv call failed!\n");
            printf("Error no %d, message: %s\n", errno, strerror(errno));
            exit(1);
        default:
            // If we're the parent, wait for the child to complete
            result = wait_for_complete(pid);
        }
        close(fd);
    }

    va_end(args);

    return result;
}
