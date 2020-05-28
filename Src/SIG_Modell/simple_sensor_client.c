#include "simple_sensor_client.h"
#include "simple_sensor_common.h"

#include "access.h"
#include "access_config.h"
#include "access_reliable.h"
#include "device_state_manager.h"
#include "nrf_mesh.h"
#include "nrf_mesh_assert.h"
#include "scanner.h"

#include "log.h"

#include <stdint.h>
#include <stddef.h>

//  Callback som blir kjørt ved sending av melding som krever svar
static void relaible_status_cb(access_model_handle_t model_handle, void* p_args, access_reliable_status_t status)
{
  simple_sensor_client_t* p_client = p_args;
  NRF_MESH_ASSERT(p_client->p_callbacks->status_cb != NULL);

  p_client->state.reliable_transfer_active = false;
  scanner_disable();
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Transfer done\n");

  switch(status)
  {
    case ACCESS_RELIABLE_TRANSFER_SUCCESS:
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Transfer success\n");
      break;

    case ACCESS_RELIABLE_TRANSFER_TIMEOUT:
      //p_client->p_callbacks->status_cb(p_client, SIMPLE_SENSOR_STATUS_NO_REPLY, NRF_MESH_ADDR_UNASSIGNED); 
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Transfer timed out\n");
      break;

    case ACCESS_RELIABLE_TRANSFER_CANCELLED:
      //p_client->p_callbacks->status_cb(p_client, SIMPLE_SENSOR_STATUS_CANCELLED, NRF_MESH_ADDR_UNASSIGNED);
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Transfer cancelled\n");
      break;

    default:
      NRF_MESH_ASSERT(false);
      break;
  }
}

//  Funksjon som blir brukt for å sende en melding som krever svar
static uint32_t send_reliable_message(const simple_sensor_client_t* p_client, simple_sensor_opcode_t opcode, 
                                      const uint8_t* p_data, uint16_t length)
{
  access_reliable_t reliable;
  reliable.model_handle = p_client->model_handle;
  reliable.message.p_buffer = p_data;
  reliable.message.length = length;
  reliable.message.opcode.opcode = opcode;
  reliable.message.opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  reliable.message.force_segmented = false;
  reliable.message.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
  reliable.message.access_token = nrf_mesh_unique_token_get();
  reliable.reply_opcode.opcode = SIMPLE_SENSOR_OPCODE_STATUS;
  reliable.reply_opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  reliable.timeout = SIMPLE_SENSOR_CLIENT_ACKED_TRANSACTION_TIMEOUT;
  reliable.status_cb = relaible_status_cb;

  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nSending GET message\nSensor ID: 0x%04x\nLength: %d\n", reliable.message.p_buffer[0] | (reliable.message.p_buffer[1] << 8), reliable.message.length);
  return access_model_reliable_publish(&reliable);
}

//  En callback som blir kjørt ved operasjonskode STATUS_OPCODE
static void handle_status_cb(access_model_handle_t handle, const access_message_rx_t* p_message, void* p_args)
{
  simple_sensor_client_t* p_client = p_args;
  NRF_MESH_ASSERT(p_client->p_callbacks->status_cb != NULL);
  
  simple_sensor_msg_status_t* p_status = (simple_sensor_msg_status_t*)p_message->p_data;
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Received status, length: %d\n", p_message->length/sizeof(simple_sensor_data_t));
  p_client->p_callbacks->status_cb(p_client, p_status, p_message->length/sizeof(simple_sensor_data_t));
}

//  En callback som blir kjørt ved operasjonskode SERIES_OPCODE
static void handle_series_cb(access_model_handle_t handle, const access_message_rx_t* p_message, void* p_args)
{
  simple_sensor_client_t* p_client = p_args;
  simple_sensor_series_column_t* p_series = (simple_sensor_series_column_t*)p_message->p_data;
  p_client->p_callbacks->series_cb(p_client, p_series, p_message->length/sizeof(simple_sensor_series_column_t));
}

//  En callback som blir kjørt ved operasjonskode DESCRIPTOR_OPCODE
static void handle_descriptor_cb(access_model_handle_t handle, const access_message_rx_t* p_message, void* p_args)
{
  simple_sensor_client_t* p_client = p_args;
  NRF_MESH_ASSERT(p_client->p_callbacks->descriptor_cb != NULL);

  simple_sensor_descriptor_t* p_descriptor = (simple_sensor_descriptor_t*)p_message->p_data;
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Received descriptor\n");
  p_client->p_callbacks->descriptor_cb(p_client, p_descriptor, p_message->length/sizeof(simple_sensor_descriptor_t));
}

//  En callback som blir kjørt ved operasjonskode CADENCE_OPCODE
static void handle_cadence_cb(access_model_handle_t handle, const access_message_rx_t* p_message, void* p_args)
{
  simple_sensor_client_t* p_client = p_args;

  p_client->p_callbacks->cadence_cb(p_client, (simple_sensor_cadence_setting_t*)p_message->p_data, p_message->length/sizeof(simple_sensor_cadence_setting_t));
}

//  En callback som blir kjørt ved operasjonskode SETTINGS_OPCODE
static void handle_settings_cb(access_model_handle_t handle, const access_message_rx_t* p_message, void* p_args)
{
  simple_sensor_client_t* p_client = p_args;

  p_client->p_callbacks->setting_cb(p_client, (simple_sensor_setting_t*)p_message->p_data, p_message->length/sizeof(simple_sensor_setting_t));
}

//  En liste med operasjonskoder og callback-funksjoner
static access_opcode_handler_t m_opcode_handlers[] = 
{
  {{SIMPLE_SENSOR_OPCODE_CADENCE_STATUS, SIMPLE_SENSOR_COMPANY_ID}, handle_cadence_cb},
  {{SIMPLE_SENSOR_OPCODE_STATUS, SIMPLE_SENSOR_COMPANY_ID}, handle_status_cb},
  {{SIMPLE_SENSOR_OPCODE_DESCRIPTOR_STATUS, SIMPLE_SENSOR_COMPANY_ID}, handle_descriptor_cb},
  {{SIMPLE_SENSOR_OPCODE_SETTING_STATUS, SIMPLE_SENSOR_COMPANY_ID}, handle_settings_cb},
  {{SIMPLE_SENSOR_OPCODE_SETTINGS_STATUS, SIMPLE_SENSOR_COMPANY_ID}, handle_settings_cb},
};

//  Funksjon som blir kjørt ved timeout
static void handle_publish_timeout(access_model_handle_t handle, void* p_args)
{
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Transfer timed out\n");
  simple_sensor_client_t* p_client = p_args;

  if(p_client->p_callbacks->status_cb != NULL)
    p_client->p_callbacks->timeout_cb(handle, p_args);
}

//  Setter opp og starter klien-modellen
uint32_t simple_sensor_client_init(simple_sensor_client_t* p_client, uint16_t element_index)
{
  access_model_add_params_t init_params;
  init_params.model_id.model_id = SIMPLE_SENSOR_CLIENT_MODEL_ID;
  init_params.model_id.company_id = SIMPLE_SENSOR_COMPANY_ID;
  init_params.element_index = element_index;
  init_params.p_opcode_handlers = &m_opcode_handlers[0];
  init_params.opcode_count = sizeof(m_opcode_handlers) / sizeof(m_opcode_handlers[0]);
  init_params.p_args = p_client;
  init_params.publish_timeout_cb = handle_publish_timeout;
  uint32_t status = access_model_add(&init_params, &p_client->model_handle);
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Opcode count: %d, 0x%04x, 0x%04x\n", init_params.opcode_count, init_params.p_opcode_handlers[0], init_params.p_opcode_handlers[2]);
  if (status == NRF_SUCCESS)
  {
      status = access_model_subscription_list_alloc(p_client->model_handle);
  }
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Sensor client started\n");
  return status;
}

//  Sender en melding med operasjonskode STATUS_GET
uint32_t simple_sensor_client_status_get(simple_sensor_client_t* p_client, uint8_t lpn_id, uint8_t sensor)
{
  if(p_client->p_callbacks->status_cb == NULL)
  {
    return NRF_ERROR_NULL;
  }

  else if(p_client->state.reliable_transfer_active)
  {
    return NRF_ERROR_INVALID_STATE;
  }

  uint8_t msg_buffer[2] = {sensor, lpn_id};
  
  uint32_t status = send_reliable_message(p_client, SIMPLE_SENSOR_OPCODE_GET, msg_buffer, sizeof(msg_buffer));

  if(status == NRF_SUCCESS)
  {
    p_client->state.reliable_transfer_active = true;
  }
  return status;
}

//  Sender en melding med operasjonskode DESCRIPTOR_GET
uint32_t simple_sensor_client_descriptor_get(simple_sensor_client_t* p_client, uint8_t sensor)
{ 
  if(p_client->p_callbacks->descriptor_cb == NULL)
  {
    return NRF_ERROR_NULL;
  }
  else if(p_client->state.reliable_transfer_active)
  {
    return NRF_ERROR_INVALID_STATE;
  }

  uint32_t status = send_reliable_message(p_client, SIMPLE_SENSOR_OPCODE_DESCRIPTOR_GET, &sensor, sizeof(sensor));
  if(status == NRF_SUCCESS)
  {
    p_client->state.reliable_transfer_active = true;
  }
  else
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Failed to send reliable message\n");
  }
  return status;
}

//  Sender en melding med operasjonskode CADENCE_GET
uint32_t simple_sensor_client_cadence_get(simple_sensor_client_t* p_client, uint8_t sensor)
{
  if(p_client->p_callbacks->cadence_cb == NULL)
  {
    return NRF_ERROR_NULL;
  }
  else if(p_client->state.reliable_transfer_active)
  {
    return NRF_ERROR_INVALID_STATE;
  }
  
  uint32_t status = send_reliable_message(p_client, SIMPLE_SENSOR_OPCODE_CADENCE_GET, &sensor, sizeof(sensor));
  if(status == NRF_SUCCESS)
  {
    p_client->state.reliable_transfer_active = true;
  }
  else
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Failed to send cadence get message\n");
  }
  return status;
}

//  Sender en melding med operasjonskode SETTING_GET
uint32_t simple_sensor_client_setting_get(simple_sensor_client_t* p_client, uint8_t sensor)
{
  if(p_client->p_callbacks->setting_cb == NULL)
  {
    return NRF_ERROR_NULL;
  }
  else if(p_client->state.reliable_transfer_active)
  {
    return NRF_ERROR_INVALID_STATE;
  }

  access_reliable_t reliable;
  reliable.message.length = sizeof(sensor);
  reliable.message.p_buffer = &sensor;
  reliable.message.opcode.opcode = SIMPLE_SENSOR_OPCODE_SETTING_GET;
  reliable.message.opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  reliable.message.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
  reliable.message.force_segmented = false;
  reliable.message.access_token = nrf_mesh_unique_token_get();
  reliable.model_handle = p_client->model_handle;
  reliable.reply_opcode.opcode = SIMPLE_SENSOR_OPCODE_SETTING_STATUS;
  reliable.reply_opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  reliable.status_cb = relaible_status_cb;
  reliable.timeout = SIMPLE_SENSOR_CLIENT_ACKED_TRANSACTION_TIMEOUT;

  return access_model_reliable_publish(&reliable);
 }

//  Sender en melding med operasjonskode SETTINGS_GET
 uint32_t simple_sensor_client_settings_get(simple_sensor_client_t* p_client)
{
  if(p_client->p_callbacks->setting_cb == NULL)
  {
    return NRF_ERROR_NULL;
  }
  else if(p_client->state.reliable_transfer_active)
  {
    return NRF_ERROR_INVALID_STATE;
  }
  
  access_reliable_t reliable;
  reliable.message.length = 1;
  reliable.message.p_buffer = NULL;
  reliable.message.opcode.opcode = SIMPLE_SENSOR_OPCODE_SETTINGS_GET;
  reliable.message.opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  reliable.message.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
  reliable.message.force_segmented = false;
  reliable.message.access_token = nrf_mesh_unique_token_get();
  reliable.model_handle = p_client->model_handle;
  reliable.reply_opcode.opcode = SIMPLE_SENSOR_OPCODE_SETTINGS_STATUS;
  reliable.reply_opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  reliable.status_cb = relaible_status_cb;
  reliable.timeout = SIMPLE_SENSOR_CLIENT_ACKED_TRANSACTION_TIMEOUT;

  return access_model_reliable_publish(&reliable);
 }

//  Sender en melding med operasjonskode SETTING_SET_OPCODE
 uint32_t simple_sensor_client_setting_set(simple_sensor_client_t* p_client, simple_sensor_setting_t* p_setting)
{
  if(p_client->p_callbacks->setting_cb == NULL)
  {
    return NRF_ERROR_NULL;
  }
  else if(p_client->state.reliable_transfer_active)
  {
    return NRF_ERROR_INVALID_STATE;
  }
  uint8_t test = 1;

  access_reliable_t reliable;
  reliable.message.length = sizeof(simple_sensor_setting_t);
  reliable.message.p_buffer = (uint8_t*)p_setting;
  reliable.message.opcode.opcode = SIMPLE_SENSOR_OPCODE_SETTING_SET;
  reliable.message.opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  reliable.message.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
  reliable.message.force_segmented = false;
  reliable.message.access_token = nrf_mesh_unique_token_get();
  reliable.model_handle = p_client->model_handle;
  reliable.reply_opcode.opcode = SIMPLE_SENSOR_OPCODE_SETTING_STATUS;
  reliable.reply_opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  reliable.status_cb = relaible_status_cb;
  reliable.timeout = SIMPLE_SENSOR_CLIENT_ACKED_TRANSACTION_TIMEOUT;

  return access_model_reliable_publish(&reliable);
 }

//  Avbryter sending av melding som krever svar
void simple_sensor_client_pending_message_cancel(simple_sensor_client_t* p_client)
{
  (void)access_model_reliable_cancel(p_client->model_handle);
}
