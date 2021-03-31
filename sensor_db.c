#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "sensor_db.h"
#include <sqlite3.h>
#include <inttypes.h>
#include "errmacros.h"



/*
 * Make a connection to the database server
 * Create (open) a database with name DB_NAME having 1 table named TABLE_NAME  
 * If the table existed, clear up the existing data if clear_up_flag is set to 1
 * Return the connection for success, NULL if an error occurs
 */
DBCONN * init_connection(char clear_up_flag, sbuffer_t *buffer, fifo_struct_t *db_fifo)
{
  sqlite3 *db = NULL;
  time_t last_try_time, cur_time;
  int num_of_try = 0;
  char *zErrMsg = 0;
  int rc;

  while(num_of_try < 3)
  {
    /* code */
    time(&last_try_time);
    time(&cur_time);
    printf("%ld\n",last_try_time);
    printf("%ld\n",cur_time);
    rc = sqlite3_open(TO_STRING(DB_NAME), &db);
    if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      char *send_buf;
      ASPRINTF_ERR(asprintf(&send_buf, "Unable to connect to SQL server.\n"));
      my_fifo_writer(db_fifo, send_buf, true); 
      while(cur_time - last_try_time < TIME_WAIT_DB)
      {
        time(&cur_time);
      }
      num_of_try++;
      if(num_of_try >= 3){
        return NULL;
      }
    }
    else{
      fprintf(stderr, "Opened database successfully\n");
      char * send_buf;
      ASPRINTF_ERR(asprintf(&send_buf, "Connection to SQL server established.\n"));
      my_fifo_writer(db_fifo, send_buf, true); 
      break;
    }
  }

  if(clear_up_flag == '1'){
    char *sql;
    sql = "DROP TABLE IF EXISTS "TO_STRING(TABLE_NAME)";"
              "CREATE TABLE SensorData(Id INTEGER PRIMARY KEY AUTOINCREMENT, sensor_id INT, sensor_value DECIMAL(4,2), timestamp TIMESTAMP);";
    int rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
    char *send_buf;
    ASPRINTF_ERR(asprintf(&send_buf, "New table <%s> created.\n", TO_STRING(TABLE_NAME)));
    my_fifo_writer(db_fifo, send_buf, true);
    if(rc){
      // fprintf(stderr, "Failed to delete table\n");
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
      sqlite3_close(db);
      return NULL;
    }
    fprintf(stdout, "Table created successfully\n");
    return db;
  }

}


/*
 * Disconnect from the database server
 */
void disconnect(DBCONN *conn, fifo_struct_t *db_fifo)
{
  int rc = sqlite3_close(conn);
  if(rc){
    printf("Fail to close the database\n");
    exit(0);
  }
  char *send_buf;
  ASPRINTF_ERR(asprintf(&send_buf, "Connection to SQL server lost.\n"));
  my_fifo_writer(db_fifo, send_buf, true);
}


/*
 * Write an INSERT query to insert a single sensor measurement
 * Return zero for success, and non-zero if an error occurs
 */
int insert_sensor(DBCONN * conn, sbuffer_t *buffer)
{
    sensor_data_t *data = (sensor_data_t *)malloc(sizeof(sensor_data_t)); // don't forget to free
    MALLOC_ERR(data); 
    char *zErrMsg = 0;
    while(sbuffer_remove_reader2(buffer, data) != SBUFFER_TERMINATED){
      
      char *sql = sqlite3_mprintf("INSERT INTO %s(sensor_id, sensor_value, timestamp) VALUES (%u, %lf, %ld)", TO_STRING(TABLE_NAME), data->id, data->value, data->ts);
      int rc = sqlite3_exec(conn, sql, 0, 0, &zErrMsg);
      sqlite3_free(sql);
      if( rc != SQLITE_OK ){
        printf("Fail to insert data");
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        sqlite3_close(conn);
      }
      else{
        fprintf(stdout, "Records created successfully\n");
      }
    }
    free(data);
    data = NULL;
    return 0;
}


/*
 * Write an INSERT query to insert all sensor measurements available in the file 'sensor_data'
 * Return zero for success, and non-zero if an error occurs
 */
int insert_sensor_from_file(DBCONN * conn, FILE * sensor_data)
{

}


/*
  * Write a SELECT query to select all sensor measurements in the table 
  * The callback function is applied to every row in the result
  * Return zero for success, and non-zero if an error occurs
  */
int find_sensor_all(DBCONN * conn, callback_t f)
{

}


/*
 * Write a SELECT query to return all sensor measurements having a temperature of 'value'
 * The callback function is applied to every row in the result
 * Return zero for success, and non-zero if an error occurs
 */
int find_sensor_by_value(DBCONN * conn, sensor_value_t value, callback_t f)
{

}


/*
 * Write a SELECT query to return all sensor measurements of which the temperature exceeds 'value'
 * The callback function is applied to every row in the result
 * Return zero for success, and non-zero if an error occurs
 */
int find_sensor_exceed_value(DBCONN * conn, sensor_value_t value, callback_t f)
{

}


/*
 * Write a SELECT query to return all sensor measurements having a timestamp 'ts'
 * The callback function is applied to every row in the result
 * Return zero for success, and non-zero if an error occurs
 */
int find_sensor_by_timestamp(DBCONN * conn, sensor_ts_t ts, callback_t f)
{

}


/*
 * Write a SELECT query to return all sensor measurements recorded after timestamp 'ts'
 * The callback function is applied to every row in the result
 * return zero for success, and non-zero if an error occurs
 */
int find_sensor_after_timestamp(DBCONN * conn, sensor_ts_t ts, callback_t f)
{
    
}