#ifndef _DEFS_H_
#define _DEFS_H_

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include "bsd/queue.h"
#include <sys/syslog.h>
#include <time.h>
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
#include <pthread.h>

// connection defines
#define PORT 9000
#define CONNS 10
#define IP_ADDR_LEN 20

// file handling defines
#define BUFFER_SIZE 1024
#define PID_FILE "/tmp/aesdsocket.pid"
#define OUT_FILE "/var/tmp/aesdsocketdata"

// timer defines
#define TIMESTAMP_MSG_SIZE 256
#define TIMER_INTERVAL 10

// shared parameters passed to each thread
struct thread_params
{
    pthread_mutex_t * p_mut;
    char ip_addr[IP_ADDR_LEN];
    bool thread_complete;
    int socket;
};

// linked list node for tracking thread ids
struct thread_id_item
{
    pthread_t id;
    struct thread_params params;
    SLIST_ENTRY(thread_id_item) entries;
};

// socket prototypes
void * socket_worker(void * params);

// daemon prototypes
int daemon_runner();


// timer prototypes
void timer_worker(union sigval sv);
timer_t setup_timer(int interval, pthread_mutex_t  * p_mutex);

#endif // _DEFS_H_
