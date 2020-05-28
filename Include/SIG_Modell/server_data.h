#ifndef SENSORS_SERVER_H
#define SENSORS_SERVER_H

#include "stdint.h"
#include "simple_sensor_server.h"
#include "simple_sensor_common.h"
#include "simple_sensor_setup_server.h"
//#include "nrf_lcd.h"

typedef struct
{
  simple_sensor_data_t status[NUMBER_OF_SENSORS];
  simple_sensor_descriptor_t descriptor[NUMBER_OF_SENSORS];
  simple_sensor_setting_t settings[NUMBER_OF_SENSORS];
  simple_sensor_cadence_setting_t cadence[NUMBER_OF_SENSORS];

  simple_sensor_series_column_t series_column[NUMBER_OF_SENSORS];
  
  //uint8_t length;
} sensors_t;



void sensors_server_init(simple_sensor_server_t* p_server, simple_sensor_setup_server_t* p_setup_server);

void sensor_set_temp(uint16_t);
void sensor_set_temp_column(uint16_t temp);
void sensor_set_humid(uint16_t);
void sensor_set_light(uint16_t);

uint16_t sensor_get_temp();
uint16_t sensor_get_humid();

#endif