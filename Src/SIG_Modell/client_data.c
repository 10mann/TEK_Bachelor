#include "client_data.h"
#include "simple_sensor_common.h"
#include "simple_sensor_client.h"
#include "LPN_config.h"
#include "log.h"

#define ALL_SENSORS NULL

sensors_client_t LPN_client[3];


void sensor_client_status_cb(const simple_sensor_client_t* p_client, simple_sensor_msg_status_t* p_message, uint16_t length);
void sensor_client_series_cb(const simple_sensor_client_t* p_client, simple_sensor_series_column_t* p_series_get, uint16_t length);
void sensor_client_descriptor_cb(const simple_sensor_client_t* p_client, simple_sensor_descriptor_t* p_message, uint16_t sensor_id);
void sensor_client_cadence_cb(const simple_sensor_client_t* p_client, simple_sensor_cadence_setting_t* p_message, uint16_t sensor_id);
void sensor_client_setting_cb(const simple_sensor_client_t* p_client, simple_sensor_setting_t* p_message, uint16_t sensor_id);

simple_sensor_client_callbacks_t sensor_client_cb = {
  .status_cb = sensor_client_status_cb,
  .series_cb = sensor_client_series_cb,
  .descriptor_cb = sensor_client_descriptor_cb,
  .cadence_cb = sensor_client_cadence_cb,
  .setting_cb = sensor_client_setting_cb
};


void sensor_client_status_cb(const simple_sensor_client_t * p_client, simple_sensor_msg_status_t* p_message, uint16_t length)
{
  uint8_t size = length * sizeof(simple_sensor_data_t);
  uint16_t sensor_id = p_message->sensor[0].id & 0xFF;
  if(sensor_id != 0)
    sensor_id--;
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Writing to sensor_id: 0x%04x\n", p_message->sensor[0].id);

  uint16_t lpn_id = (p_message->sensor[0].id >> 8);
  if(lpn_id == 0)
    lpn_id = LPN;
  lpn_id--;

  memcpy(&LPN_client[lpn_id].status[sensor_id], p_message, size);

  print_status(lpn_id, sensor_id, length);

}

//  Funksjon for å håndtere series-meldinger, lagrer data lokalt i en sensor-strukt via memcpy
void sensor_client_series_cb(const simple_sensor_client_t* p_client, simple_sensor_series_column_t* p_series_get, uint16_t length)
{
  uint8_t size = length * sizeof(simple_sensor_series_column_t);
  uint8_t sensor_id = p_series_get->sensor_columns[0].id & 0xFF;
  if(sensor_id == 0)
    sensor_id = SENSOR_1;
  else
    sensor_id--;

  uint8_t lpn_id = p_series_get->sensor_columns[0].id >> 8;
  if(lpn_id == 0)
    lpn_id = LPN;
  lpn_id--;

  memcpy(&LPN_client[lpn_id].series_column[sensor_id].sensor_columns[p_series_get->sensor_columns->x], p_series_get, size);

  print_series(lpn_id, sensor_id, p_series_get->sensor_columns->x, length);
}

//  Funksjon for å håndtere descriptor-meldinger, lagrer data lokalt i en sensor-strukt via memcpy
void sensor_client_descriptor_cb(const simple_sensor_client_t* p_client, simple_sensor_descriptor_t* p_message, uint16_t length)
{
  uint8_t size = length * sizeof(simple_sensor_descriptor_t);
  uint16_t sensor_id = p_message->simple_sensor_property_id & 0xFF;
  uint16_t lpn_id = (p_message->simple_sensor_property_id >> 8) && 0xFF;

  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nDescriptor, ID: 0x%04x\n", sensor_id);

  memcpy(&LPN_client[lpn_id].descriptor[sensor_id - 1], p_message, size);

  print_descriptor(lpn_id, sensor_id, length);
}

//  Funksjon for å håndtere cadence-meldinger, lagrer data lokalt i en sensor-strukt via memcpy
void sensor_client_cadence_cb(const simple_sensor_client_t* p_client, simple_sensor_cadence_setting_t* p_message, uint16_t length)
{
  uint8_t size = length * sizeof(simple_sensor_cadence_setting_t);
  uint16_t sensor_id = p_message->simple_sensor_property_id & 0xFF;
  uint16_t lpn_id = (p_message->simple_sensor_property_id >> 8) && 0xFF;

  memcpy(&LPN_client[lpn_id].cadence[sensor_id - 1], p_message, size);
  
  print_cadende(lpn_id, sensor_id, length);
}

//  Funksjon for å håndtere setting-meldinger, lagrer data lokalt i en sensor-strukt via memcpy
void sensor_client_setting_cb(const simple_sensor_client_t* p_client, simple_sensor_setting_t* p_message, uint16_t length)
{
  uint8_t size = length * sizeof(simple_sensor_setting_t);
  uint16_t sensor_id = p_message->simple_sensor_property_id & 0xFF;
  uint16_t lpn_id = (p_message->simple_sensor_property_id >> 8) && 0xFF;

  memcpy(&LPN_client[lpn_id].settings[sensor_id - 1], p_message, size);

  print_setting(lpn_id, sensor_id, length);
}

//  Setter riktig callback-funksjoner for client-modellen
void sensors_client_init(simple_sensor_client_t* p_client)
{
  p_client->p_callbacks = &sensor_client_cb;
}

//  Skriver ut mottatt sensor-data til RTT-viewer
void print_status(uint8_t lpn_id, uint8_t id, uint8_t length)
{
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Length: %d\n", length);
  for(uint8_t i = 0; i < length; i++)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nIndex: %d\n", i);
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nSensor status\nSensor ID: 0x%04x\nSensor value: 0x%04x\n", 
                                        LPN_client[lpn_id].status[id + i].id, 
                                        LPN_client[lpn_id].status[id + i].value
                                        );
  }
}

//  Skriver ut mottatt data til RTT-viewer
void print_series(uint8_t lpn_id, uint8_t sensor_id, uint8_t col_id, uint8_t length)
{
  for(uint8_t i = 0; i < length; i++)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nSensor series\nSensor ID: 0x%04x\n", LPN_client[lpn_id].series_column[sensor_id].sensor_columns[i].id);
    for(uint8_t j = 0; j < NUMBER_OF_COLUMNS; j++)
    {
      __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nCol %d\nColumn x: %d\nColumn width: %d\nColumn height: %d\n", LPN_client[lpn_id].series_column[sensor_id].sensor_columns[j].x, LPN_client[lpn_id].series_column[sensor_id].sensor_columns[j].width, LPN_client[lpn_id].series_column[sensor_id].sensor_columns[j].y);
    }
  }
}

//  Skriver ut mottatt data til RTT-viewer
void print_descriptor(uint8_t lpn_id, uint8_t id, uint8_t length)
{
  for(uint16_t i = 0; i < length; i++)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nSensor descriptor\nSensor %d\nMeasurement period: %d\nNegative tolerance: %d\nPositive tolerance: %d\nProperty ID: %d\nSampling function: 0x%02x\nUpdate interval: %d\n", 
    id + i,
    LPN_client[lpn_id].descriptor[id - 1 + i].simple_sensor_measurement_period, 
    LPN_client[lpn_id].descriptor[id - 1 + i].simple_sensor_negative_tolerance, 
    LPN_client[lpn_id].descriptor[id - 1 + i].simple_sensor_positive_tolerance, 
    LPN_client[lpn_id].descriptor[id - 1 + i].simple_sensor_property_id,
    LPN_client[lpn_id].descriptor[id - 1 + i].simple_sensor_sampling_function,
    LPN_client[lpn_id].descriptor[id - 1 + i].simple_sensor_update_interval
    );
  }
}

//  Skriver ut mottatt data til RTT-viewer
void print_cadende(uint8_t lpn_id, uint8_t id, uint8_t length)
{
    for(uint8_t i = 0; i < length; i++)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nSensor cadence\nProperty ID: 0x%04x\nFast cadence period: 0x%04x\nFast cadence high: 0x%04x\nFast cadence low: 0x%04x\nStatus min interval: 0x%04x\nTrigger delta down: 0x%04x\nTrigger delta up: 0x%04x\nTrigger type: 0x%04x\n",
                                        LPN_client[lpn_id].cadence[id - 1 + i].simple_sensor_property_id,
                                        LPN_client[lpn_id].cadence[id - 1 + i].simple_sensor_fast_cadence_period,
                                        LPN_client[lpn_id].cadence[id - 1 + i].simple_sensor_fast_cadence_high,
                                        LPN_client[lpn_id].cadence[id - 1 + i].simple_sensor_fast_cadence_low,
                                        LPN_client[lpn_id].cadence[id - 1 + i].simple_sensor_status_min_interval,
                                        LPN_client[lpn_id].cadence[id - 1 + i].simple_sensor_status_trigger_delta_down,
                                        LPN_client[lpn_id].cadence[id - 1 + i].simple_sensor_status_trigger_delta_up,
                                        LPN_client[lpn_id].cadence[id - 1 + i].simple_sensor_status_trigger_type
                                        );
  }
}

//  Skriver ut mottatt data til RTT-viewer
void print_setting(uint8_t lpn_id, uint8_t id, uint8_t length)
{
    for(uint8_t i = 0; i < length; i++)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\nSensor settings\nSensor ID: 0x%04x\nSettings access: 0x%04x\nSettings ID: 0x%04x\nSettings raw: 0x%04x\n",
                                        LPN_client[lpn_id].settings[id + i - 1].simple_sensor_property_id, 
                                        LPN_client[lpn_id].settings[id + i - 1].simple_sensor_setting_access,
                                        LPN_client[lpn_id].settings[id + i - 1].simple_sensor_setting_id,
                                        LPN_client[lpn_id].settings[id + i - 1].simple_sensor_setting_raw
                                        );
  }
}