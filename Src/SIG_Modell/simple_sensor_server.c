#include "simple_sensor_server.h"
//#include "simple_sensor_common.h"

#include <stdint.h>
#include <stddef.h>

#include "access.h"
#include "access_config.h"
#include "access_reliable.h"
#include "nrf_mesh_assert.h"
#include "log.h"


//  Funksjon for å svare på en STATUS_GET_OPCODE
static uint32_t reply_status(const simple_sensor_server_t* p_server, const access_message_rx_t* p_message, uint8_t* p_data)
{
  access_message_tx_t reply;
  reply.opcode.opcode = SIMPLE_SENSOR_OPCODE_STATUS;
  reply.opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  reply.p_buffer = p_data;
  reply.length = sizeof(simple_sensor_data_t);
  if(p_message->p_data[0] == NULL)
  {
    reply.length = sizeof(simple_sensor_msg_status_t);
  }
  reply.force_segmented = false;
  reply.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
  reply.access_token = nrf_mesh_unique_token_get();

  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Replying to status get %d\n", (uint16_t)p_message->p_data[0]);
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nReplying status\nData: [0x%02x, 0x%02x, 0x%02x, 0x%02x]\nLength: %d\n", p_data[0], p_data[1], p_data[2], p_data[3], sizeof(p_data));

  return access_model_reply(p_server->model_handle, p_message, &reply);
}

//  Funksjon for å svare på en DESCRIPTOR_GET_OPCODE
static uint32_t reply_descriptor(const simple_sensor_server_t* p_server, const access_message_rx_t* p_message, uint8_t* p_data)
{
  uint16_t length;
  if(p_message->p_data[0] == NULL)
  {
    length = NUMBER_OF_SENSORS * sizeof(simple_sensor_descriptor_t);
  }
  else
  {
    length = sizeof(simple_sensor_descriptor_t);
  }

  access_message_tx_t reply;
  reply.opcode.opcode = SIMPLE_SENSOR_OPCODE_DESCRIPTOR_STATUS;
  reply.opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  reply.p_buffer = p_data;
  reply.length = length;
  reply.force_segmented =false;
  reply.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
  reply.access_token = nrf_mesh_unique_token_get();

  return access_model_reply(p_server->model_handle, p_message, &reply);
}

//  Funksjon for å håndtere STATUS_OPCODE
static void handle_get_cb(access_model_handle_t handle, const access_message_rx_t* p_message, void* p_args)
{
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Handling get message\n");
  simple_sensor_server_t* p_server = p_args;
  if(p_server->p_callbacks->get_cb == NULL)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Callback get not declared\n");
  }
  
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "p_data = [%d, %d]\n", p_message->p_data[0], p_message->p_data[1]);
  uint16_t sensor_id = p_message->p_data[0] | (p_message->p_data[1] << 8);
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nReplying to status get\nSensor ID: 0x%04x\nLength: %d\n", sensor_id, p_message->length);
  uint32_t status = reply_status(p_server, p_message, p_server->p_callbacks->get_cb(p_server, sensor_id));
  if(status != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error replying status: %d\n", status);
  }
}

//  Funksjon for å håndtere SERIES_OPCODE
static void handle_series_get_cb(access_model_handle_t handle, const access_message_rx_t* p_message, void* p_args)
{
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Series get\n");
  simple_sensor_server_t* p_server = p_args;
  simple_sensor_series_get_t* series_get = (simple_sensor_series_get_t*)p_message->p_data;
  uint32_t status = reply_status(p_server, p_message, p_server->p_callbacks->get_series_cb(p_server, series_get));
  if(status != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error replying series: %d\n", status);
  }
}

//  Funksjon for å håndtere DESCRIPTOR_OPCODE
static void handle_descriptor_get_cb(access_model_handle_t handle, const access_message_rx_t* p_message, void* p_args)
{
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Descriptor get\n");
  simple_sensor_server_t* p_server = p_args;
  
  uint32_t status = reply_descriptor(p_server, p_message, p_server->p_callbacks->descriptor_get(p_server, p_message->p_data[0]));
  if(status != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error replying descriptor: %d\n", status);
  }
}

//  En liste med operasjonskoder og tilhørende callback-funksjoner
static const access_opcode_handler_t m_opcode_handlers[] =
{
  {ACCESS_OPCODE_VENDOR(SIMPLE_SENSOR_OPCODE_GET, SIMPLE_SENSOR_COMPANY_ID), handle_get_cb},
  {ACCESS_OPCODE_VENDOR(SIMPLE_SENSOR_OPCODE_SERIES_GET, SIMPLE_SENSOR_COMPANY_ID), handle_series_get_cb},
  {ACCESS_OPCODE_VENDOR(SIMPLE_SENSOR_OPCODE_DESCRIPTOR_GET, SIMPLE_SENSOR_COMPANY_ID), handle_descriptor_get_cb},
};

//  Denne kjører hvis det forekommer en timeout
static void handle_publish_timeout(access_model_handle_t handle, void* p_args)
{
  simple_sensor_server_t* p_server = p_args;
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\n\nReplying to status timed out\n\n");
  //(void)simple_sensor_server_status_publish(p_server, p_server->p_callbacks->get_cb(p_server, NULL));
}

//  Funksjon for å iverksette server-modellens
uint32_t simple_sensor_server_init(simple_sensor_server_t* p_server, uint16_t element_index)
{
  p_server->tid = 0;
  access_model_add_params_t init_params;
  init_params.element_index = element_index;
  init_params.model_id.model_id = SIMPLE_SENSOR_SERVER_MODEL_ID;
  init_params.model_id.company_id = SIMPLE_SENSOR_COMPANY_ID;
  init_params.p_opcode_handlers = &m_opcode_handlers[0];
  init_params.opcode_count = sizeof(m_opcode_handlers) / sizeof(m_opcode_handlers[0]);
  init_params.p_args = p_server;
  init_params.publish_timeout_cb = handle_publish_timeout;
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Opcode count: %d\n", init_params.opcode_count);
  uint32_t status = access_model_add(&init_params, &p_server->model_handle);
  if (status == NRF_SUCCESS)
  {
      status = access_model_subscription_list_alloc(p_server->model_handle);
  }
  return status;
}

//  Funksjon som blir brukt til å sende status-melding ut i nettverket
uint32_t simple_sensor_server_status_publish(simple_sensor_server_t* p_server, uint16_t sensor_id)
{
  simple_sensor_msg_status_t status;
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Sending status\nSensor ID: 0x%04x\nSensor value: 0x%04x\n", status.sensor[0].id, status.sensor[0].value);
  access_message_tx_t msg;
  msg.opcode.opcode = SIMPLE_SENSOR_OPCODE_STATUS;
  msg.opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  msg.p_buffer = p_server->p_callbacks->get_cb(p_server, sensor_id);
  msg.length = ((sensor_id & 0xFF) == NULL) ? sizeof(simple_sensor_msg_status_t) : sizeof(simple_sensor_data_t);
  msg.force_segmented = false;
  msg.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
  msg.access_token = nrf_mesh_unique_token_get();

  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nSending sensor status message\nLength: %d\nSensor ID: 0x%04x\n", msg.length, sensor_id);//msg.p_buffer[0] | (msg.p_buffer[1] << 8));

  return access_model_publish(p_server->model_handle, &msg);
}

//  Blir brukt til å sende series-melding
uint32_t simple_sensor_server_series_publish(simple_sensor_server_t* p_server, uint16_t sensor_id, uint8_t x_start, uint8_t x_end)
{
  simple_sensor_series_column_t series;
  simple_sensor_series_get_t series_get = {.sensor_id = sensor_id, .x_start = x_start, .x_end = x_end};
  access_message_tx_t message;
  message.opcode.opcode = SIMPLE_SENSOR_OPCODE_SERIES_STATUS;
  message.opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  message.p_buffer = p_server->p_callbacks->get_series_cb(p_server, &series_get);
  message.length = ((sensor_id & 0xFF) == NULL) ? sizeof(simple_sensor_series_column_t) : sizeof(simple_sensor_column_t);
  message.force_segmented = false;
  message.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
  message.access_token = nrf_mesh_unique_token_get();

  return access_model_publish(p_server->model_handle, &message);
}

//  Sender en beskrivelsesmelding
uint32_t simple_sensor_server_descriptor_publish(simple_sensor_server_t* p_server, uint16_t sensor_id)
{
  access_message_tx_t message;
  message.opcode.opcode = SIMPLE_SENSOR_OPCODE_DESCRIPTOR_STATUS;
  message.opcode.company_id = SIMPLE_SENSOR_COMPANY_ID;
  message.p_buffer = p_server->p_callbacks->descriptor_get(p_server, sensor_id);
  message.length = (sensor_id == NULL) ? NUMBER_OF_SENSORS * sizeof(simple_sensor_descriptor_t) : sizeof(simple_sensor_descriptor_t);
  message.force_segmented = false;
  message.transmic_size = NRF_MESH_TRANSMIC_SIZE_DEFAULT;
  message.access_token = nrf_mesh_unique_token_get();

  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nSending sensor descriptor message\nLength: %d\nSensor ID: 0x%04x\n", message.length, message.p_buffer[0]);

  return access_model_publish(p_server->model_handle, &message);
}

