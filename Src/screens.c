#include "screens.h"
#include "nrf_gfx.h"
#include "screen_config.h"
#include "waveshare_epd.h"
#include "nrf_font.h"
#include "LPN1.h"
#include "server_data.h"
#include "log.h"
#include "nrf_delay.h"
#include "bat_saadc.h"
#include "app_timer.h"

APP_TIMER_DEF(graph_timeout);

extern nrf_lcd_t lcd;
extern sensors_t LPN_client[3];

extern const nrf_gfx_font_desc_t orkney_8ptFontInfo;
extern const nrf_gfx_font_desc_t orkney_24ptFontInfo;
extern const nrf_gfx_font_desc_t arial_48ptFontInfo;
extern const nrf_gfx_font_desc_t arial_16ptFontInfo;
//  Batteri 
extern uint8_t white_battery_00[];
extern uint8_t white_battery_20[];
extern uint8_t white_battery_50[];
extern uint8_t white_battery_80[];
extern uint8_t white_battery_100[];
extern uint8_t black_battery_100[];
//  Bakgrunn  
extern uint8_t nabo_node_demo[];
extern uint8_t nabo_node_basic[];
extern uint8_t mini_nabo_node_demo[];
extern uint8_t local_node_basic[];
extern uint8_t mini_local_node_basic[];
//  BLE 
extern uint8_t BLE_connected_black[];
extern uint8_t BLE_connected_white[];
extern uint8_t BLE_disconnected_white[];
// Intervall  
extern uint8_t timer_1_black[];
extern uint8_t timer_2_black[];
extern uint8_t timer_3_black[];
extern uint8_t timer_1_white[];
extern uint8_t timer_2_white[];
extern uint8_t timer_3_white[];
//  Lys 
extern uint8_t bulb[];
extern uint8_t sol[];
extern uint8_t dark[];

uint16_t curr_sensor_1_data = 0;
uint16_t curr_sensor_2_data = 0;
uint16_t curr_sensor_3_data = 0;
uint16_t curr_battery_level = BATT_LEVEL_0;
bool curr_provisioned = false;
uint16_t curr_time_mode = LPN_INTERVAL_MODE_2;
uint8_t light_level = 0;
bool graph = false;

//  Skriver bakgrunnsbilde til skjerm-bufferet, skriver også andre ikoner som skal vises
void draw_background()
{
  #if EPAPER_MODULE2_9
  draw_bitmap(0, 0, WSEPD_WIDTH, WSEPD_HEIGHT, local_node_basic);
  #else
  draw_bitmap(0, 0, WSEPD_WIDTH, WSEPD_HEIGHT, mini_local_node_basic);
  #endif
  print_lpn_id();
  draw_battery();
  draw_bt_icon();
  draw_time_interval();
  draw_light_level();
}

//  Tegner bakgrunn for ekstern node, den er lik som lokal, 
void draw_background_external(uint16_t lpn_id)
{
  #if EPAPER_MODULE2_9
  draw_bitmap(0, 0, WSEPD_WIDTH, WSEPD_HEIGHT, local_node_basic);
  //nrf_gfx_background_set(&lcd, local_node_basic);
  #else
  draw_bitmap(0, 0, WSEPD_WIDTH, WSEPD_HEIGHT, mini_local_node_basic);
  //nrf_gfx_background_set(&lcd, mini_local_node_basic);
  #endif
  print_external_lpn_id(lpn_id);
}

//  En funksjon for å finne piksel-lengden på en streng
uint16_t string_to_pixel_length(const nrf_gfx_font_desc_t* p_font, uint8_t* string)
{
  uint16_t length = 0;
  uint16_t space = 0;
  uint16_t char_len = 0;

  for(int i = 0; string[i] != '\0'; i++)
  {
    space = p_font->spacePixels * 2;
    char_len = p_font->charInfo[string[i] - p_font->startChar].widthBits;
    if(char_len > p_font->height)
      char_len = p_font->charInfo['1' - p_font->startChar].widthBits;
    length += space;
    length += char_len;
  }
  return length;
}

//  Skriver hvilken LPN man ser på
void print_lpn_id()
{
  const nrf_gfx_font_desc_t* p_font = &orkney_8ptFontInfo;
  char string[6];
  snprintf(string, sizeof(string), "LPN %d\n", LPN);
  nrf_gfx_point_t text_pt = {.y = WSEPD_HEIGHT / 2 - string_to_pixel_length(p_font, string) / 2, .x = 5};
  nrf_gfx_print(&lcd, &text_pt, 0x0000, string, p_font, false);
}

//  Skriver LPN ID til ekstern LPN, inverterte farger fra lokal LPN og med en svart firkant for å vise at data er esktern
void print_external_lpn_id(uint16_t lpn_id)
{
  const nrf_gfx_font_desc_t* p_font = &orkney_8ptFontInfo;
  char string[6];
  nrf_gfx_rect_t rect = {.x = 0, .y = 0, .width = 25, .height = WSEPD_HEIGHT};
  snprintf(string, sizeof(string), "LPN %d\n", lpn_id);
  nrf_gfx_point_t text_pt = {.y = WSEPD_HEIGHT / 2 - string_to_pixel_length(p_font, string) / 2, .x = 5};
  nrf_gfx_rect_draw(&lcd, &rect, 1, 0x0000, true);
  nrf_gfx_print(&lcd, &text_pt, 0xFFFF, string, p_font, false);
}

//  Skriver sensordata ut på skjermen
void write_sensor_data(uint8_t lpn_id)
{
  curr_sensor_1_data = calculate_sensor_1_data(lpn_id);
  curr_sensor_2_data = calculate_sensor_2_data(lpn_id);
  curr_sensor_3_data = calculate_sensor_3_data(lpn_id); 
  char sensor_1_s[3];
  nrf_gfx_point_t temp_pt;
  const nrf_gfx_font_desc_t* p_temp_font = &arial_48ptFontInfo;
  snprintf(sensor_1_s, sizeof(sensor_1_s), "%d\n", curr_sensor_1_data);
  temp_pt.x = TEMP_PT_X; // 124 + p_temp_font->height / 10;
  temp_pt.y = TEMP_PT_Y - string_to_pixel_length(p_temp_font, sensor_1_s); // 130 - string_to_pixel_length(p_temp_font, temp_s);

  char sensor_2_s[3];
  nrf_gfx_point_t humid_pt;
  const nrf_gfx_font_desc_t* p_humid_font = &orkney_24ptFontInfo;
  snprintf(sensor_2_s, sizeof(sensor_2_s), "%d\n", curr_sensor_2_data);
  humid_pt.x = HUMID_PT_X; // + p_humid_font->height / 10;
  humid_pt.y = HUMID_PT_Y - string_to_pixel_length(p_humid_font, sensor_2_s); //124

  char sensor_3_s[6];
  nrf_gfx_point_t light_pt;
  const nrf_gfx_font_desc_t* p_light_font = &arial_16ptFontInfo;
  snprintf(sensor_3_s, sizeof(sensor_3_s), "%d\n", curr_sensor_3_data);
  light_pt.x = LIGHT_PT_X; // 178 + p_light_font->height / 10;
  light_pt.y = LIGHT_PT_Y - string_to_pixel_length(p_light_font, sensor_3_s);

  nrf_gfx_print(&lcd, &temp_pt, 0x0000, sensor_1_s, p_temp_font, false);
  nrf_gfx_print(&lcd, &humid_pt, 0x0000, sensor_2_s, p_humid_font, false);
  nrf_gfx_print(&lcd, &light_pt, 0x0000, sensor_3_s, p_light_font, false);
}

// En funksjon som returnere 1 hvis det er en endring på skjermen og at skjermen skal oppdateres
bool screen_change(uint8_t lpn_id)
{
  if(curr_sensor_1_data != calculate_sensor_1_data(lpn_id))
    return true;
  else if(curr_sensor_2_data != calculate_sensor_2_data(lpn_id))
    return true;
  else if(curr_sensor_3_data != calculate_sensor_3_data(lpn_id))
    return true;
  else if(curr_battery_level != get_batt_level())
    return true;
  else if(curr_provisioned != device_is_proviosioned())
    return true;
  else if(curr_time_mode != get_LPN_interval_mode())
    return true;
  else
    return false;
}

//  En funksjon som tegner en graf over de ti siste temperaturmålingene
//  Tegned punkter og streker mellom punktene
//  Øvre og nedre grense justerer seg etter hvor mye temperaturen er spredt
void draw_temp_graph()
{
  graph = true;
  uint16_t temp[NUMBER_OF_COLUMNS];
  calc_column_data(temp);
  uint8_t max_temp = max_column_temp(temp);
  if(max_temp > GRAPH_MAX_TEMP)
    max_temp = GRAPH_MAX_TEMP;
  uint8_t min_temp = min_column_temp(temp) - 1;
  if(min_temp < GRAPH_MIN_TEMP)
    min_temp = GRAPH_MIN_TEMP;
  uint8_t width = (WSEPD_HEIGHT - GRAPH_START) / NUMBER_OF_COLUMNS;
  nrf_gfx_line_t x_axis = {.thickness = 2, .x_start = 0, .x_end = WSEPD_WIDTH, .y_start = GRAPH_START, .y_end = GRAPH_START};
  uint8_t temp_space = (GRAPH_TOP - GRAPH_BOTTOM) / (GRAPH_LINES - 1);
  uint8_t temp_diff = (max_temp - min_temp) / (GRAPH_LINES - 1) + 1;
  uint8_t max_temp_shown_temp = min_temp + temp_diff * (GRAPH_LINES - 1);
  const nrf_gfx_font_desc_t* p_temp_font = &arial_16ptFontInfo;
  nrf_gfx_point_t value_pt = {.x = 0, .y = 0};
  char temp_diff_s[3];
  char temp_s[3];
  __LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Max temp %d, max shown temp %d, temp diff %d\n", max_temp, max_temp_shown_temp, temp_diff);
  nrf_gfx_circle_t circle[NUMBER_OF_COLUMNS];
  nrf_gfx_init(&lcd);
#if EPAPER_MODULE1_54
  fill_screen();
  display_partial(0, 0, WSEPD_WIDTH, WSEPD_HEIGHT);
#endif
  
  nrf_gfx_line_draw(&lcd, &x_axis, 0x0000);

  for(uint8_t i = 0; i < GRAPH_LINES; i++)
  {
    value_pt.x = i * temp_space + GRAPH_BOTTOM;
    snprintf(temp_diff_s, sizeof(temp_diff_s), "%d\n", temp_diff * i + min_temp);
    nrf_gfx_print(&lcd, &value_pt, 0x0000, temp_diff_s, p_temp_font, false);
  }

  nrf_gfx_line_t pt_line;
  pt_line.thickness = 1;

  for(uint8_t i = 0; i < NUMBER_OF_COLUMNS; i++)
  {
    circle[i].r = 3;
    circle[i].x = (double)(temp[i] - min_temp) / (double)(max_temp_shown_temp - min_temp) * (double)GRAPH_TOP + GRAPH_BOTTOM + (double)p_temp_font->height / 2;
    circle[i].y = GRAPH_START + 5 + width * i;
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Circle.x = %d, Circle.y = %d, top = %d, max temp = %d\n", circle[i].x, circle[i].y, GRAPH_TOP, max_temp);
    if(temp[i] > 0 && circle[i].x < GRAPH_TOP)
    {
      if(i > 0)
      {
        pt_line.x_start = circle[i - 1].x;
        pt_line.x_end = circle[i].x;
        pt_line.y_start = circle[i - 1].y;
        pt_line.y_end = circle[i].y;
        //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Drawing line from %d, %d to %d, %d\n", pt_line.x_start, pt_line.y_start, pt_line.x_end, pt_line.y_end);
        nrf_gfx_line_draw(&lcd, &pt_line, 0x0000);
      }
      nrf_gfx_circle_draw(&lcd, &circle[i], 0x0000, true);
    }
  }

  //display_partial(0, 0, WSEPD_WIDTH, WSEPD_HEIGHT);
#if EPAPER_MODULE1_54
  display_partial(0, 0, WSEPD_WIDTH, WSEPD_HEIGHT);
#else
  nrf_gfx_display(&lcd);
#endif
  nrf_gfx_uninit(&lcd);

  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Starting countdown\n");
  //uint32_t time = HAL_SECS_TO_RTC_TICKS(10);
  app_timer_start(graph_timeout, GRAPH_TIME_ON_SCREEN, NULL);
}

//  Tegner et estimat på gjenstående batteriprosent, 100%, 80%, 50%, 20% og 0%
void draw_battery()
{
  curr_battery_level = get_batt_level();
  if(curr_battery_level == BATT_LEVEL_100)
  {
    draw_bitmap(BATT_PT_X, BATT_PT_Y, BATT_ICON_WIDTH, BATT_ICON_HEIGHT, white_battery_100);
  }
  else if(curr_battery_level == BATT_LEVEL_80)
  {
    draw_bitmap(BATT_PT_X, BATT_PT_Y, BATT_ICON_WIDTH, BATT_ICON_HEIGHT, white_battery_80);
  }
  else if(curr_battery_level == BATT_LEVEL_50)
  {
    draw_bitmap(BATT_PT_X, BATT_PT_Y, BATT_ICON_WIDTH, BATT_ICON_HEIGHT, white_battery_50);
  }
  else if(curr_battery_level == BATT_LEVEL_20)
  {
    draw_bitmap(BATT_PT_X, BATT_PT_Y, BATT_ICON_WIDTH, BATT_ICON_HEIGHT, white_battery_20);
  }
  else
  {
    draw_bitmap(BATT_PT_X, BATT_PT_Y, BATT_ICON_WIDTH, BATT_ICON_HEIGHT, white_battery_00);
  }
}

//  Tegner et ikon som indikerer om noden er provisjonert eller ikke
void draw_bt_icon()
{
  curr_provisioned = device_is_proviosioned();
  if(curr_provisioned)
  {
    draw_bitmap(BT_PT_X, BT_PT_Y, BT_ICON_WIDTH, BT_ICON_HEIGHT, BLE_connected_white);
  }
  else
  {
    draw_bitmap(BT_PT_X, BT_PT_Y, BT_ICON_WIDTH, BT_ICON_HEIGHT, BLE_disconnected_white);
  }
}

//  Tegner en klokke med prikker som sier hvilket intervall som er valgt
void draw_time_interval()
{
  curr_time_mode = get_LPN_interval_mode();
  if(curr_time_mode == LPN_INTERVAL_MODE_1)
  {
    draw_bitmap(TIME_PT_X, TIME_PT_Y, TIME_ICON_WIDTH, TIME_ICON_HEIGHT, timer_1_white);
  }
  else if(curr_time_mode == LPN_INTERVAL_MODE_2)
  {
    draw_bitmap(TIME_PT_X, TIME_PT_Y, TIME_ICON_WIDTH, TIME_ICON_HEIGHT, timer_2_white);
  }
  else if(curr_time_mode == LPN_INTERVAL_MODE_3)
  {
    draw_bitmap(TIME_PT_X, TIME_PT_Y, TIME_ICON_WIDTH, TIME_ICON_HEIGHT, timer_3_white);
  }
}

//  Sjekker omtrent hvor lyst det er og tegner tilsvarende lys-ikon
void draw_light_level()
{
  uint16_t lux = calculate_sensor_3_data(LPN - 1);
  if(lux > 3500)
  {
    draw_bitmap(LIGHT_LEVEL_PT_X, LIGHT_LEVEL_PT_Y, LIGHT_ICON_WIDTH, LIGHT_ICON_HEIGHT, sol);
  }
  else if(lux > 100)
  {
    draw_bitmap(LIGHT_LEVEL_PT_X, LIGHT_LEVEL_PT_Y, LIGHT_ICON_WIDTH, LIGHT_ICON_HEIGHT, bulb);
  }
  else
  {
    draw_bitmap(LIGHT_LEVEL_PT_X, LIGHT_LEVEL_PT_Y, LIGHT_ICON_WIDTH, LIGHT_ICON_HEIGHT, dark);
  }
}

//  Oppdaterer skjermen når grafen ikke lenger skal vises
void timeout_handler()
{
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Changing screen\n");
  update_data(true);
}

//  Oppretter en timer for å holde styr på hvor lenge grafen skal vises på skjermen
//  Det skjedde noe feil ved oppretting av bilder, så det blir kjørt en funksjon som fjerner en svart strek fra bildene
void screens_init()
{
  //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Creating graph timer\n");
  app_timer_create(&graph_timeout, APP_TIMER_MODE_SINGLE_SHOT, timeout_handler);
  delete_bitmap_line(BATT_ICON_WIDTH, BATT_ICON_HEIGHT, white_battery_100);
  delete_bitmap_line(BATT_ICON_WIDTH, BATT_ICON_HEIGHT, white_battery_80);
  delete_bitmap_line(BATT_ICON_WIDTH, BATT_ICON_HEIGHT, white_battery_50);
  delete_bitmap_line(BATT_ICON_WIDTH, BATT_ICON_HEIGHT, white_battery_20);
  delete_bitmap_line(BATT_ICON_WIDTH, BATT_ICON_HEIGHT, white_battery_00);
  delete_bitmap_line(LIGHT_ICON_WIDTH, LIGHT_ICON_HEIGHT, bulb);
  delete_bitmap_line(LIGHT_ICON_WIDTH, LIGHT_ICON_HEIGHT, sol);
  delete_bitmap_line(LIGHT_ICON_WIDTH, LIGHT_ICON_HEIGHT, dark);
  delete_bitmap_line(BT_ICON_WIDTH, BT_ICON_HEIGHT, BLE_connected_white);
  delete_bitmap_line(BT_ICON_WIDTH, BT_ICON_HEIGHT, BLE_disconnected_white);
  delete_bitmap_line(TIME_ICON_WIDTH, TIME_ICON_HEIGHT, timer_1_white);
  delete_bitmap_line(TIME_ICON_WIDTH, TIME_ICON_HEIGHT, timer_2_white);
  delete_bitmap_line(TIME_ICON_WIDTH, TIME_ICON_HEIGHT, timer_3_white);
}

