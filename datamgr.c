// #define SET_MAX_TEMP 20
// #define SET_MIN_TEMP 10
#include <stdio.h>
#include <assert.h>
#include "datamgr.h"
#include <inttypes.h>
#include "sbuffer.h"
#include "errmacros.h"
//#include "config.h"

#define MEMORY_ERROR "malloc failed"
#define INVALID_SENSOR_ID_ERROR "sensor id does not exist"
#define NON_FILE_EXISTED_ERROR "file does not exist"

struct map_data{
    sensor_id_t sensor_id;
    room_id_t room_id;
};
typedef struct map_data map_data_t;

struct datamgr_element {
    map_data_t mapData;
    sensor_value_t avg_value;
    sensor_ts_t last_modified_time;
    sensor_value_t record[RUN_AVG_LENGTH];
    int record_num;
};
typedef struct datamgr_element datamgr_element_t;

void * element_copy(void * element);
void element_free(void ** element);
int element_compare(void * x, void * y);

dplist_t * map_list =NULL;

//This method holds the core functionality of your datamgr. It takes in 2 file pointers to the sensor files and parses them.
//When the method finishes all data should be in the internal pointer list and all log messages should be printed to stderr.
void datamgr_parse_sensor_files(FILE *fp_sensor_map, sbuffer_t *buffer, fifo_struct_t *datamgr_fifo)
{
    map_list = dpl_create(&element_copy,&element_free,&element_compare);
        room_id_t room_id;
        fscanf(fp_sensor_map,"%hu",&room_id);
        sensor_id_t sensor_id;
        fscanf(fp_sensor_map,"%hu",&sensor_id);

    while(!feof(fp_sensor_map)){
        datamgr_element_t * new_element = malloc(sizeof(datamgr_element_t));
        ERROR_HANDLER(new_element==NULL,MEMORY_ERROR);
        new_element->mapData.sensor_id = sensor_id;
        new_element->mapData.room_id = room_id;
        new_element->avg_value=0;
        new_element->record[RUN_AVG_LENGTH];
        new_element->last_modified_time=0;
        new_element->record_num = 0;
        dpl_insert_at_index(map_list,new_element,dpl_size(map_list),false);
        fscanf(fp_sensor_map,"%hu",&room_id);
        fscanf(fp_sensor_map,"%hu",&sensor_id);
    }
    fclose(fp_sensor_map);

    sensor_data_t *data = (sensor_data_t *)malloc(sizeof(sensor_data_t)); //starts to collect data
    MALLOC_ERR(data);

    while(sbuffer_remove_reader1(buffer, data) != SBUFFER_TERMINATED){
       
        int index = dpl_get_index_of_element(map_list,(void *)data);
        if(index < 0)
        {
            char *send_buf;
            ASPRINTF_ERR(asprintf(&send_buf, " Received sensor data with invalid sensor node ID <%"SCNd16"> \n", data->id));
            my_fifo_writer(datamgr_fifo, send_buf, true);
            continue;
        }
        datamgr_element_t *sensor_with_id_from_buffer = (datamgr_element_t *)(dpl_get_element_at_index(map_list,index));
        int num = sensor_with_id_from_buffer->record_num;
        sensor_with_id_from_buffer->record[num % 5] = data->value;
        sensor_with_id_from_buffer->record_num = num + 1;
        sensor_with_id_from_buffer->last_modified_time = data->ts;
        // datamgr_element_t* element;
        for(int i = 0; i < dpl_size(map_list); i++)
        {
            datamgr_element_t *element = (datamgr_element_t*)dpl_get_element_at_index( map_list, i );
            printf("%" PRIu16 " %" PRIu16 " %g %ld\n",element->mapData.sensor_id,element->mapData.room_id,element->avg_value,element->last_modified_time);
        }
        
        if(sensor_with_id_from_buffer->record_num < RUN_AVG_LENGTH){
            fprintf(stderr,"Too less data.\n");
        }
        else
        {
            sensor_value_t temperature_total=0;
            for(int i=0; i<RUN_AVG_LENGTH; i++){
                temperature_total += sensor_with_id_from_buffer->record[i];
            }
            sensor_with_id_from_buffer->avg_value = temperature_total/RUN_AVG_LENGTH;
            if(sensor_with_id_from_buffer->avg_value<SET_MIN_TEMP)
            {
                fprintf(stderr,"Warning: sensor %d shows: in room %d temperature %f is too low. TIME: %ld\n",
                sensor_with_id_from_buffer->mapData.sensor_id, sensor_with_id_from_buffer->mapData.room_id, sensor_with_id_from_buffer->avg_value, sensor_with_id_from_buffer->last_modified_time);
            }
            if(sensor_with_id_from_buffer->avg_value>SET_MAX_TEMP)
            {
                fprintf(stderr,"Warning: sensor %d shows: in room %d temperature %f is too high. TIME: %ld\n",
                sensor_with_id_from_buffer->mapData.sensor_id, sensor_with_id_from_buffer->mapData.room_id, sensor_with_id_from_buffer->avg_value, sensor_with_id_from_buffer->last_modified_time);
            }
        }
    }
            
    int a = datamgr_get_total_sensors();
    printf("The number of sensors  = %d\n",a);  
    datamgr_element_t* element;
    for(int i = 0; i < dpl_size(map_list); i++)
    {
        element = dpl_get_element_at_index( map_list, i );
        printf("%" PRIu16 " %" PRIu16 " %g %ld\n",element->mapData.sensor_id,element->mapData.room_id,element->avg_value,element->last_modified_time);
    }
    free(data);
    datamgr_free();
}
    
//    datamgr_free();

void datamgr_free()
//This method should be called to clean up the datamgr, and to free all used memory.
//After this, any call to datamgr_get_room_id, datamgr_get_avg, datamgr_get_last_modified or datamgr_get_total_sensors will not return a valid result
{
    ERROR_HANDLER(map_list == NULL," list is NULL!(datamgr)");
    assert(map_list != NULL);
    dpl_free(&map_list,true);
}

uint16_t datamgr_get_room_id(sensor_id_t sensor_id)
//Gets the room ID for a certain sensor ID
//Use ERROR_HANDLER() if sensor_id is invalid
{
    int found = 0;
    for(int value=0; value<dpl_size(map_list); value++)
    {
        sensor_id_t id_from_data = ((datamgr_element_t*)dpl_get_element_at_index(map_list,value))->mapData.sensor_id;
        if(sensor_id==id_from_data)
        {
            found = 1;
            return ((datamgr_element_t*)dpl_get_element_at_index(map_list,value))->mapData.room_id;
        }
    }
    ERROR_HANDLER(found==0,INVALID_SENSOR_ID_ERROR);
    return -1;
}

sensor_value_t datamgr_get_avg(sensor_id_t sensor_id)
//Gets the running AVG of a certain senor ID (if less then RUN_AVG_LENGTH measurements are recorded the avg is 0)
//Use ERROR_HANDLER() if sensor_id is invalid
{
    int found = 0;
    for(int value=0; value<dpl_size(map_list); value++)
    {
        sensor_id_t id_from_data = ((datamgr_element_t*)dpl_get_element_at_index(map_list,value))->mapData.sensor_id;
        if(sensor_id==id_from_data)
        {
            found =1;
            return ((datamgr_element_t*)dpl_get_element_at_index(map_list,value))->avg_value;
        }
    }
    ERROR_HANDLER(found==0,INVALID_SENSOR_ID_ERROR);
    return -1;
}

time_t datamgr_get_last_modified(sensor_id_t sensor_id)
//Returns the time of the last reading for a certain sensor ID
//Use ERROR_HANDLER() if sensor_id is invalid
{
    int found = 0;
    for(int value=0; value<dpl_size(map_list); value++)
    {
        sensor_id_t id_from_data = ((datamgr_element_t*)dpl_get_element_at_index(map_list,value))->mapData.sensor_id;
        if(sensor_id==id_from_data)
        {
            found = 1;
            return ((datamgr_element_t*)dpl_get_element_at_index(map_list,value))->last_modified_time;
        }
    }
    ERROR_HANDLER(found==0,INVALID_SENSOR_ID_ERROR);
    return -1;

}

int datamgr_get_total_sensors()
//Return the total amount of unique sensor ID's recorded by the datamgr
{
    return dpl_size(map_list);
}

void * element_copy(void * element)
{
    datamgr_element_t * new_element = malloc(sizeof(datamgr_element_t));
    *new_element = *(datamgr_element_t*)element;
    return new_element;
}

void element_free(void ** element)
{
    free(*element);
}

int element_compare(void * x, void * y)
{
    datamgr_element_t a = *(datamgr_element_t*)x;
    sensor_data_t b = *(sensor_data_t*)y;
    if(a.mapData.sensor_id==b.id){
          return 1;
    }else{
        return 0;
    }
}



