#ifndef SENSOR_UART_H
#define SENSOR_UART_H

#define STATUS 0x01
#define DESCRIPTOR 0x02
#define CADENCE 0x03
#define TRANSFER_DONE 0xA5


void uart_send(uint8_t* data, uint16_t length);

#endif // SENSOR_UART_H
