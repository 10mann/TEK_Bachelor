// waveshare_epd.h
// Waveshare EPaper Display
// Implementation for Nordic nRF5x GFX Library
#ifndef WAVESHARE_H
#define WAVESHARE_H

#include "nrf_gfx.h"

void wsepd154_draw_monobmp(const uint8_t *image_buffer);

ret_code_t wsepd154_init();
void wsepd154_uninit();
void wsepd154_pixel_draw(uint16_t, uint16_t, uint32_t);
void wsepd154_rect_draw(uint16_t, uint16_t, uint16_t, uint16_t, uint32_t);
void wsepd154_display();
void wsepd154_rotation_set(nrf_lcd_rotation_t rotation);
void wsepd154_display_invert(bool);
void display_partial(uint16_t, uint16_t, uint16_t, uint16_t);
void display_partial_full();
void setPartialUpdate();
void clear_screen_buffer();
void clear_screen();
void fill_screen();
void draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* buff);
void delete_bitmap_line(uint16_t width, uint16_t height, uint8_t* buff);
#endif