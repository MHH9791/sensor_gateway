#ifndef _GNU_SOURCE
#define _GNU_SOURCE

#include "datamgr.c"
#include "sensor_db.c"
#include "lib/dplist.c"
#include "sbuffer.c"
#include "connmgr.c"
#include "fifo.c"
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

#define FIFO_NAME "logFifo"
#define LOG_NAME "sensor_gateway.log"
#define ROOM_SENSOR "room_sensor.map"

typedef struct share_resorce
{
    sbuffer_t *buffer;
    int port;
    fifo_struct_t fifo_log;
} share_resorce_t;

void *buffer_to_datamgr(void *buffer);
void *connmgr_to_buffer(void *buffer);
void *buffer_to_db(void *buffer);

int main(int argc, char *argv[])
{
    pthread_mutex_t  mutex_fifo;
    sbuffer_t *buffer =NULL;
    sbuffer_init(&buffer);
    MUTX_ERR(pthread_mutex_init(&mutex_fifo, NULL));
    int pipe_fd[2];

    if(argc < 2)
    {
        printf("Please indicate the port of sensor and server in command line! \n");
        exit(0);
    }
    int server_port = atoi(argv[1]);
    pipe(pipe_fd);
    CHECK_MKFIFO(mkfifo(FIFO_NAME, 0666));

    pid_t child_pid = fork();

    if(child_pid == 0){
        printf("Child process is running\n");
        close(pipe_fd[1]);    //only for read, close write
        FILE *fd_log_write = fopen(LOG_NAME, "w");
        FILE_ERROR(fd_log_write, LOG_NAME);

        FILE *fd_fifo_read = fopen(FIFO_NAME, "r");
        FILE_ERROR(fd_fifo_read, FIFO_NAME);

        int main_process_finished = 0;
        char recv_buf[FIFO_BUFF_SIZE];
        int sequence = 1;
        while(main_process_finished == 0){
            if(fgets(recv_buf, FIFO_BUFF_SIZE, fd_fifo_read) != NULL){
                char *sequence_str;
                ASPRINTF_ERR(asprintf(&sequence_str, "< %d >  ", sequence));
                fprintf(fd_log_write, "%s", sequence_str);
                fprintf(fd_log_write, "%s", recv_buf);
                fflush(fd_log_write);
	            free(sequence_str);
                sequence++;
            }
            else{
                read(pipe_fd[0],&main_process_finished,sizeof(main_process_finished));
            }
        }
        close(pipe_fd[0]);
        FILE_CLOSE_ERROR(fclose(fd_log_write));
        FILE_CLOSE_ERROR(fclose(fd_fifo_read));
    }
    else{
        FILE *fd_fifo_write = fopen(FIFO_NAME, "w");
		close(pipe_fd[0]);

        share_resorce_t * share_resorce = (share_resorce_t *)malloc(sizeof(share_resorce_t));
        MALLOC_ERR(share_resorce);
        share_resorce->buffer = buffer;
        share_resorce->port = server_port;
        share_resorce->fifo_log.mutex_fifo= &mutex_fifo;
        share_resorce->fifo_log.fifo_reader = fd_fifo_write;
        pthread_t writing_to_buffer;
        pthread_t data_to_datamgr;
        pthread_t data_to_db; 
        PTHR_ERR(pthread_create(&writing_to_buffer, NULL, &connmgr_to_buffer, (void *)share_resorce));
        PTHR_ERR(pthread_create(&data_to_datamgr, NULL, &buffer_to_datamgr, (void *)share_resorce));
        PTHR_ERR(pthread_create(&data_to_db, NULL, &buffer_to_db, (void *)share_resorce));
        PTHR_ERR(pthread_join(writing_to_buffer, NULL));
        DEBUG_PRINTF("writing to buffer over\n");

        PTHR_ERR(pthread_join(data_to_datamgr, NULL));
        DEBUG_PRINTF("data to datamgr over\n");
    
        PTHR_ERR(pthread_join(data_to_db, NULL));
        DEBUG_PRINTF("data to db over\n");

        SBUFFER_ERR(sbuffer_free(&buffer));
        FILE_CLOSE_ERROR(fclose(fd_fifo_write));

        int main_process_finished = 1;
        write(pipe_fd[1], &main_process_finished, sizeof(main_process_finished));
        close(pipe_fd[1]);

        waitpid(child_pid, NULL, 0);
        printf("child and parent process over\n");

        MUTX_ERR(pthread_mutex_destroy(&mutex_fifo));
        free(share_resorce);

        

        // if(fopen("/home/menghao/Downloads/menghao_final/sensor_data","rb") == NULL)
        // {
        //     printf("Fail to open sensor_data");
        //     exit(0);
        // }
        // FILE * sensor_data_file = fopen("/home/menghao/Downloads/menghao_final/sensor_data","rb");
        // if(fopen("/home/menghao/Downloads/menghao_final/room_sensor.map","r") == NULL)
        //     {
        //         printf("error");
        //         exit(0);
        //     }
        // FILE * sensor_map_file = fopen("/home/menghao/Downloads/menghao_final/room_sensor.map","r");

        // datamgr_parse_sensor_files(sensor_map_file,sensor_data_file);
        // uint16_t a = datamgr_get_room_id(112);
        // sensor_value_t b = datamgr_get_avg(112);
        // sensor_ts_t c= datamgr_get_last_modified(112);
        // int d = datamgr_get_total_sensors();
        // char clear_up_flag = '1';
        // sqlite3 *db = init_connection(clear_up_flag);
        // insert_sensor(db, 12, 112, 1636363);
        // disconnect(db);
    }
    // datamgr_free();
        return 0;
}

void *connmgr_to_buffer(void *share_resorce)
{
    sbuffer_t *buffer = ((share_resorce_t *)share_resorce)->buffer;
    int port = ((share_resorce_t *)share_resorce)->port;
    fifo_struct_t *connmgr_fifo = &(((share_resorce_t *)share_resorce)->fifo_log);

    connmgr_listen(port, buffer, connmgr_fifo); //connmgr_free is inside the connmgr_listen
    return NULL;
}

void *buffer_to_datamgr(void *share_resorce)
{
    FILE *sensor_map_file = fopen(ROOM_SENSOR, "r");

    if (sensor_map_file == NULL)
    {
        perror("Error while opening the data file.\n");
        exit(EXIT_FAILURE);
    }
    sbuffer_t *buffer = ((share_resorce_t *)share_resorce)->buffer;
    fifo_struct_t *datamgr_fifo = &((share_resorce_t *)share_resorce)->fifo_log;
    datamgr_parse_sensor_files(sensor_map_file, buffer, datamgr_fifo);
    fclose(sensor_map_file);
    return NULL;
}

void *buffer_to_db(void *share_resorce)
{   
    sbuffer_t *buffer = ((share_resorce_t *)share_resorce)->buffer;
    fifo_struct_t *db_fifo = &((share_resorce_t *)share_resorce)->fifo_log;
    sqlite3 *db = init_connection('1', buffer, db_fifo);
    if(db == NULL) // database failed, connmgr is shut down. Still some data in sbuffer
    {
        sensor_data_t *data = (sensor_data_t *)malloc(sizeof(sensor_data_t)); 
        MALLOC_ERR(data);
        while(sbuffer_remove_reader2(buffer, data) != SBUFFER_TERMINATED){}// help datamgr to get the data
        free(data);
        return NULL;
    }
    
    insert_sensor(db, buffer);
    
    disconnect(db, db_fifo);
    
    if((buffer->head) != NULL)// if database exit abnormally, the rest data in buffer will be removed
    {
        sensor_data_t *data = (sensor_data_t *)malloc(sizeof(sensor_data_t)); 
        MALLOC_ERR(data);
        while(sbuffer_remove_reader2(buffer, data) != SBUFFER_TERMINATED){}
        free(data);
    }     
    return NULL;

}

#endif