#ifndef LPN1_H
#define LPN1_H

#include "simple_sensor_client.h"
#include "simple_sensor_server.h"

#define GET_REQ_TIME 150

void init_lpn();
void update_and_send_data(simple_sensor_server_t* p_server);
bool update_data(bool force);
void send_data(simple_sensor_server_t* p_server);

void read_sensors(uint16_t* raw, double* calc);

void display_LPN_1_data(simple_sensor_client_t* p_client);
void display_LPN_2_data(simple_sensor_client_t* p_client);
void display_LPN_3_data(simple_sensor_client_t* p_client);

void print_sensor_values(bool external);
void print_extern_sensor_values();

uint16_t calculate_sensor_1_data(uint16_t lpn_id);
uint16_t calculate_sensor_2_data(uint16_t lpn_id);
uint16_t calculate_sensor_3_data(uint16_t lpn_id);

void calc_column_data(uint16_t* data);
uint8_t max_column_temp(uint16_t* data);
uint8_t min_column_temp(uint16_t* data);

bool device_is_proviosioned();

#endif