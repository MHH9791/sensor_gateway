#include "fifo.h"
#include <stdlib.h>



void my_fifo_writer(fifo_struct_t *connmgr_fifo, char *send_buf, bool with_time_stamp)
{
    char *send_time_buf;
    if(with_time_stamp == true)
    {
        time_t current_time_fifo;
        time(&current_time_fifo);
        struct tm *tm_time = (struct tm*)malloc(sizeof(struct tm));
        gmtime_r(&current_time_fifo, tm_time); 
        asprintf(&send_time_buf, "<Time: %d/%d/%d  %d:%d:%d (%ld)> ",
             tm_time->tm_mday,
             1 + tm_time->tm_mon,
             1900 + tm_time->tm_year,
             2 + tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec,
             current_time_fifo
             );
        free(tm_time);
        tm_time = NULL;
    }
    pthread_mutex_lock((connmgr_fifo->mutex_fifo));
    if(with_time_stamp == true)
    {
        if (fputs(send_time_buf, connmgr_fifo->fifo_reader) == EOF)
        {
            fprintf( stderr, "Error writing data to fifo\n");
            free(send_time_buf);
            send_time_buf = NULL;
            exit( EXIT_FAILURE );
        }
        free(send_time_buf);
        send_time_buf = NULL;
    }
    if (fputs(send_buf, connmgr_fifo->fifo_reader) == EOF)
    {
        fprintf( stderr, "Error writing data to fifo\n");
        free(send_buf);
        send_buf = NULL;
        exit( EXIT_FAILURE );
    }
    FFLUSH_ERROR(fflush(connmgr_fifo->fifo_reader));
    pthread_mutex_unlock((connmgr_fifo->mutex_fifo));
    free(send_buf);
}
