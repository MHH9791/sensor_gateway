#ifndef _SBUFFER_H_
#define _SBUFFER_H_

#include "config.h"
#include "lib/dplist.h"
#include "lib/tcpsock.h"
#include <stdbool.h>

/*
 * general information:
 * SBUFFER_FAILURE: some error with sbuffer.
 * SBUFFER_SUCCESS: the buffer method is successful.
 * SBUFFER_TERMINATED: the transmission is terminated and last data is read.
 */
#define SBUFFER_FAILURE -1
#define SBUFFER_SUCCESS 0
#define SBUFFER_TERMINATED 1

typedef struct sbuffer sbuffer_t;

typedef struct sbuffer_node sbuffer_node_t;

struct sbuffer_node{
    sbuffer_node_t *next;
    sensor_data_t data;
};

struct sbuffer {
    sbuffer_node_t *head;
    sbuffer_node_t *tail;
    pthread_mutex_t mutx; //main mutex lock
    pthread_cond_t cond_wr; //condition var between writer and reader
    pthread_cond_t cond_rd; //condition var between readers
    char transmission_is_finish; // to determine whether the connmgr stops sending data
    char reader1_is_ready; // to determine whether reader 1 has read the current node
    char reader2_is_ready; // to determine whether reader 2 has read the current node
    char database_fail; // to dertime whether connection to database fail. if fail, it will force connmgr to send terminated signal.
};

/*
 * Allocates and initializes a new shared buffer
 * Returns SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occured
 */
int sbuffer_init(sbuffer_t ** buffer);

/*
 * free sbuffer head. The function will be called at the end of main thread. At that time, 
 * all the nodes are already freed, because of the blocking mechanism of sbuffer_remove().
 * the sbuffer_free only needs to destory the lock and condition var, and free itself. 
 */
int sbuffer_free(sbuffer_t **buffer);

/*
 * General usage:
 * The two funstions are used for get data and remove sbuffer node at the same time.
 * Each function should be called by different thread, otherwise it will be blocked.
 * In the program, reader1 is for datamgr and reader2 is for strmgr. However, reader1
 * for strmgr and reader2 for datamgr is also possible. Once the function return SBUFFER_TERMINATED,
 * the function should never be called to avoid blocking(in this program).
 *
 * Mechnism: (same for reader1 and reader2)
 * 1. Before entering the main struct, the function will test whether there is node in the sbuffer. 
 * If not, cond_wait will block the thread and wait for the signal of sbuffer_insert(), which
 * will be called by connmgr.
 *  
 * 2. The signal is sent or there still has nodes in sbuffer. Assume the thread is the first to read the buffer,
 * the data will be read(node is not deleted). Afterwards, cond_wait will block this thread and wait for 
 * the other thread to read the buffer and send data to unblock this thread. Meantime, a signal cond_wr will be sent 
 * to wake the other thread. 
 *
 * 3. The other thread enthering the main struct and read the data. Afterwards, the node will be removed by this 
 * thread and signal will be sent to wake the thread that first entering the node. If that's the last node and transmission
 * is finished, SBUFFER_TERMINATED will be return.
 *
 * Remarks: The signal cond_wr in the step 2 is to avoid blocking. When two threads are blocked before first cond_wait
 * (buffer->head == NULL & transmission is not finished). When sbuffer_insert() is called, only one thread will be waked.
 * So the other thread will be blocked if that's the last sbuffer_insert();
 */
int sbuffer_remove_reader1(sbuffer_t *buffer, sensor_data_t *data);
int sbuffer_remove_reader2(sbuffer_t *buffer, sensor_data_t *data);

/* 
 * Called by connmgr. Insert new data into sbuffer. is_last determine whether the data is last data.
 */
int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data, bool is_last);

void sbuffer_database_fail(sbuffer_t *buffer);

char sbuffer_check_database_fail(sbuffer_t *buffer);

#endif  //_SBUFFER_H_

