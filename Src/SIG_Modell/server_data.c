#include "server_data.h"
#include "simple_sensor_server.h"
#include "simple_sensor_setup_server.h"
#include "log.h"
#include "LPN_config.h"

sensors_t sensor;

extern sensors_t LPN_client[3];

uint8_t temp_column_index = 0;
uint8_t humid_column_index = 0;
uint8_t light_column_index = 0;

uint8_t* sensor_server_get_cb(const simple_sensor_server_t* p_server, uint16_t sensor_id);
uint8_t* sensor_server_get_series_cb(const simple_sensor_server_t* p_server, simple_sensor_series_get_t* series_get);
uint8_t* sensor_server_descriptor_get_cb(const simple_sensor_server_t* p_server, uint16_t sensor_id);
uint8_t* sensor_setup_server_cadence_get_cb(const simple_sensor_setup_server_t* p_setup_server, uint16_t sensor_id);
uint8_t* sensor_setup_server_setting_get_cb(const simple_sensor_setup_server_t* p_setup_server, uint16_t sensor_id);

//  Callback-funksjoner til sensor-server-modellen
const simple_sensor_callbacks_t sensor_server_cb = 
{
  .get_cb = sensor_server_get_cb,
  .get_series_cb = sensor_server_get_series_cb,
  .descriptor_get = sensor_server_descriptor_get_cb
};

//  Callback-funksjoner til sensor-setup-server-modellen
const simple_sensor_setup_server_callbacks_t sensor_setup_server_cb = 
{
  .cadence_get = sensor_setup_server_cadence_get_cb,
  .setting_get = sensor_setup_server_setting_get_cb,
};

//  En funksjon for å returnere en peker til data som skal overføres i mesh-nettverket
uint8_t* sensor_server_get_cb(const simple_sensor_server_t* p_server, uint16_t sensor_id)
{
  uint8_t lpn_id = ((sensor_id >> 8));
  if(lpn_id == 0)
    lpn_id = LPN;
  lpn_id--;

  sensor_id &= 0xFF;
  if(sensor_id == 0)
    sensor_id = SENSOR_1;
  else
    sensor_id--;

  return (uint8_t*)&LPN_client[lpn_id].status[sensor_id];
}

//  En funksjon for å returnere en peker til data som skal overføres i mesh-nettverket
uint8_t* sensor_server_get_series_cb(const simple_sensor_server_t* p_server, simple_sensor_series_get_t* series_get)
{
  uint8_t lpn_id = (series_get->sensor_id >> 8);
  if(lpn_id == 0)
    lpn_id = LPN;
  lpn_id--;

  uint8_t sensor_id = series_get->sensor_id & 0xFF;
  if(sensor_id == 0)
    sensor_id = SENSOR_1;
  else
    sensor_id--;

  return (uint8_t*)&LPN_client[lpn_id].series_column[sensor_id].sensor_columns[series_get->x_start];
}

//  En funksjon for å returnere en peker til data som skal overføres i mesh-nettverket
uint8_t* sensor_server_descriptor_get_cb(const simple_sensor_server_t* p_server, uint16_t sensor_id)
{
  if(sensor_id != NULL)
  {
    return (uint8_t*)&LPN_client[LPN - 1].descriptor[sensor_id - 1];
  }
  else
  {
    return (uint8_t*)&LPN_client[LPN - 1].descriptor[0];
  }
}

//  En funksjon for å returnere en peker til data som skal overføres i mesh-nettverket
uint8_t* sensor_setup_server_cadence_get_cb(const simple_sensor_setup_server_t* p_setup_server, uint16_t sensor_id)
{
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Cadence get\n");
  if(sensor_id != NULL)
  {
    return (uint8_t*)&LPN_client[LPN - 1].cadence[sensor_id - 1];
  }
  else
  {
    return (uint8_t*)&LPN_client[LPN - 1].cadence[0];
  }
}

//  En funksjon for å returnere en peker til data som skal overføres i mesh-nettverket
uint8_t* sensor_setup_server_setting_get_cb(const simple_sensor_setup_server_t* p_setup_server, uint16_t sensor_id)
{
  if(sensor_id != NULL)
  {
    return (uint8_t*)&LPN_client[LPN - 1].settings[sensor_id - 1];
  }
  else
  {
    return (uint8_t*)&LPN_client[LPN - 1].settings[0];
  }
}

//  Setter opp startverdier for sensor-modellen
void sensors_server_init(simple_sensor_server_t* p_server, simple_sensor_setup_server_t* p_setup_server)
{
  for(int j = 0; j < 3; j++)
  {
    for(uint8_t i = 0; i < NUMBER_OF_SENSORS; i++)
    {

      LPN_client[j].status[i].id = ((j + 1) << 8) | (i + 1);
      //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "ID: 0x%04x\n", sensor.status[i].id);
      LPN_client[j].status[i].value = 0;

      LPN_client[j].descriptor[i].simple_sensor_sampling_function = SIMPLE_SENSOR_SAMPLING_INSTANTANEOUS;
      LPN_client[j].descriptor[i].simple_sensor_negative_tolerance = 0x0136;
      LPN_client[j].descriptor[i].simple_sensor_positive_tolerance = 0x0136;
      LPN_client[j].descriptor[i].simple_sensor_property_id = sensor.status[i].id;
      LPN_client[j].descriptor[i].simple_sensor_measurement_period = 0;
      LPN_client[j].descriptor[i].simple_sensor_update_interval = 0;

      LPN_client[j].cadence[i].simple_sensor_property_id = sensor.status[i].id;
      LPN_client[j].cadence[i].simple_sensor_fast_cadence_period = 0;
      LPN_client[j].cadence[i].simple_sensor_status_trigger_type = 0;
      LPN_client[j].cadence[i].simple_sensor_status_trigger_delta_down = 0;
      LPN_client[j].cadence[i].simple_sensor_status_trigger_delta_up = 0;
      LPN_client[j].cadence[i].simple_sensor_status_min_interval = 0;
      LPN_client[j].cadence[i].simple_sensor_fast_cadence_low = 0;
      LPN_client[j].cadence[i].simple_sensor_fast_cadence_high = 0;

      LPN_client[j].settings[i].simple_sensor_property_id = sensor.status[i].id;
      LPN_client[j].settings[i].simple_sensor_setting_access = 0;
      LPN_client[j].settings[i].simple_sensor_setting_id = 0;
      LPN_client[j].settings[i].simple_sensor_setting_raw = 0;

      for(int k = 0; k < NUMBER_OF_COLUMNS; k++)
      {
        LPN_client[j].series_column[i].sensor_columns[k].id = LPN_client[j].status[i].id;
        LPN_client[j].series_column[i].sensor_columns[k].width = 20;
        LPN_client[j].series_column[i].sensor_columns[k].x = k * LPN_client[j].series_column[i].sensor_columns[k].width + 5;
        LPN_client[j].series_column[i].sensor_columns[k].y = 0;
        //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "LPN %d, col %d, [%d, %d]\n", j, k, LPN_client[j].series_column[i].sensor_columns[k].x, LPN_client[j].series_column[i].sensor_columns[k].y);
      }

      //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Sensor: %d, ID: 0x%04x, value: 0x%04x\n", i, sensor.status[i].id, sensor.status[i].value);
    }

    //memcpy(&LPN_client[j], &sensor, sizeof(sensors_t));
  }

  p_server->p_callbacks = &sensor_server_cb;
  p_setup_server->p_callbacks = &sensor_setup_server_cb;
}

void sensor_set_temp(uint16_t temp)
{
  LPN_client[LPN - 1].status[SENSOR_1].value = temp;
}

void sensor_set_temp_column(uint16_t temp)
{
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Adding %d to column, index %d\n", temp, temp_column_index);
  uint8_t index = 0;
  if(temp_column_index >= NUMBER_OF_COLUMNS)
  {
    for(uint8_t i = 0; i < NUMBER_OF_COLUMNS - 1; i++)
    {
      LPN_client[LPN - 1].series_column[SENSOR_1].sensor_columns[i].y = LPN_client[LPN - 1].series_column[SENSOR_1].sensor_columns[i + 1].y;
    }
    LPN_client[LPN - 1].series_column[SENSOR_1].sensor_columns[NUMBER_OF_COLUMNS - 1].y = temp;
  }

  else
  {
    LPN_client[LPN - 1].series_column[SENSOR_1].sensor_columns[temp_column_index++].y = temp;
  }
/*
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Temp column: [%d, %d, %d, %d, %d]\n",
                                      LPN_client[LPN - 1].series_column[TEMP_SENSOR - 1].sensor_columns[index++].y, 
                                      LPN_client[LPN - 1].series_column[TEMP_SENSOR - 1].sensor_columns[index++].y, 
                                      LPN_client[LPN - 1].series_column[TEMP_SENSOR - 1].sensor_columns[index++].y, 
                                      LPN_client[LPN - 1].series_column[TEMP_SENSOR - 1].sensor_columns[index++].y, 
                                      LPN_client[LPN - 1].series_column[TEMP_SENSOR - 1].sensor_columns[index++].y);
*/
}

void sensor_set_humid(uint16_t humid)
{
  LPN_client[LPN - 1].status[SENSOR_2].value = humid;
}

void sensor_set_light(uint16_t light)
{
  LPN_client[LPN - 1].status[SENSOR_3].value = light;
}

uint16_t sensor_get_temp()
{
  return LPN_client[LPN - 1].status[0].value;
}

uint16_t sensor_get_humid()
{
  return LPN_client[LPN - 1].status[1].value;
}