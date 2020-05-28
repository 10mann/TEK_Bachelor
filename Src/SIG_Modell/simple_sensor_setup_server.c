#include "simple_sensor_setup_server.h"

#include <stdint.h>
#include <stddef.h>

#include "access.h"
#include "access_config.h"
#include "access_reliable.h"
#include "nrf_mesh_assert.h"
#include "log.h"

static void handle_settings_get(access_model_handle_t handle, const access_message_rx_t* p_message, void* p_args)
{
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "SETTINGS GET\n");
  simple_sensor_setup_server_t* p_setup_server = p_args;
}


static void handle_cadence_get(access_model_handle_t handle, const access_message_rx_t* p_message, void* p_args)
{
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "CADENCE GET\n");
  simple_sensor_setup_server_t* p_setup_server = p_args;
}


static void handle_setting_get(access_model_handle_t handle, const access_message_rx_t* p_message, void* p_args)
{
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "SETTING GET\n");
  simple_sensor_setup_server_t* p_setup_server = p_args;
  uint8_t* p_buffer = p_setup_server->p_callbacks->setting_get(p_setup_server, p_message->p_data[0]);
}

//  En liste med operasjonskoder og handler-funksjoner
static const access_opcode_handler_t m_setup_opcode_handlers[] =
{
  {ACCESS_OPCODE_VENDOR(SIMPLE_SENSOR_OPCODE_CADENCE_GET, SIMPLE_SENSOR_COMPANY_ID), handle_cadence_get},
  {ACCESS_OPCODE_VENDOR(SIMPLE_SENSOR_OPCODE_SETTINGS_GET, SIMPLE_SENSOR_COMPANY_ID), handle_settings_get},
  {ACCESS_OPCODE_VENDOR(SIMPLE_SENSOR_OPCODE_SETTING_GET, SIMPLE_SENSOR_COMPANY_ID), handle_setting_get},

};


static void handle_publish_timeout(access_model_handle_t handle, void* p_args)
{
  simple_sensor_setup_server_t* p_setup_server = p_args;
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\n\nReplying to status timed out\n\n");
  //(void)simple_sensor_server_status_publish(p_server, p_server->p_callbacks->get_cb(p_server, NULL));
}

//  Starter setup-server-modellen
uint32_t simple_sensor_setup_server_init(simple_sensor_setup_server_t* p_setup_server, uint16_t element_index)
{
  access_model_add_params_t init_params;
  init_params.element_index = element_index;
  init_params.model_id.model_id = SIMPLE_SENSOR_SETUP_SERVER_MODEL_ID;
  init_params.model_id.company_id = SIMPLE_SENSOR_COMPANY_ID;
  init_params.p_opcode_handlers = &m_setup_opcode_handlers[0];
  init_params.opcode_count = sizeof(m_setup_opcode_handlers) / sizeof(m_setup_opcode_handlers[0]);
  init_params.p_args = p_setup_server;
  init_params.publish_timeout_cb = handle_publish_timeout;
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Opcode count: %d, 0x%04x, 0x%04x\n", init_params.opcode_count, m_setup_opcode_handlers[0], m_setup_opcode_handlers[2]);

  uint32_t status = access_model_add(&init_params, &p_setup_server->model_handle);
  if (status == NRF_SUCCESS)
  {
      status = access_model_subscription_list_alloc(p_setup_server->model_handle);
  }
  return status;
}

//  Funksjon for å sende cadence-melding
uint32_t simple_sensor_setup_server_cadence_status_publish(simple_sensor_setup_server_t* p_setup_server, uint16_t sensor_id)
{
  access_message_tx_t message;
  message.p_buffer = p_setup_server->p_callbacks->cadence_get(p_setup_server, sensor_id);
  message.length = (sensor_id == NULL) ? NUMBER_OF_SENSORS * sizeof(simple_sensor_cadence_setting_t) : sizeof(simple_sensor_cadence_setting_t);
  message.opcode.opcode = SIMPLE_SENSOR_OPCODE_CADENCE_STATUS;
  message.opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  message.force_segmented = false;
  message.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
  message.access_token = nrf_mesh_unique_token_get();
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nSending cadence set message\nLength: %d\nSensor ID: 0x%04x\n", message.length, message.p_buffer[0]);
  return access_model_publish(p_setup_server->model_handle, &message);
}

//  Sender en setting-melding
uint32_t simple_sensor_setup_server_setting_status_publish(simple_sensor_setup_server_t* p_setup_server, uint16_t sensor_id)
{
  access_message_tx_t message;
  message.p_buffer = p_setup_server->p_callbacks->setting_get(p_setup_server, sensor_id);
  message.length = sizeof(simple_sensor_setting_t);
  message.force_segmented = false;
  message.opcode.opcode = SIMPLE_SENSOR_OPCODE_SETTING_STATUS;
  message.opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  message.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
  message.access_token = nrf_mesh_unique_token_get();
  return access_model_publish(p_setup_server->model_handle, &message);
}

//  Sender en settings-melding, lik som setting, men inneholder alle innstillinger
uint32_t simple_sensor_setup_server_settings_status_publish(simple_sensor_setup_server_t* p_setup_server)
{
  access_message_tx_t message;
  message.p_buffer = p_setup_server->p_callbacks->setting_get(p_setup_server, 0);
  message.length = NUMBER_OF_SENSORS * sizeof(simple_sensor_setting_t);
  message.force_segmented = false;
  message.opcode.opcode = SIMPLE_SENSOR_OPCODE_SETTINGS_STATUS;
  message.opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  message.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
  message.access_token = nrf_mesh_unique_token_get();
  return access_model_publish(p_setup_server->model_handle, &message);
}

