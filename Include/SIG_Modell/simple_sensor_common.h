#ifndef SIMPLE_SENSOR_COMMON_H
#define SIMPLE_SENSOR_COMMON_H
#include "access.h"
#include <stdint.h>

#define SIMPLE_SENSOR_COMPANY_ID (0xFFFF)

#define NUMBER_OF_SENSORS 3
#define NUMBER_OF_COLUMNS 10

//  Operasjonskoder brukt til å tyde mesh-meldinger
typedef enum
{
  SIMPLE_SENSOR_OPCODE_DESCRIPTOR_GET = 0x8230,
  SIMPLE_SENSOR_OPCODE_DESCRIPTOR_STATUS = 0x51,
  SIMPLE_SENSOR_OPCODE_GET = 0x8231,
  SIMPLE_SENSOR_OPCODE_STATUS = 0x52,
  SIMPLE_SENSOR_OPCODE_COLUMN_GET = 0x8232,
  SIMPLE_SENSOR_OPCODE_COLUMN_STATUS = 0x53,
  SIMPLE_SENSOR_OPCODE_SERIES_GET = 0x8233,
  SIMPLE_SENSOR_OPCODE_SERIES_STATUS = 0x54,
  SIMPLE_SENSOR_OPCODE_CADENCE_GET = 0x8234,
  SIMPLE_SENSOR_OPCODE_CADENCE_SET = 0x55,
  SIMPLE_SENSOR_OPCODE_CADENCE_SET_UNRELIABLE = 0x56,
  SIMPLE_SENSOR_OPCODE_CADENCE_STATUS = 0x57,
  SIMPLE_SENSOR_OPCODE_SETTINGS_GET = 0x8235,
  SIMPLE_SENSOR_OPCODE_SETTINGS_STATUS = 0x58,
  SIMPLE_SENSOR_OPCODE_SETTING_GET = 0x8236,
  SIMPLE_SENSOR_OPCODE_SETTING_SET = 0x59,
  SIMPLE_SENSOR_OPCODE_SETTING_SET_UNRELIABLE = 0x5A,
  SIMPLE_SENSOR_OPCODE_SETTING_STATUS = 0x5B


} simple_sensor_opcode_t;

// Forskjellige typer målinger som kan beskrives i sensor-beskrivelsen
//  I vårt system blir disse ikke benyttet
typedef enum
{
  SIMPLE_SENSOR_SAMPLING_UNSPECIFIED = 0x00,
  SIMPLE_SENSOR_SAMPLING_INSTANTANEOUS = 0x01,
  SIMPLE_SENSOR_SAMPLING_ARITHMETIC_MEAN = 0x02,
  SIMPLE_SENSOR_SAMPLING_RMS = 0x03,
  SIMPLE_SENSOR_SAMPLING_MAXIMUM = 0x04,
  SIMPLE_SENSOR_SAMPLING_MINIMUM = 0x05,
  SIMPLE_SENSOR_SAMPLING_ACCUMULATED = 0x06,
  SIMPLE_SENSOR_SAMPLING_COUNT = 0x07

} simple_sensor_sampling_function_t;

//  En beskrivelsesmelding
typedef struct
{
  uint16_t simple_sensor_property_id;
  uint16_t simple_sensor_positive_tolerance;
  uint16_t simple_sensor_negative_tolerance;
  uint8_t simple_sensor_sampling_function;
  uint8_t simple_sensor_measurement_period;
  uint8_t simple_sensor_update_interval;

} simple_sensor_descriptor_t; 

//  En innstillingsmelding
typedef struct
{
  uint16_t simple_sensor_property_id;
  uint16_t simple_sensor_setting_id;
  uint8_t simple_sensor_setting_access;
  uint16_t simple_sensor_setting_raw;

} simple_sensor_setting_t;

//  En "Cadence"-melding
typedef struct
{
  uint16_t simple_sensor_property_id;
  uint8_t simple_sensor_fast_cadence_period;
  uint16_t simple_sensor_status_trigger_type;
  uint16_t simple_sensor_status_trigger_delta_down;
  uint16_t simple_sensor_status_trigger_delta_up;
  uint8_t simple_sensor_status_min_interval;
  uint16_t simple_sensor_fast_cadence_low;
  uint16_t simple_sensor_fast_cadence_high;

} simple_sensor_cadence_setting_t;

//  Strukt som inneholder sensor-data og ID
typedef struct
{
  uint16_t value;
  uint16_t id;

} simple_sensor_data_t;

// Statusmelding, inneholder data til alle sensorer på noden
typedef struct
{
  simple_sensor_data_t sensor[NUMBER_OF_SENSORS];

} simple_sensor_msg_status_t;

//  Kolonne-strukt
typedef struct
{
  uint16_t id;
  uint16_t x;
  uint16_t width;
  uint16_t y;

} simple_sensor_column_t;

//  kolonne for alle sensorene på noden
typedef struct
{
  simple_sensor_column_t sensor_columns[NUMBER_OF_COLUMNS];

} simple_sensor_series_column_t;

//  Kolonne-melding
typedef struct
{
  uint16_t sensor_id;
  uint8_t x_start;
  uint8_t x_end;

} simple_sensor_series_get_t;


#endif