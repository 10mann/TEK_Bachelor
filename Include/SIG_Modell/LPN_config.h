#ifndef LPN_CONFIG_H
#define LPN_CONFIG_H

#define LPN_DATA_PIN 23
#define LPN_CLK_PIN 21

#define SENSOR_1 0
#define SENSOR_2 1
#define SENSOR_3 2

#define LPN_ID_1 1
#define LPN_ID_2 2
#define LPN_ID_3 3

#define LPN_INTERVAL_MODE_1 1
#define LPN_INTERVAL_MODE_2 2
#define LPN_INTERVAL_MODE_3 3

#define SET_INTERVAL_PIN 22
#define GET_REMOTE_DATA_PIN 12
#define DRAW_GRAPH_PIN 32

#define LPN LPN_ID_1

#if LPN == LPN_ID_1
#define LPN1 1
#define LPN2 0
#define LPN3 0

#elif LPN == LPN_ID_2
#define LPN1 0
#define LPN2 1
#define LPN3 0


#elif LPN == LPN_ID_3
#define LPN1 0
#define LPN2 0
#define LPN3 1

#endif

#endif