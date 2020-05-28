#ifndef SIMPLE_SENSOR_SETUP_SERVER_H
#define SIMPLE_SENSOR_SETUP_SERVER_H

#include <stdint.h>
#include <stdbool.h>

#include "simple_sensor_server.h"
#include "simple_sensor_common.h"
#include "access.h"

#define SIMPLE_SENSOR_SETUP_SERVER_MODEL_ID (0x1101)

typedef struct __simple_sensor_setup_server simple_sensor_setup_server_t;

typedef uint8_t* (*simple_sensor_setup_server_cadence_get_cb_t)(const simple_sensor_setup_server_t* p_self, uint16_t sensor_id);

typedef uint8_t* (*simple_sensor_setup_server_settings_get_cb_t)(const simple_sensor_setup_server_t* p_self, uint16_t sensor_id);

typedef uint8_t* (*simple_sensor_setup_server_setting_get_cb_t)(const simple_sensor_setup_server_t* p_self, uint16_t sensor_id);

typedef void (*simple_sensor_setup_server_cadence_set_cb_t)(const simple_sensor_setup_server_t* p_self, uint8_t* p_data, uint16_t sensor_id);

typedef void (*simple_sensor_setup_server_setting_set_cb_t)(const simple_sensor_setup_server_t* p_self, uint16_t sensor_id);

//  Callback-funksjoner som tilhører setup-server-modellen
typedef struct
{
  simple_sensor_setup_server_cadence_get_cb_t cadence_get;

  simple_sensor_setup_server_settings_get_cb_t settings_get;

  simple_sensor_setup_server_setting_get_cb_t setting_get;

  simple_sensor_setup_server_cadence_set_cb_t cadence_set;

  simple_sensor_setup_server_setting_set_cb_t setting_set;

} simple_sensor_setup_server_callbacks_t;


struct __simple_sensor_setup_server
{
  access_model_handle_t model_handle;

  const simple_sensor_setup_server_callbacks_t* p_callbacks;
};


uint32_t simple_sensor_setup_server_init(simple_sensor_setup_server_t* p_setup_server, uint16_t element_index);

uint32_t simple_sensor_setup_server_cadence_status_publish(simple_sensor_setup_server_t* p_setup_server, uint16_t sensor_id);

uint32_t simple_sensor_setup_server_setting_status_publish(simple_sensor_setup_server_t* p_setup_server, uint16_t sensor_id);

uint32_t simple_sensor_setup_server_settings_status_publish(simple_sensor_setup_server_t* p_setup_server);

#endif