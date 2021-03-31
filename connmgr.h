#ifndef _CONNMGR_H_
#define _CONNMGR_H_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include "config.h"
#include "lib/tcpsock.h"
#include "lib/tcpsock.c"
#include <poll.h>
#include "config.h"
#include "sbuffer.h"
#include "fifo.h"


#define	CONNMGR_NO_ERROR		0
#define	CONNMGR_POLL_ERROR	1  // failed poll

#define	CONNMGR_ADDRESS_ERROR	2  // invalid port and/or IP address
#define	CONNMGR_SOCKOP_ERROR	3  // socket operator (socket, listen, bind, accept,...) error
#define CONNMGR_CONNECTION_CLOSED	4  // send/receive indicate connection is closed
#define	CONNMGR_MEMORY_ERROR	5  // mem alloc error

#ifndef TIMEOUT
#define TIMEOUT 5
#endif
struct sock{
    tcpsock_t* addr;
    int sd;   //sock id
    int status;      //0 open  1 closed
    long lastActive;
};
typedef struct sock sock_t;
sock_t* connmgr;  //store socks

// This method holds the core functionality of your connmgr. It starts listening on the given port and when 
// a sensor node connects it writes the data to a sensor_data_recv file. 
// This file must have the same format as the sensor_data file in assignment 6 and 7.

void connmgr_listen(int port_number, sbuffer_t *buffer, fifo_struct_t *connmgr_fifo);

//This method should be called to clean up the connmgr, and to free all used memory. After this no new connections will be accepted
void connmgr_free();

#endif 