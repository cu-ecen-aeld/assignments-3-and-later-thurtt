#include "defs.h"

// global-ish vars
bool shutdown_complete = false;
bool daemon_mode = false;

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

// entrypoint
int main(int argc, char * argv[])
{
    int opt;
    int bytesread = 0;

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
            bytesread = fread(s_pid, sizeof(char), 10, hFile);
            if( bytesread == 0 )
            {
                printf("Warning: 0 bytes read");
            }
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
    char ip_string[IP_ADDR_LEN];

    // Remove any old files
    remove(OUT_FILE);

    // create a new mutex
    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

    // create the head node to our thread id linked list
    SLIST_HEAD(thread_id_list_head, thread_id_item) thread_id_list = SLIST_HEAD_INITIALIZER(thread_id_list);
    SLIST_INIT(&thread_id_list);

    // create a new timer
    setup_timer(10, &mut);

    // Main socket handling loop
    while(true)
    {
        syslog(LOG_INFO, "Waiting for connection...\n");
        sck_connected = accept(sck, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
        inet_ntop(AF_INET, &(addr.sin_addr), (char *)&ip_string, IP_ADDR_LEN);

        // set up a new thread
        struct thread_id_item * thread_id_node = malloc(sizeof(struct thread_id_item));
        thread_id_node->params.thread_complete = false;
        thread_id_node->params.p_mut = &mut;
        thread_id_node->params.socket = sck_connected;
        strncpy(thread_id_node->params.ip_addr, ip_string, IP_ADDR_LEN);

        // spawn the thread and add it to our trcking list
        pthread_create(&(thread_id_node->id), NULL, socket_worker, &(thread_id_node->params));
        SLIST_INSERT_HEAD(&thread_id_list,  thread_id_node, entries);


        // clean up any completed threads
        struct thread_id_item * p_item = NULL;
        struct thread_id_item * tmp = NULL;
        SLIST_FOREACH_SAFE(p_item, &thread_id_list, entries, tmp) {

            if( p_item->params.thread_complete == true )
            {
                SLIST_REMOVE(&thread_id_list, p_item, thread_id_item, entries);
                pthread_join(p_item->id, NULL);
                free(p_item);
            }
        }
    }
}
