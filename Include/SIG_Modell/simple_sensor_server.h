#ifndef SIMPLE_SENSOR_SERVER_H
#define SIMPLE_SENSOR_SERVER_H

#include <stdint.h>
#include <stdbool.h>

#include "access.h"
#include "simple_sensor_common.h"

#define SIMPLE_SENSOR_SERVER_MODEL_ID (0x1100)

typedef struct __simple_sensor_server simple_sensor_server_t;

typedef uint8_t* (*simple_sensor_get_cb_t)(const simple_sensor_server_t* p_self, uint16_t sensor_id);

typedef uint8_t* (*simple_sensor_get_series_cb_t)(const simple_sensor_server_t* p_self, simple_sensor_series_get_t* p_series_get);

typedef uint8_t* (*simple_sensor_descriptor_get_cb_t)(const simple_sensor_server_t* p_self, uint16_t sensor_id);

//  Funksjonspekere til "callback"-funksjoner, disse blir satt i sensor_data.c
typedef struct 
{
  simple_sensor_get_cb_t get_cb;

  simple_sensor_get_series_cb_t get_series_cb;

  simple_sensor_descriptor_get_cb_t descriptor_get;

} simple_sensor_callbacks_t;

//  Modell-strukt, inneholder det nødvendige for modellen
struct __simple_sensor_server
{
  access_model_handle_t model_handle;

  uint32_t tid;

  const simple_sensor_callbacks_t* p_callbacks;
};  

uint32_t simple_sensor_server_init(simple_sensor_server_t* p_server, uint16_t element_index);

uint32_t simple_sensor_server_status_publish(simple_sensor_server_t* p_server, uint16_t sensor_id);

uint32_t simple_sensor_server_series_publish(simple_sensor_server_t* p_server, uint16_t sensor_id, uint8_t x_start, uint8_t x_end);

uint32_t simple_sensor_server_descriptor_publish(simple_sensor_server_t* p_server, uint16_t sensor_id);

#endif