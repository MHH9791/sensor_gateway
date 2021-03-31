#ifndef DEBUG // disable assert
  #define NDEBUG
#endif

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include "sbuffer.h"
#include "lib/dplist.h"
#include "lib/tcpsock.h"
#include "errmacros.h"



/*
 * more general information on the sbuffer.h file
 */
int sbuffer_init(sbuffer_t **buffer)
{
    *buffer = (sbuffer_t *)malloc(sizeof(sbuffer_t));
    MALLOC_ERR(buffer); 

    (*buffer)->head = NULL;
    (*buffer)->tail = NULL;
    (*buffer)->transmission_is_finish = '0'; 
    (*buffer)->reader1_is_ready = '0'; 
    (*buffer)->reader2_is_ready = '0'; 
    (*buffer)->database_fail = '0'; 
    
    MUTX_ERR(pthread_mutex_init(&((*buffer)->mutx), NULL)); 
    MUTX_ERR(pthread_cond_init( &((*buffer)->cond_wr), NULL));
    MUTX_ERR(pthread_cond_init( &((*buffer)->cond_rd), NULL)); 
    return SBUFFER_SUCCESS;
}

/*
 * more explanation in the sbuffer.h file
 */
int sbuffer_free(sbuffer_t **buffer)
{
    MUTX_ERR(pthread_mutex_destroy(&((*buffer)->mutx)));
    MUTX_ERR(pthread_cond_destroy(&((*buffer)->cond_wr)));
    MUTX_ERR(pthread_cond_destroy(&((*buffer)->cond_rd)));
    free(*buffer);
    *buffer = NULL; //set pointer to NULL after free it
    return SBUFFER_SUCCESS;
}


/*
 * more explanation in the sbuffer.h file
 */
int sbuffer_remove_reader1(sbuffer_t *buffer, sensor_data_t *data)
{
    assert(buffer != NULL);
    sbuffer_node_t *dummy;
    MUTX_ERR(pthread_mutex_lock(&(buffer->mutx))); //lock the struct. 
    while(buffer->head == NULL && buffer->transmission_is_finish != '1') //while loop is to avoid spurious wakeup
    {  // if sentence: connmgr is still running and there is no data to read in the buffer currently
        MUTX_ERR(pthread_cond_wait(&(buffer->cond_wr), &(buffer->mutx))); //wait for new node
    }

    *data = buffer->head->data; // only the pointer is returned. So outside the function, the data struct should be allocated
    dummy = buffer->head; 

    assert(buffer->reader1_is_ready != '1'); 
    if(buffer->reader2_is_ready == '1') //reader2 has read the first node
    {
        if (buffer->head == buffer->tail) // buffer has only one node
        {
            buffer->head = NULL;
            buffer->tail = NULL;
            if(buffer->transmission_is_finish == '1') //connmgr has been closed(last node to read in the buffer)
            {
                free(dummy); 
                buffer->reader2_is_ready = '0'; //reset flag for reader2 to quit the while loop after cond_signal
                MUTX_ERR(pthread_mutex_unlock(&(buffer->mutx)));
                MUTX_ERR(pthread_cond_signal(&(buffer->cond_rd))); //tell reader2 that reader1 has read the data
                return SBUFFER_TERMINATED; //last data 
            }
        }
        else // buffer has many nodes empty
        {
            buffer->head = buffer->head->next;
        }
        free(dummy);
        dummy = NULL;
        buffer->reader2_is_ready = '0'; 
        MUTX_ERR(pthread_mutex_unlock(&(buffer->mutx)));
        MUTX_ERR(pthread_cond_signal(&(buffer->cond_rd)));
        return SBUFFER_SUCCESS; //successfully get new data, and transmission is not finished yet
    }
    else if(buffer->reader2_is_ready == '0') //reader1 is the first thread to read the node
    {
        buffer->reader1_is_ready = '1'; //set flag
        if(buffer->head != NULL)
        {
            MUTX_ERR(pthread_cond_signal(&(buffer->cond_wr))); //more explain in sbuffer.h. 
        }
        while(buffer->reader1_is_ready == '1')
        {
            MUTX_ERR(pthread_cond_wait(&(buffer->cond_rd), &(buffer->mutx))); //be blocked, wait reader2 to finish the reading
        }
        if(buffer->transmission_is_finish == '1' && buffer->head == NULL) //transmission finished and last node is removed
        {
            MUTX_ERR(pthread_mutex_unlock(&(buffer->mutx)));
            return SBUFFER_TERMINATED; //last data
        }
        MUTX_ERR(pthread_mutex_unlock(&(buffer->mutx)));
        return SBUFFER_SUCCESS; //successfully read data
    }
    //should never reach here
    return SBUFFER_FAILURE;
}


/*
 * same mechanism with sbuffer_remove_reader1(), only change some parameters
 */
int sbuffer_remove_reader2(sbuffer_t *buffer, sensor_data_t *data)
{
    assert(buffer != NULL);
    sbuffer_node_t *dummy;
    MUTX_ERR(pthread_mutex_lock(&(buffer->mutx)));
    while(buffer->head == NULL && buffer->transmission_is_finish != '1')
    {
        MUTX_ERR(pthread_cond_wait(&(buffer->cond_wr), &(buffer->mutx)));
    }
    assert(buffer->head != NULL);
    *data = buffer->head->data;
    dummy = buffer->head;
    assert(buffer->reader2_is_ready != '1');

    if(buffer->reader1_is_ready == '1')
    {
        if (buffer->head == buffer->tail) // buffer has only one node
        {
            assert(buffer->head != NULL && buffer->tail != NULL);
            buffer->head = NULL;
            buffer->tail = NULL;
            if(buffer->transmission_is_finish == '1')
            {
                free(dummy);
                buffer->reader1_is_ready = '0';
                MUTX_ERR(pthread_mutex_unlock(&(buffer->mutx)));
                MUTX_ERR(pthread_cond_signal(&(buffer->cond_rd)));
                return SBUFFER_TERMINATED;
            }
        }
        else  // buffer has many nodes empty
        {
            buffer->head = buffer->head->next;
        }
        free(dummy);
        dummy = NULL;
        buffer->reader1_is_ready = '0';
        MUTX_ERR(pthread_mutex_unlock(&(buffer->mutx)));
        MUTX_ERR(pthread_cond_signal(&(buffer->cond_rd)));
        return SBUFFER_SUCCESS;
    }
    else if(buffer->reader1_is_ready == '0')
    {
        buffer->reader2_is_ready = '1';
        if(buffer->head != NULL)
        {
            MUTX_ERR(pthread_cond_signal(&(buffer->cond_wr)));
        }
        while(buffer->reader2_is_ready == '1')
        {
            MUTX_ERR(pthread_cond_wait(&(buffer->cond_rd), &(buffer->mutx)));
        }
        if(buffer->transmission_is_finish == '1' && buffer->head == NULL)
        {
            MUTX_ERR(pthread_mutex_unlock(&(buffer->mutx)));
            return SBUFFER_TERMINATED;
        }
        MUTX_ERR(pthread_mutex_unlock(&(buffer->mutx)));
        return SBUFFER_SUCCESS;
    }
    //should never reach here
    return SBUFFER_FAILURE;
}

/*
 * more information on sbuffer.h 
 */
int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data, bool is_last)
{
    sbuffer_node_t *dummy;
    bool is_empty = false;
    if (buffer == NULL) return SBUFFER_FAILURE;
    dummy = (sbuffer_node_t *)malloc(sizeof(sbuffer_node_t));
    MALLOC_ERR(dummy); 

    dummy->data = *data; //insert data
    dummy->next = NULL;
    MUTX_ERR(pthread_mutex_lock(&(buffer->mutx))); 
    if(buffer->tail == NULL) // buffer empty 
    {
        buffer->head = buffer->tail = dummy;
	is_empty = true;
    }
    else // buffer not empty
    {
        buffer->tail->next = dummy;
        buffer->tail = buffer->tail->next;
        printf("reach here 5\n");
    }
    if(is_last == true)
    {
        buffer->transmission_is_finish = '1';
    }
    MUTX_ERR(pthread_mutex_unlock(&(buffer->mutx)));
    if(is_empty == true)  //avoid useless signal (if buffer is not empty, no reader thread will be block at the begining of sbuffer_remove)
    {
        MUTX_ERR(pthread_cond_signal(&(buffer->cond_wr)));
    }
    return SBUFFER_SUCCESS;
}

/*
 * set database_fail bit to 1
 */
void sbuffer_database_fail(sbuffer_t *buffer)
{
    MUTX_ERR(pthread_mutex_lock(&(buffer->mutx)));
    buffer->database_fail = '1';
    MUTX_ERR(pthread_mutex_unlock(&(buffer->mutx)));
}

/*
 * check database_fail bit
 */
char sbuffer_check_database_fail(sbuffer_t *buffer)
{
    char database_is_fail;
    MUTX_ERR(pthread_mutex_lock(&(buffer->mutx)));
    database_is_fail = buffer->database_fail;
    MUTX_ERR(pthread_mutex_unlock(&(buffer->mutx)));
    return database_is_fail;
}
