#ifndef SIMPLE_SENSOR_CLIENT_H
#define SIMPLE_SENSOR_CLIENT_H

#include "access.h"
#include "simple_sensor_common.h"
#include <stdint.h>

#ifndef SIMPLE_SENSOR_CLIENT_ACKED_TRANSACTION_TIMEOUT
#define SIMPLE_SENSOR_CLIENT_ACKED_TRANSACTION_TIMEOUT (SEC_TO_US(2))
#endif

#define SIMPLE_SENSOR_CLIENT_MODEL_ID (0X1102)

//  Definisjoner av typer som blir sjekket etter en overføring
typedef enum
{
  SIMPLE_SENSOR_STATUS_NO_REPLY,
  SIMPLE_SENSOR_STATUS_CANCELLED
} simple_sensor_status_t;

typedef struct __simple_sensor_client simple_sensor_client_t;

typedef void (*simple_sensor_status_cb_t)(const simple_sensor_client_t* p_self, simple_sensor_msg_status_t* p_message, uint16_t length);

typedef void (*simple_sensor_series_cb_t)(const simple_sensor_client_t* p_self, simple_sensor_series_column_t* get_series, uint16_t length);

typedef void (*simple_sensor_descriptor_cb_t)(const simple_sensor_client_t* p_self, simple_sensor_descriptor_t* p_message, uint16_t length);

typedef void (*simple_sensor_timeout_cb_t)(access_model_handle_t handle, void *p_self);

typedef void (*simple_sensor_cadence_cb_t)(const simple_sensor_client_t* p_self, simple_sensor_cadence_setting_t* p_message, uint16_t length);

typedef void (*simple_sensor_setting_cb_t)(const simple_sensor_client_t* p_self, simple_sensor_setting_t* p_message, uint16_t length);

//  Callback-funksjoner som tilhører sensor-klienten
typedef struct
{
  simple_sensor_status_cb_t status_cb;
  simple_sensor_series_cb_t series_cb;
  simple_sensor_timeout_cb_t timeout_cb;
  simple_sensor_descriptor_cb_t descriptor_cb;
  simple_sensor_cadence_cb_t cadence_cb;
  simple_sensor_setting_cb_t setting_cb;

} simple_sensor_client_callbacks_t;

struct __simple_sensor_client
{
  access_model_handle_t model_handle;

  const simple_sensor_client_callbacks_t* p_callbacks;

  struct 
  {
    bool reliable_transfer_active;
  } state;
};

uint32_t simple_sensor_client_init(simple_sensor_client_t* p_client, uint16_t element_index);

uint32_t simple_sensor_client_status_get(simple_sensor_client_t* p_client, uint8_t lpn_id, uint8_t sensor);

uint32_t simple_sensor_client_descriptor_get(simple_sensor_client_t* p_client, uint8_t sensor);

uint32_t simple_sensor_client_cadence_get(simple_sensor_client_t* p_client, uint8_t sensor);

uint32_t simple_sensor_client_setting_get(simple_sensor_client_t* p_client, uint8_t sensor);

uint32_t simple_sensor_client_pending_msg_cancel(simple_sensor_client_t* p_client);

#endif