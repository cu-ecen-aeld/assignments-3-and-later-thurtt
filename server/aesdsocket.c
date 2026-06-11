#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>


// global-ish vars
bool shutdown_complete = false;
bool daemon_mode = false;

// constants
static const int PORT = 9000;
static const char PID_FILE[] = "/tmp/aesdsocket.pid";
static const char OUT_FILE[] = "/var/tmp/aesdsocketdata";
static const int CONNS = 10;


void show_help()
{
    printf("\naesdsocket command line help\n");
    printf("----------------------------\n\n");
    printf("Usage:\n");
    printf("  aesdsocket\n");
    printf("  aesdsocket -d\n");
    printf("  aesdsocket -s\n");
    printf("  aesdsocket -h\n\n");
    printf("Options:\n");
    printf("-d     Start aesdsocket as a daemon\n");
    printf("-s     Shut down the currently running daemon\n");
    printf("-h     Show this screen\n\n");
}

static void sig_handler(int signo)
{
    syslog(LOG_DEBUG, "Caught signal, exiting");
    if( daemon_mode == true )
    {
        remove(PID_FILE);
    }

    exit(0);
}

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

int main(int argc, char * argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "dsh")) != -1) {
        switch (opt) {
        case 'd':
            daemon_mode = true;
            break;
        case 's':
            FILE * hFile = fopen(PID_FILE, "rt");
            if( hFile == NULL )
            {
                printf("Could not locate the pid at %s. You will need to kill the process manually", PID_FILE);
                exit(1);
            }
            char s_pid[10];
            memset(s_pid, 0, 10);
            fread(s_pid, sizeof(char), 10, hFile);
            fclose(hFile);
            int pid = atoi(s_pid);
            printf("Shutting down pid: %d\n", pid);
            kill(pid, SIGTERM);
            exit(0);
        case 'h':
            show_help();
            exit(1);
        }
    }

    // register a handler for SIGINT and SIGTERM
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    // create a new socket
    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if( sck == -1 )
    {
        printf("Error attempting to create a socket\n");
        printf("%s\n", strerror(errno));
        exit(-1);
    }


    // attempt to listen on the specified port
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    // bind the socket to the port
    int rc = bind(sck, (struct sockaddr *)&addr, addrlen);
    if( rc == -1 )
    {
        printf("Error attempting to bind the socket: %s\n", strerror(errno));
        exit(-1);
    }

    // listen for incoming connections
    rc = listen(sck, CONNS);
    if( rc == -1 )
    {
        printf("Error attempting to listen on the socket: %s\n", strerror(errno));
        exit(-1);
    }

    if( daemon_mode == true )
    {
        printf("Starting aesdsocket in daemon mode...\n");

        switch(fork())
        {
          case -1:
            return -1;
          case 0:
            rc = daemon_runner();
            if( rc != 0 )
            {
                return rc;
            }
            break;
          default:
            exit(0);
        }
    }

    int sck_connected;
    char ip_string[20];

    // Remove any old files
    remove(OUT_FILE);

    // Main socket handling loop
    while(true)
    {
        syslog(LOG_INFO, "Waiting for connection...\n");
        sck_connected = accept(sck, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
        inet_ntop(AF_INET, &(addr.sin_addr), (char *)&ip_string, 20);
        syslog(LOG_INFO, "Accepted connection from %s", ip_string);

        FILE * hOut = fopen(OUT_FILE, "a+");
        const size_t buffer_size = 1024;
        size_t bytesread = 0;
        char buffer[buffer_size];
        memset(&buffer, 0, buffer_size);

        while( true )
        {
            bytesread = read(sck_connected, (void *)buffer, buffer_size);
            syslog(LOG_INFO, "Received %d bytes from client", (int)bytesread);
            fwrite(buffer, sizeof(char), bytesread, hOut);

            if( bytesread < buffer_size )
            {
                fseek(hOut, 0, SEEK_SET);
                memset(&buffer, 0, buffer_size);
                bytesread = 0;
                while( true )
                {
                    bytesread = fread(&buffer, sizeof(char), buffer_size, hOut);
                    write(sck_connected, buffer, bytesread);
                    if( bytesread < buffer_size )
                    {
                        break;
                    }
                }
                syslog(LOG_INFO, "Closed connection from %s", ip_string);
                break;
            }
        }
        fclose(hOut);

        sleep(1);
    }
}
