#include "sensors_client.h"
#include "simple_sensor_common.h"
#include "simple_sensor_client.h"
//#include "LPN_config.h"
//#include "LPN1.h"
#include "log.h"
#include "sensor_uart.h"

#define TEMP_SENSOR 0x01
#define HUMID_SENSOR 0x02
#define LIGHT_SENSOR 0x03
#define ALL_SENSORS NULL

sensors_client_t sensor_client;
//extern nrf_lcd_t lcd;

void sensor_client_status_cb(const simple_sensor_client_t * p_client, simple_sensor_msg_status_t* p_message, uint16_t length);
void sensor_client_descriptor_cb(const simple_sensor_client_t* p_client, simple_sensor_descriptor_t* p_message, uint16_t sensor_id);
void sensor_client_cadence_cb(const simple_sensor_client_t* p_client, simple_sensor_cadence_setting_t* p_message, uint16_t sensor_id);
void sensor_client_setting_cb(const simple_sensor_client_t* p_client, simple_sensor_setting_t* p_message, uint16_t sensor_id);

simple_sensor_client_callbacks_t sensor_client_cb = {
  .status_cb = sensor_client_status_cb,
  .descriptor_cb = sensor_client_descriptor_cb,
  .cadence_cb = sensor_client_cadence_cb,
  .setting_cb = sensor_client_setting_cb
};


void sensor_client_status_cb(const simple_sensor_client_t * p_client, simple_sensor_msg_status_t* p_message, uint16_t length)
{
  uint8_t size = length * sizeof(simple_sensor_data_t);
  uint16_t sensor_id = p_message->sensor[0].id & 0xFF;
  
  uint8_t buffer[7];
  buffer[0] = STATUS;
  buffer[5] = TRANSFER_DONE;
  buffer[6] = TRANSFER_DONE;

  memcpy(&sensor_client.status[sensor_id - 1], p_message, size);

  for(uint8_t i = 0; i < length; i++)
  {
    buffer[1] = sensor_client.status[sensor_id - 1 + i].id >> 8;
    buffer[2] = sensor_client.status[sensor_id - 1 + i].id & 0xFF;
    buffer[3] = sensor_client.status[sensor_id - 1 + i].value >> 8;
    buffer[4] = sensor_client.status[sensor_id - 1 + i].value & 0xFF;

    uart_send(buffer, sizeof(buffer));
  }

  print_status(p_message->sensor[0].id, length);
  
  

  print_status(sensor_id, length);

  #if LPN1
  //LPN1_print_status(&lcd, sensor_client.status[sensor_id - 1].value);
  #endif
}

void sensor_client_descriptor_cb(const simple_sensor_client_t* p_client, simple_sensor_descriptor_t* p_message, uint16_t length)
{
  uint8_t size = length * sizeof(simple_sensor_descriptor_t);
  uint16_t sensor_id = p_message->simple_sensor_property_id & 0xFF;

  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nDescriptor, ID: 0x%04x\n", sensor_id);

  memcpy(&sensor_client.descriptor[sensor_id - 1], p_message, size);

  print_descriptor(sensor_id, length);
}

void sensor_client_cadence_cb(const simple_sensor_client_t* p_client, simple_sensor_cadence_setting_t* p_message, uint16_t length)
{
  uint8_t size = length * sizeof(simple_sensor_cadence_setting_t);
  uint16_t sensor_id = p_message->simple_sensor_property_id & 0xFF;

  memcpy(&sensor_client.cadence[sensor_id - 1], p_message, size);
  
  print_cadende(sensor_id, length);
}

void sensor_client_setting_cb(const simple_sensor_client_t* p_client, simple_sensor_setting_t* p_message, uint16_t length)
{
  uint8_t size = length * sizeof(simple_sensor_setting_t);
  uint16_t sensor_id = p_message->simple_sensor_property_id & 0xFF;

  memcpy(&sensor_client.settings[sensor_id - 1], p_message, size);

  print_setting(p_message->simple_sensor_property_id, length);
}

void sensors_client_init(simple_sensor_client_t* p_client)
{
  p_client->p_callbacks = &sensor_client_cb;
  //lcd = screen;
}

void print_status(uint8_t id, uint16_t length)
{
  for(uint8_t i = 0; i < length; i++)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nSensor status\nSensor ID: 0x%04x\nSensor value: 0x%04x\n", 
                                        sensor_client.status[id - 1 + i].id, 
                                        sensor_client.status[id - 1 + i].value
                                        );
  }
}

void print_descriptor(uint8_t id, uint16_t length)
{
  for(uint16_t i = 0; i < length; i++)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nSensor descriptor\nSensor %d\nMeasurement period: %d\nNegative tolerance: %d\nPositive tolerance: %d\nProperty ID: %d\nSampling function: 0x%02x\nUpdate interval: %d\n", 
    id + i,
    sensor_client.descriptor[id - 1 + i].simple_sensor_measurement_period, 
    sensor_client.descriptor[id - 1 + i].simple_sensor_negative_tolerance, 
    sensor_client.descriptor[id - 1 + i].simple_sensor_positive_tolerance, 
    sensor_client.descriptor[id - 1 + i].simple_sensor_property_id,
    sensor_client.descriptor[id - 1 + i].simple_sensor_sampling_function,
    sensor_client.descriptor[id - 1 + i].simple_sensor_update_interval
    );
  }
}

void print_cadende(uint8_t id, uint16_t length)
{
    for(uint8_t i = 0; i < length; i++)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nSensor cadence\nProperty ID: 0x%04x\nFast cadence period: 0x%04x\nFast cadence high: 0x%04x\nFast cadence low: 0x%04x\nStatus min interval: 0x%04x\nTrigger delta down: 0x%04x\nTrigger delta up: 0x%04x\nTrigger type: 0x%04x\n",
                                        sensor_client.cadence[id - 1 + i].simple_sensor_property_id,
                                        sensor_client.cadence[id - 1 + i].simple_sensor_fast_cadence_period,
                                        sensor_client.cadence[id - 1 + i].simple_sensor_fast_cadence_high,
                                        sensor_client.cadence[id - 1 + i].simple_sensor_fast_cadence_low,
                                        sensor_client.cadence[id - 1 + i].simple_sensor_status_min_interval,
                                        sensor_client.cadence[id - 1 + i].simple_sensor_status_trigger_delta_down,
                                        sensor_client.cadence[id - 1 + i].simple_sensor_status_trigger_delta_up,
                                        sensor_client.cadence[id - 1 + i].simple_sensor_status_trigger_type
                                        );
  }
}

void print_setting(uint8_t id, uint16_t length)
{
    for(uint8_t i = 0; i < length; i++)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nSensor settings\nSensor ID: 0x%04x\nSettings access: 0x%04x\nSettings ID: 0x%04x\nSettings raw: 0x%04x\n",
                                        sensor_client.settings[id + i - 1].simple_sensor_property_id, 
                                        sensor_client.settings[id + i - 1].simple_sensor_setting_access,
                                        sensor_client.settings[id + i - 1].simple_sensor_setting_id,
                                        sensor_client.settings[id + i - 1].simple_sensor_setting_raw
                                        );
  }
}