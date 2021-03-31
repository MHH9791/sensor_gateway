#ifndef _FIFO_H_
#define _FIFO_H_

#include <stdio.h>
#include <pthread.h>
#include "errmacros.h"
#include <stdbool.h>

typedef struct  
{
  pthread_mutex_t *mutex_fifo;
  FILE * fifo_reader;
}fifo_struct_t;

void my_fifo_writer(fifo_struct_t *connmgr_fifo, char *send_buf, bool with_time_stamp);

#endif