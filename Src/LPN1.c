#include "LPN1.h"

#include "screen_config.h"
#include "nrf_gfx.h"
#include "waveshare_epd.h"
#include "client_data.h"
#include "server_data.h"
#include "simple_sensor_client.h"
#include "scanner.h"
#include "screens.h"
#include "log.h"
#include "BME280.h"
#include "HDC1080.h"
#include "TSL2581.h"
#include "si7021.h"
#include "CCS811.h"
#include "BMP180.h"
#include "app_timer.h"
#include "twi_sensor.h"
#include "bat_saadc.h"
#include "nrf_drv_saadc.h"

extern const nrf_lcd_t lcd;
APP_TIMER_DEF(reliable_transfer_time);

extern const nrf_gfx_font_desc_t orkney_8ptFontInfo;
extern const nrf_gfx_font_desc_t orkney_24ptFontInfo;
extern const nrf_gfx_font_desc_t arial_48ptFontInfo;
extern const nrf_gfx_font_desc_t arial_16ptFontInfo;

extern const uint16_t local_node_basic[];          // Local screen background         128 x 296
extern const uint16_t nabo_node_basic[];           // Neighbor screen background      128 x 296
extern const uint16_t nabo_node_demo[];            // Neighbor demo                   128 x 296

extern const uint16_t mini_local_node_basic[];     // Local screen background         200 x 200
extern const uint16_t mini_nabo_node_basic[];      // Neighbor screen background      200 x 200
extern const uint16_t mini_nabo_node_demo[];       // Neighbor demo                   200 x 200

extern sensors_t LPN_client[3];

uint16_t prev_sensor_1_data = 0;
uint16_t prev_sensor_2_data = 0;
uint16_t prev_sensor_3_data = 0;
uint8_t updated_lpn_id = 0;


//  Blir brukt til å opdpatere sensor-data og skjermen
//  Sensor-data lagres i en sensor-strukt som ligger i client_data.c
bool update_data(bool force)
{
  updated_lpn_id = LPN - 1;
  uint16_t raw[NUMBER_OF_SENSORS] = {0};
  double calc[NUMBER_OF_SENSORS] = {0};
  bool changed = false;
  
  read_sensors(raw, calc);

  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Temp: 0x%04x, %d, humid: 0x%04x, %d\n", raw[0], HDC_calc_temp(raw[0]), raw[1], HDC_calc_humid(raw[1]));
  
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Temp: %d, prev_temp: %d\n", (uint16_t)calc[TEMP_SENSOR - 1], prev_temp);
  if((uint16_t)calc[SENSOR_1] != prev_sensor_1_data)
  {
    changed = true;
    prev_sensor_1_data = (uint16_t)calc[SENSOR_1];
    sensor_set_temp(raw[SENSOR_1]);
  }

  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Humid: %d, prev_hmuid: %d\n", (uint16_t)calc[HUMID_SENSOR - 1], prev_humid);
  if((uint16_t)calc[SENSOR_2] != prev_sensor_2_data)
  {
    changed = true;
    prev_sensor_2_data = (uint16_t)calc[SENSOR_2];
    sensor_set_humid(raw[SENSOR_2]);
  }

  if((uint16_t)calc[SENSOR_3] != prev_sensor_3_data)
  {
    changed = true;
    prev_sensor_3_data = (uint16_t)calc[SENSOR_3];
    sensor_set_light(raw[SENSOR_3]);
  }

  sensor_set_temp_column(raw[SENSOR_1]);
  if(screen_change(updated_lpn_id) || force)
  {
    print_sensor_values(false);
  }

  return changed;
}

//  Sender oppdatert data ut i nettverket
void send_data(simple_sensor_server_t* p_server)
{
  simple_sensor_server_status_publish(p_server, (LPN << 8) & 0xFF00);
}

//  Sjekker om data er ny fra gammel data og sender ut i nettverket
void update_and_send_data(simple_sensor_server_t* p_server)
{
  if(update_data(false))
  {
    send_data(p_server);
  }
}

//  Henter ekstern data hvis det ikke er den lokale noden og viser det på skjermen
void display_LPN_1_data(simple_sensor_client_t* p_client)
{
  if(LPN != LPN_ID_1)
  {
    updated_lpn_id = LPN_ID_1 - 1;
    scanner_enable();
    simple_sensor_client_status_get(p_client, LPN_ID_1, 0);

    app_timer_start(reliable_transfer_time, HAL_MS_TO_RTC_TICKS(GET_REQ_TIME), NULL);
  }

  else
  {
    update_data(false);
  }
}

//  Henter ekstern data hvis det ikke er den lokale noden og viser det på skjermen
void display_LPN_2_data(simple_sensor_client_t* p_client)
{
  if(LPN != LPN_ID_2)
  {
    updated_lpn_id = LPN_ID_2 - 1;
    scanner_enable();
    simple_sensor_client_status_get(p_client, LPN_ID_2, 0);
    //print_sensor_values();
    app_timer_start(reliable_transfer_time, HAL_MS_TO_RTC_TICKS(GET_REQ_TIME), NULL);
  }

  else
  {
    update_data(false);
  }
}

//  Henter ekstern data hvis det ikke er den lokale noden og viser det på skjermen
void display_LPN_3_data(simple_sensor_client_t* p_client)
{
  if(LPN != LPN_ID_3)
  {
    updated_lpn_id = LPN_ID_3 - 1;
    scanner_enable();
    simple_sensor_client_status_get(p_client, LPN_ID_3, 0);

    app_timer_start(reliable_transfer_time, HAL_MS_TO_RTC_TICKS(GET_REQ_TIME), NULL);
  }

  else
  {
    update_data(false);
  }
}

//  Funksjon for å lese sensor-data for de forskjellige sensorene
void read_sensors(uint16_t* raw, double* calc)
{
  enable_sensors();
  uint32_t err = saadc_init();
  if(err != NRF_SUCCESS)
  {
    __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Error adc %d\n", err);
  }
  nrf_drv_saadc_sample();
  nrf_delay_ms(100);
  twi_init();
#if LPN == LPN_ID_1
  si7021_get_temp_hum(raw, calc);
  raw[2] = get_LDR_data();
  calc[2] = calc_lux_level(raw[2]);

#elif LPN == LPN_ID_2
  BME280_get_temp_hum(raw, calc);
  raw[2] = get_LDR_data();
  calc[2] = calc_lux_level(raw[2]);

#elif LPN == LPN_ID_3
  TSL2581_read(&raw[2]);
  calc[2] = raw[2];
  HDC1080_get_temp_hum(raw, calc);
#endif
#if LPN == LPN_ID_1
#endif

  twi_uninit();
  disable_sensors();
  saadc_uninit();
}

//  Beregner data til SI-enheter, [C]
uint16_t calculate_sensor_1_data(uint16_t lpn_id)
{
  if(lpn_id == (LPN_ID_1 - 1))
  {
    return (uint16_t)si7021_calc_temp(LPN_client[lpn_id].status[SENSOR_1].value);
  }

  else if(lpn_id == (LPN_ID_2 - 1))
  {
    return (uint16_t)BME280_calculate_temp(LPN_client[lpn_id].status[SENSOR_1].value);
  }

  else if(lpn_id == (LPN_ID_3 - 1))
  {
    return (uint16_t)HDC_calc_temp(LPN_client[lpn_id].status[SENSOR_1].value);
  }
  return 0;
}

//  Beregner data til SI-enheter, [%]
uint16_t calculate_sensor_2_data(uint16_t lpn_id)
{
  if(lpn_id == LPN_ID_1 - 1)
  {
    return (uint16_t)si7021_calc_humid(LPN_client[lpn_id].status[SENSOR_2].value);
  }

  else if(lpn_id == LPN_ID_2 - 1)
  {
    return (uint16_t)BME280_calculate_hum(LPN_client[lpn_id].status[SENSOR_2].value);
  }

  else if(lpn_id == LPN_ID_3 - 1)
  {
    return (uint16_t)HDC_calc_humid(LPN_client[lpn_id].status[SENSOR_2].value);
  }
  return 0;
}

//  Beregner data til SI-enheter, [Lux]
uint16_t calculate_sensor_3_data(uint16_t lpn_id)
{
  if(lpn_id == LPN_ID_1 - 1)
  {
    return calc_lux_level(LPN_client[lpn_id].status[SENSOR_3].value);
  }

  else if(lpn_id == LPN_ID_2 - 1)
  {
    return calc_lux_level(LPN_client[lpn_id].status[SENSOR_3].value);
  }

  else if(lpn_id == LPN_ID_3 - 1)
  {
    return LPN_client[lpn_id].status[SENSOR_3].value;
  }
  return 0;
}

//  Beregner SI-enhet for kolonne-data
void calc_column_data(uint16_t* data)
{
#if LPN == LPN_ID_1
for(uint8_t i = 0; i < NUMBER_OF_COLUMNS; i++)
{
  data[i] = si7021_calc_temp(LPN_client[updated_lpn_id].series_column[0].sensor_columns[i].y);
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Data[%d] = %d\n", i, data[i]);
}

#elif LPN == LPN_ID_2
for(uint8_t i = 0; i < NUMBER_OF_COLUMNS; i++)
{
  data[i] = BME280_calculate_temp(LPN_client[updated_lpn_id].series_column[0].sensor_columns[i].y);
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Data[%d] = %d\n", i, data[i]);
}

#elif LPN == LPN_ID_3
for(uint8_t i = 0; i < NUMBER_OF_COLUMNS; i++)
{
  data[i] = HDC_calc_temp(LPN_client[updated_lpn_id].series_column[0].sensor_columns[i].y);
  //  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Data[%d] = %d, %d\n", i, data[i], LPN_client[updated_lpn_id].series_column[0].sensor_columns[i].y);
}
#endif
}

//  Finner maks-temperatur i kolonnen, blir brukt til å skalere grafen
uint8_t max_column_temp(uint16_t* data)
{
  uint8_t max_temp = 0;
  for(uint8_t i = 0; i < NUMBER_OF_COLUMNS; i++)
  {
    if(data[i] > max_temp)
    {
      max_temp = data[i];
    }
  }
  return max_temp;
}

//  Finner min-temperaturen i kolonnen, blir brukt til å skalere grafen
uint8_t min_column_temp(uint16_t* data)
{
  uint8_t min_temp = 99;
  for(uint8_t i = 0; i < NUMBER_OF_COLUMNS; i++)
  {
    if((data[i] < min_temp) && data[i] > 0)
    {
      min_temp = data[i];
    }
  }
  return min_temp;
}

//  Skriver sensor-verdier ut på skjermen
//  Sjekker hvilken skjerm som er koblet til og benytter delvis oppdatering hvis det er 1.54"-skjermen
void print_sensor_values(bool external)
{
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Value changed\n");
#if EPAPER_MODULE1_54
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Updating display\n");
  nrf_gfx_init(&lcd);
  clear_screen();
  if(external)
  {
    draw_background_external(updated_lpn_id + 1);
  }
  else
  {
    draw_background();
  }
  write_sensor_data(updated_lpn_id);
  display_partial(0, 0, WSEPD_WIDTH, WSEPD_HEIGHT);
  nrf_gfx_uninit(&lcd);

#else
  nrf_gfx_init(&lcd);
  if(external)
  {
    draw_background_external(updated_lpn_id + 1);
  }
  else
  {
    draw_background();
  }
  write_sensor_data(updated_lpn_id);
  nrf_gfx_display(&lcd);
  nrf_gfx_uninit(&lcd);
#endif
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Display updated\n");
}

//  Setter opp en teller som blir brukt for å gi noden tid til å hente ekstern data før skjermen oppdateres
void init_lpn()
{
  app_timer_create(&reliable_transfer_time, APP_TIMER_MODE_SINGLE_SHOT, print_extern_sensor_values);
  screens_init();
}

//  Tvinger en oppdatering av skjermen etter ekstern data er hentet
void print_extern_sensor_values()
{
  print_sensor_values(true);
}
