#include "defs.h"

void * socket_worker(void * params)
{
    if(params == NULL)
    {
        syslog(LOG_ERR, "Cannot start thread, params struct is null");
        return NULL;
    }

    pid_t tid = gettid();

    struct thread_params * p_thread_params = (struct thread_params *)params;

    syslog(LOG_INFO, "Accepted connection from %s", p_thread_params->ip_addr);

    FILE * hOut = fopen(OUT_FILE, "a+");
    const size_t buffer_size = 1024;
    size_t bytesread = 0;
    char buffer[buffer_size];
    memset(&buffer, 0, buffer_size);

    while( true )
    {
        bytesread = read(p_thread_params->socket, (void *)buffer, buffer_size);
        syslog(LOG_INFO, "[[%d]] Received %d bytes from client", tid, (int)bytesread);

        pthread_mutex_lock(p_thread_params->p_mut);
        fwrite(buffer, sizeof(char), bytesread, hOut);
        pthread_mutex_unlock(p_thread_params->p_mut);

        if( bytesread < buffer_size )
        {
            fseek(hOut, 0, SEEK_SET);
            memset(&buffer, 0, buffer_size);
            bytesread = 0;
            int byteswritten = 0;
            while( true )
            {
                bytesread = fread(&buffer, sizeof(char), buffer_size, hOut);
                pthread_mutex_lock(p_thread_params->p_mut);
                byteswritten = write(p_thread_params->socket, buffer, bytesread);
                if( byteswritten == 0 )
                {
                    printf("Warning: 0 bytes read");
                }
                pthread_mutex_unlock(p_thread_params->p_mut);
                if( bytesread < buffer_size )
                {
                    break;
                }
            }
            syslog(LOG_INFO, "[[%d]] Closed connection from %s", tid, p_thread_params->ip_addr);
            break;
        }
    }
    fclose(hOut);
    p_thread_params->thread_complete = true;

    return NULL;
}
