#include "defs.h"

void timer_worker(union sigval sv)
{
    char timestamp_message[TIMESTAMP_MSG_SIZE];
    memset(timestamp_message, 0 , TIMESTAMP_MSG_SIZE);

    // worker for the timer method
    pthread_mutex_t * p_mutex = (pthread_mutex_t *)sv.sival_ptr;

    FILE * hOut = fopen(OUT_FILE, "a+");
    char buffer[BUFFER_SIZE];
    memset(&buffer, 0, BUFFER_SIZE);

    time_t tme;
    struct tm * tm_struct;
    time(&tme);
    tm_struct = gmtime(&tme);
    strftime(timestamp_message, TIMESTAMP_MSG_SIZE, "timestamp:%a, %d %b %Y %T %z\n", tm_struct);
    pthread_mutex_lock(p_mutex);
    fwrite(timestamp_message, sizeof(char), strlen(timestamp_message), hOut);
    pthread_mutex_unlock(p_mutex);
    fclose(hOut);
}

timer_t setup_timer(int interval, pthread_mutex_t  * p_mutex)
{
    timer_t timer_id = 0;

    struct itimerspec its = {
        .it_value.tv_sec = 0,
        .it_value.tv_nsec = 1,
        .it_interval.tv_sec = interval,
        .it_interval.tv_nsec = 0
    };

    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timer_worker;
    sev.sigev_value.sival_ptr = p_mutex;
    sev.sigev_notify_attributes = NULL;

    if (timer_create(CLOCK_REALTIME, &sev, &timer_id) == -1) {
        syslog(LOG_ERR, "Error attempting to create the timer");
        return timer_id;
    }

    if (timer_settime(timer_id, 0, &its, NULL) == -1) {
        syslog(LOG_ERR, "Error attempting to set the initial start time");
        return timer_id;
    }
    return timer_id;

}
