#ifndef TSL2581_H
#define TSL2581_H

#include <stdio.h>

#define CMD 0x80
#define POWER_ON 0x01
#define POWER_OFF 0x00
#define CTRL_REG 0x00
#define ADC_EN 0x03
#define ITIME  0x6C //0xb6
#define ALS_REG 0x01
#define TSL2581_ADDR 0x39
#define WORD_BIT 0xb4
#define CH0_REG 0x14
#define CH1_REG 0x16
#define AN_REG 0x07


//-------- WAVESHARE --------//
//ANALOG
#define GAIN_1X 0x00
#define GAIN_8X 0x01
#define GAIN_16X 0x02
#define GAIN_111X 0x03


#define LUX_SCALE 16 // scale by 2^16
#define RATIO_SCALE 9 // scale ratio by 2^9
//---------------------------------------------------
// Integration time scaling factors
//---------------------------------------------------
#define CH_SCALE 16 // scale channel values by 2^16

// Nominal 400 ms integration. 
// Specifies the integration time in 2.7-ms intervals
// 400/2.7 = 148
#define NOM_INTEG_CYCLE 148
//---------------------------------------------------
// Gain scaling factors
//---------------------------------------------------
#define CH0GAIN128X 107 // 128X gain scalar for Ch0
#define CH1GAIN128X 115 // 128X gain scalar for Ch1

//---------------------------------------------------
#define K1C 0x009A // 0.30 * 2^RATIO_SCALE
#define B1C 0x2148 // 0.130 * 2^LUX_SCALE
#define M1C 0x3d71 // 0.240 * 2^LUX_SCALE

#define K2C 0x00c3 // 0.38 * 2^RATIO_SCALE
#define B2C 0x2a37 // 0.1649 * 2^LUX_SCALE
#define M2C 0x5b30 // 0.3562 * 2^LUX_SCALE

#define K3C 0x00e6 // 0.45 * 2^RATIO_SCALE
#define B3C 0x18ef // 0.0974 * 2^LUX_SCALE
#define M3C 0x2db9 // 0.1786 * 2^LUX_SCALE

#define K4C 0x0114 // 0.54 * 2^RATIO_SCALE
#define B4C 0x0fdf // 0.062 * 2^LUX_SCALE
#define M4C 0x199a // 0.10 * 2^LUX_SCALE

#define K5C 0x0114 // 0.54 * 2^RATIO_SCALE
#define B5C 0x0000 // 0.00000 * 2^LUX_SCALE
#define M5C 0x0000 // 0.00000 * 2^LUX_SCALE
//---------------------------------------------------

void TSL2581_init();
void TSL2581_read(uint16_t* calc);
uint16_t calculateLux();
#endif //TSL2581_H
