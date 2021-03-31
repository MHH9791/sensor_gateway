#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "connmgr.h"
#include "errmacros.h"


void connmgr_exit();

#ifdef DEBUG
	#define CONNMGR_DEBUG_PRINTF(condition,...)									\
		do {												\
		   if((condition)) 										\
		   {												\
			fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);	\
			fprintf(stderr,__VA_ARGS__);								\
		   }												\
		} while(0)						
#else
	#define TCP_DEBUG_PRINTF(...) (void)0
#endif

#define CONNMGR_ERR_HANDLER(condition,...)	\
	do {						\
		if ((condition))			\
		{					\
		  CONNMGR_DEBUG_PRINTF(1,"error condition \"" #condition "\" is true\n");	\
		  __VA_ARGS__;				\
		}					\
	} while(0)

// #define FILE_ERROR (fp,error_msg) 	\
//     do { \
// 					  if ((fp)==NULL) { \
// 					    printf("%s\n",(error_msg)); \
// 					    exit(EXIT_FAILURE); \
// 					  }	\
// 					} while(0)


void * element_copy(void * element);
void element_free(void ** element);
int element_compare(void * x, void * y);

void connmgr_listen(int port_number, sbuffer_t *buffer, fifo_struct_t *connmgr_fifo)
{
    connmgr=(sock_t*)malloc(sizeof(sock_t));
    tcpsock_t *server = NULL;
    tcpsock_t *client = NULL;
    sensor_data_t data;
    int bytes, result;
    int conn_counter=0;
    long timeNow;
    long lastActive;
    
    connmgr->addr=server;
    connmgr->sd=0; //add server sock in the connmgr
    connmgr->status=0;
    time(&connmgr->lastActive);

    if (tcp_passive_open(&server,port_number)!=TCP_NO_ERROR) exit(EXIT_FAILURE);
    printf("connect manager is starting to listen");
    char *send_buf;
    ASPRINTF_ERR(asprintf(&send_buf, "Server is started!\n"));
    my_fifo_writer(connmgr_fifo, send_buf, true);

    struct pollfd * fd_array;
    fd_array=(struct pollfd*)malloc(sizeof(struct pollfd));
    fd_array->fd=server->sd;
    fd_array->events=POLLIN;

    while(1)
    {
        
        int revent_num = poll(fd_array,conn_counter+1,-1);
        printf("revent num = %d\n", revent_num);

        if((fd_array->revents&POLLIN)==POLLIN)  //check if there is connection  socket detector
        {
            if (tcp_wait_for_connection(server,&client)!=TCP_NO_ERROR) exit(EXIT_FAILURE);
            conn_counter++;
            connmgr= realloc(connmgr,(1+conn_counter)*sizeof(sock_t));
            (connmgr+conn_counter)->addr=client;
            (connmgr+conn_counter)->sd=conn_counter; //add client sock in the connmgr
            (connmgr+conn_counter)->status=0;
            time(&(connmgr+conn_counter)->lastActive);
            printf("Incoming  client %d connection\n",conn_counter);
            fflush(stdout);
            
            fd_array=(struct pollfd*)realloc(fd_array,(1+conn_counter)* sizeof(struct pollfd));
            (fd_array+conn_counter)->fd=client->sd;      //add new poll detection
            (fd_array+conn_counter)->events=POLLIN;
            
        }

        
        for(int i=1;i<=conn_counter;i++)            //detect if a certain sensor can receive data
        {
            if((connmgr+i)->status) {            //if already closed
                continue;
            } 
                                  //not closed but inactive
            time(&timeNow);
            if(timeNow-(connmgr+i)->lastActive>TIMEOUT)
            {
                tcp_close(&((connmgr+i)->addr));                
                (connmgr+i)->status=1;                
                printf("Timeout !connition with clint %d is closed by server\n", i);
                fflush(stdout);
                  continue;
            }


            if((((fd_array+i)->revents&POLLIN)==POLLIN)|POLLERR)
            {
                // read sensor ID
                bytes = sizeof(data.id);
                result = tcp_receive((connmgr+i)->addr,(void *)&data.id,&bytes);
                // read temperature
                bytes = sizeof(data.value);
                result = tcp_receive((connmgr+i)->addr,(void *)&data.value,&bytes);
                // read timestamp
                bytes = sizeof(data.ts);
                result = tcp_receive((connmgr+i)->addr, (void *)&data.ts,&bytes);
                if ((result==TCP_NO_ERROR) && bytes) 
                {
                    SBUFFER_ERR(sbuffer_insert(buffer, &data, false));  //send data into sbuffer
                    time(&(connmgr+i)->lastActive);
                    time(&lastActive);
                    // revent_num--;
                    
                }
                if (result==TCP_CONNECTION_CLOSED){
                    char *send_buf;
                    ASPRINTF_ERR(asprintf(&send_buf, "A sensor node with %" PRIu16 " stops sending data.\n", data.id));
                    my_fifo_writer(connmgr_fifo, send_buf, true);
                    tcp_close(&((connmgr+i)->addr));
                    (connmgr+i)->status=1;
                    revent_num--;
                }
                
                // if((revent_num-1)<=0)
                //     break;
            }
             
        }
        time(&timeNow);
        if(timeNow-lastActive>TIMEOUT)
        {                   
            printf("Timeout! server closed \n");
            fflush(stdout);
            break;  
        }
        
    }
   //  exit.......
   free(fd_array);
   for(int i=1;i<conn_counter;i++)
   {
       if((connmgr+i)->status==0)
       tcp_close(&((connmgr+i)->addr));  //或许可以用socket里的变量
   }
   free(server);
   free(connmgr);
}
