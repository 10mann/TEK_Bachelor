#ifndef SENSORS_CLIENT_H
#define SENSORS_CLIENT_H
#include "stdint.h"
#include "simple_sensor_client.h"
//#include "nrf_gfx.h"

typedef struct
{
  simple_sensor_data_t status[NUMBER_OF_SENSORS];
  simple_sensor_descriptor_t descriptor[NUMBER_OF_SENSORS];
  simple_sensor_setting_t settings[NUMBER_OF_SENSORS];
  simple_sensor_cadence_setting_t cadence[NUMBER_OF_SENSORS];

  uint8_t length;

} sensors_client_t;

void sensors_client_init(simple_sensor_client_t* p_client);

void print_status(uint8_t id, uint16_t length);
void print_descriptor(uint8_t id, uint16_t length);
void print_cadende(uint8_t id, uint16_t length);
void print_setting(uint8_t id, uint16_t length);
//void sensor_server_set_cb(const simple_sensor_server_t* p_self, uint8_t* p_data)

#endif