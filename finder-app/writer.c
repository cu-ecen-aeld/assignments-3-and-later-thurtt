#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

const int ERR_MSG_SIZE = 255;

void error_check(int argc)
{
    printf("\n");

    if(argc < 3)
    {
        printf("Error: The writefile was not provided\n");
    }
    if(argc == 1)
    {
        printf("Error: The string to write was not provided\n");
    }

    printf("-------------------------------------------------------\n\n");
    printf("Writer usage:\n");
    printf("$ ./writer <writefile> <writestr>\n\n");
    printf("Where:\n");
    printf("<writefile> is the full path to the file to create\n");
    printf("<writestr> is the string to write to the file\n\n");
}

void write_error(int priority, const char * errmsg)
{
    syslog(priority, "%s", errmsg);
    printf("%s\n", errmsg);
}

int main(int argc, char * argv[])
{
    // Enable syslog reporting
    openlog("ecea-5305-writer", LOG_PID, LOG_USER);

    // buffer for error messages
    char errmsg[ERR_MSG_SIZE];


    // Validate inputs
    if(argc != 3)
    {
        error_check(argc);
        return 1;
    }
    const char * writefile = argv[1];
    const char * writestr = argv[2];


    // open the output file
    syslog(LOG_DEBUG, "Opening file: %s", writefile);
    FILE * h_file = fopen(writefile, "wt");
    if(h_file == 0)
    {
        int err = errno;
        snprintf(errmsg, ERR_MSG_SIZE, "Error opening %s for writing. errno: %d, message: %s", writefile, err, strerror(err));
        write_error(LOG_ERR, errmsg);
        return 1;
    }


    // Write the string to the output file
    int rc = 0;
    int bytes_written = fwrite(writestr, sizeof(char), strlen(writestr), h_file);
    syslog(LOG_DEBUG, "Wrote %d bytes to %s", bytes_written, writefile);
    if( bytes_written == 0 )
    {
        int err = errno;
        snprintf(errmsg, ERR_MSG_SIZE, "Error writing string to %s. errno: %d, message: %s", writefile, err, strerror(err));
        write_error(LOG_ERR, errmsg);
        rc = 1;
    }

    // cleanup
    fclose(h_file);
    return rc;
}
