// Waveshare EPaper Display
// Implementation for Nordic nRF5x GFX Library

#include "sdk_common.h"

//#include "paper.h"
#include "screen_config.h"

#include "nrf_lcd.h"
#include "nrf_drv_spi.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"
#include "waveshare_epd.h"
#include "nrf_font.h"
#include "nrf_gfx.h"
#include "log.h"
//#include "app_timer.h"

//#define NRF_LOG_MODULE_NAME "GFX_WS"
//#include "nrf_log.h"

// EPD1IN54 commands
#define DRIVER_OUTPUT_CONTROL                       0x01
#define BOOSTER_SOFT_START_CONTROL                  0x0C
#define GATE_SCAN_START_POSITION                    0x0F
#define DEEP_SLEEP_MODE                             0x10
#define DATA_ENTRY_MODE_SETTING                     0x11
#define SW_RESET                                    0x12
#define TEMPERATURE_SENSOR_CONTROL                  0x1A
#define MASTER_ACTIVATION                           0x20
#define DISPLAY_UPDATE_CONTROL_1                    0x21
#define DISPLAY_UPDATE_CONTROL_2                    0x22
#define WRITE_RAM                                   0x24
#define WRITE_VCOM_REGISTER                         0x2C
#define WRITE_LUT_REGISTER                          0x32
#define SET_DUMMY_LINE_PERIOD                       0x3A
#define SET_GATE_TIME                               0x3B
#define BORDER_WAVEFORM_CONTROL                     0x3C
#define SET_RAM_X_ADDRESS_START_END_POSITION        0x44
#define SET_RAM_Y_ADDRESS_START_END_POSITION        0x45
#define SET_RAM_X_ADDRESS_COUNTER                   0x4E
#define SET_RAM_Y_ADDRESS_COUNTER                   0x4F
#define TERMINATE_FRAME_READ_WRITE                  0xFF

const nrf_lcd_t nrf_lcd_wsepd154;

extern const nrf_gfx_font_desc_t orkney_8ptFontInfo;
extern const nrf_gfx_font_desc_t orkney_24ptFontInfo;
extern const uint16_t test_smil[];
/*
const unsigned char lut_full_update[] = {
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22,
    0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88,
    0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51,
    0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};
*/

const unsigned char lut_full_update[] = {
    0x50, 0xAA, 0x55, 0xAA, 0x11, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xFF, 0xFF, 0x1F, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char lut_partial_update[] = {
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


lcd_cb_t wsepd154_cb = {
    .height = WSEPD_HEIGHT,
    .width = WSEPD_WIDTH,
};

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(WSEPD_SPI_INSTANCE);
uint8_t screen_buffer[((WSEPD_WIDTH % 8 == 0)? (WSEPD_WIDTH / 8 ): (WSEPD_WIDTH / 8 + 1)) * WSEPD_HEIGHT];

static inline void spi_write(const void * data, size_t size)
{
    APP_ERROR_CHECK(nrf_drv_spi_transfer(&spi, data, size, NULL, 0));
}

//  Venter til BUSY-pinnen på skjerm trekkes lav
static void wait_until_idle(void)
{
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Busy: %d\n", nrf_gpio_pin_read(WSEPD_BUSY));
    while(nrf_gpio_pin_read(WSEPD_BUSY) == 1) {      //LOW: idle, HIGH: busy
      //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "BUSY\n");
        nrf_delay_ms(100);
    }
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Busy: %d\n\n", nrf_gpio_pin_read(WSEPD_BUSY));
}

//  Skriver kommando til skjermen
static inline void write_command(uint8_t c)
{
    nrf_gpio_pin_clear(WSEPD_DC);
    spi_write(&c, sizeof(c));
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "Command: 0x%x, size: %d\n", c, sizeof(c));
    nrf_gpio_pin_set(WSEPD_SPI_CS);
}

//  Skriver data til skjermen
static inline void write_data(uint8_t c)
{
    nrf_gpio_pin_set(WSEPD_DC);
    spi_write(&c, sizeof(c));
}

//  Setter markøren på skjermen
static void set_cursor(uint16_t x, uint16_t y)
{
    write_command(SET_RAM_X_ADDRESS_COUNTER);
    write_data((x >> 3) & 0xff);

    write_command(SET_RAM_Y_ADDRESS_COUNTER);
    write_data(y & 0xFF);
    write_data((y >> 8) & 0xFF);
    wait_until_idle();
}

//  Setter adresse-vindu som skal benyttes på skjermen
static void set_addr_window(uint16_t x_0, uint16_t y_0, uint16_t x_1, uint16_t y_1)
{
    ASSERT(x_0 < x_1);
    ASSERT(y_0 < y_1);

    write_command(SET_RAM_X_ADDRESS_START_END_POSITION);
    write_data((x_0 >> 3) & 0xff);
    write_data((x_1 >> 3) & 0xff);
    write_command(SET_RAM_Y_ADDRESS_START_END_POSITION);
    write_data(y_0 & 0xff);
    write_data((y_0 >> 8) & 0xff);
    write_data(y_1 & 0xff);
    write_data((y_1 >> 8) & 0xff);
}

//  Viser bilde på skjermen fra RAM
static void turn_on_display(void)
{
    write_command(DISPLAY_UPDATE_CONTROL_2);
    write_data(0xC4); //C4 C4 C7 F7
    write_command(MASTER_ACTIVATION);
    write_command(TERMINATE_FRAME_READ_WRITE);

    wait_until_idle();
}

//  Viser delvis bilde på skjermen fra RAM
static void turn_on_display_part()
{
    write_command(DISPLAY_UPDATE_CONTROL_2);
    write_data(0xFF); //C4 FF
    write_command(MASTER_ACTIVATION);
    write_command(TERMINATE_FRAME_READ_WRITE);

    wait_until_idle();
}

static void wsepd_reset(void)
{
    nrf_gpio_pin_set(WSEPD_RST);
    nrf_delay_ms(200);
    nrf_gpio_pin_clear(WSEPD_RST);
    nrf_delay_ms(10); //10 200
    nrf_gpio_pin_set(WSEPD_RST);
    nrf_delay_ms(200);
    wait_until_idle();
}

//  Innstillinger for oppstart av skjermen
static void command_list(void)
{
    wsepd_reset();
#if EPAPER_MODULE2_9
    write_command(0x12);
    wait_until_idle();
    write_command(DRIVER_OUTPUT_CONTROL);
    write_data((WSEPD_HEIGHT - 1) & 0xFF);
    write_data(((WSEPD_HEIGHT - 1) >> 8) & 0xFF);
    write_data(0x00);

    write_command(BOOSTER_SOFT_START_CONTROL);
    write_data(0xD7);
    write_data(0xD6);
    write_data(0x9D);

    write_command(WRITE_VCOM_REGISTER);
    write_data(0xA8);

    write_command(SET_DUMMY_LINE_PERIOD);
    write_data(0x1A);

    write_command(SET_GATE_TIME);
    write_data(0x08);

    write_command(BORDER_WAVEFORM_CONTROL);
    write_data(0x03);

    write_command(DATA_ENTRY_MODE_SETTING);
    write_data(0x03);

    write_command(WRITE_LUT_REGISTER);
    for(uint8_t i = 0; i < 30; i++)
    {
        // TODO: allow partial update mode
        write_data(lut_full_update[i]);
    }

#else
/*
    write_command(0x12);
    wait_until_idle();
*/
    write_command(DRIVER_OUTPUT_CONTROL);
    write_data(0xC7);
    write_data(0x00);
    write_data(0x00); //0x00

    write_command(BOOSTER_SOFT_START_CONTROL);
    write_data(0x8F);
    write_data(0x8F);
    write_data(0x8F);
/*
    write_data((WSEPD_HEIGHT - 1) & 0xFF);
    write_data(((WSEPD_HEIGHT - 1) >> 8) & 0xFF);
    write_data(0x00);
*/
    write_command(DATA_ENTRY_MODE_SETTING);
    //write_data(0x01);
    write_data(0x03);
    
    write_command(SET_RAM_X_ADDRESS_START_END_POSITION);
    write_data(0x00);
    write_data(0x18);

    write_command(SET_RAM_Y_ADDRESS_START_END_POSITION);
    write_data(0xC7);
    write_data(0x00);
    write_data(0x00);
    write_data(0x00);
/*
    write_command(0x1A);
    //write_data(0x1);
    write_data(0x90);
*/
    write_command(BORDER_WAVEFORM_CONTROL);
    write_data(0x01); //0x01 0xFF

/*
    write_command(0x12);
    write_data(0x80);
*/

    write_command(0x18);
    write_data(0x80); //0x80

    write_command(DISPLAY_UPDATE_CONTROL_2);
    write_data(0xB1); // B1 B0
    write_command(0X20);

/*
    write_command(DISPLAY_UPDATE_CONTROL_1);
    write_data(0x80);
*/
    write_command(SET_RAM_X_ADDRESS_COUNTER);
    write_data(0x00);

    write_command(SET_RAM_Y_ADDRESS_COUNTER);
    write_data(0xC7);
    write_data(0x00);  


    wait_until_idle();

#endif
}

//  Starter opp maskinvare som blir brukt til å drive skjermen
static ret_code_t hardware_init(void)
{
    ret_code_t err_code;

    nrf_gpio_cfg_output(WSEPD_DC);
    nrf_gpio_cfg_output(WSEPD_RST);
#if LPN == LPN_ID_1
    nrf_gpio_cfg_output(WSEPD_VCC);
    nrf_gpio_pin_set(WSEPD_VCC);
#endif
    nrf_gpio_pin_set(WSEPD_RST);
    nrf_gpio_cfg_input(WSEPD_BUSY, NRF_GPIO_PIN_NOPULL);
    nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_DEFAULT_CONFIG;
    spi_config.sck_pin  = WSEPD_SPI_CLK;
    spi_config.mosi_pin = WSEPD_SPI_MOSI;
    spi_config.ss_pin = WSEPD_SPI_CS;

    err_code = nrf_drv_spi_init(&spi, &spi_config, NULL, NULL);
    if(err_code != NRF_SUCCESS)
    {
      //nrf_gpio_pin_set(5);
      //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "\n\nSPI setup: %d\n\n", err_code);
    }
    return err_code;
}

//  Starter skjermen med innstillinger valgt i comand_list
//  Setter også opp SPI
ret_code_t wsepd154_init(void)
{
    ret_code_t err_code;
    err_code = hardware_init();
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "EPAPER hardware started\n");
    command_list();
    memset(screen_buffer, 0xff, sizeof(screen_buffer));
    return 0;
}

//  Setter skjermen i sleep, skrur skjermen helt av på LPN 1
//  Deaktiverer SPI for å spare strøm i dvale
void wsepd154_uninit(void)
{
    ASSERT(wsepd154_cb.state != NRFX_DRV_STATE_UNINITIALIZED);
    write_command(0x12);
    write_command(DEEP_SLEEP_MODE);
    write_data(0x01);
    nrf_drv_spi_uninit(&spi);
#if LPN == LPN_ID_1
    nrf_gpio_pin_clear(WSEPD_VCC);
#endif
    nrf_gpio_pin_set(WSEPD_DC);
    nrf_gpio_pin_set(WSEPD_SPI_CLK);
    nrf_gpio_pin_set(WSEPD_SPI_CS);
    nrf_gpio_pin_set(WSEPD_SPI_MOSI);
}


void wsepd154_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
{
    uint16_t x_, y_;
 
    if(x > WSEPD_WIDTH || y > WSEPD_HEIGHT){
        //NRF_LOG_INFO("Exceeding display boundaries\r\n");
        return;
    }
    switch(wsepd154_cb.rotation) 
    {
        case NRF_LCD_ROTATE_90:
            x_ = WSEPD_WIDTH - y - 1;
            y_ = x;
            break;
        case NRF_LCD_ROTATE_180:
            x_ = WSEPD_WIDTH - x - 1;
            y_ = WSEPD_HEIGHT - y - 1;
            break;
        case NRF_LCD_ROTATE_270:
            x_ = y;
            y_ = WSEPD_HEIGHT - x - 1;
            break;
        default:
            x_ = x;
            y_ = y;
            break;
    }
    if(x_ > WSEPD_WIDTH || y_ > WSEPD_HEIGHT){
        //NRF_LOG_INFO("Exceeding display boundaries\r\n");
        return;
    }
    uint16_t addr = x_ / 8 + y_ * ((WSEPD_WIDTH % 8) == 0 ? WSEPD_WIDTH / 8 : WSEPD_WIDTH / 8 + 1);
    uint8_t rdata = screen_buffer[addr];
    if(color == 0)
        screen_buffer[addr] = rdata & ~(0x80 >> (x_ % 8));
    else
        screen_buffer[addr] = rdata | (0x80 >> (x_ % 8));
}

void wsepd154_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    uint16_t x_, y_;
 
    // TODO: This could be done about 8x faster with cheeky bitfield manipulations
    //     but it's really difficult to get it all perfect given the 6 conditions
    for(y_ = y; y_ < (y + height); y_ ++)
    {
        for(x_ = x; x_ < (x + width); x_ ++)
        {
            wsepd154_pixel_draw(x_, y_, color);
        }
    }
}

//  Sjekker først hvilken skjerm som er koblet til, deretter kjøres riktig kode for å skrive til skjermen
#if EPAPER_MODULE1_54
void wsepd154_display(void)
{
    uint16_t width;
    uint32_t addr = 0;
    uint16_t i, j;

    width = (WSEPD_WIDTH % 8 == 0) ? (WSEPD_WIDTH / 8) : (WSEPD_WIDTH / 8 + 1);
    set_addr_window(0, 0, WSEPD_WIDTH - 1, WSEPD_HEIGHT - 1);
    set_cursor(0, 0);
    write_command(WRITE_RAM);
    for(i = 0; i < WSEPD_HEIGHT; i++)
    {
        for(j = 0; j < width; j++)
        {
            addr = j + i * width;
            write_data(screen_buffer[addr]);
        }
    }

    write_command(0x26);
    for(i = 0; i < WSEPD_HEIGHT; i++)
    {
        for(j = 0; j < width; j++)
        {
            addr = j + i * width;
            write_data(screen_buffer[addr]);
        }
    }

    //turn_on_display_part();
    turn_on_display();
}
#else
void wsepd154_display(void)
{
    uint16_t width;
    uint32_t addr = 0;
    uint16_t i, j;

    width = (WSEPD_WIDTH % 8 == 0) ? (WSEPD_WIDTH / 8) : (WSEPD_WIDTH / 8 + 1);
    set_addr_window(0, 0, WSEPD_WIDTH - 1, WSEPD_HEIGHT - 1);
    set_cursor(0, 0);
    write_command(WRITE_RAM);
    for(i = 0; i < WSEPD_HEIGHT; i++)
    {
      for(j = 0; j < width; j++)
      {
        addr = j + i * width;
        write_data(screen_buffer[addr]);
      }
    }
    turn_on_display();
}
#endif

//  Måtte legge til en funksjon for å delvis oppdatere skjermen, denne implementasjonen manglet fra før
void display_partial(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    uint16_t width, screen_width;
    uint32_t addr = 0;
    uint16_t i, j;
    width = (w % 8 == 0) ? (w / 8) : (w / 8 + 1);
    screen_width = (WSEPD_WIDTH % 8 == 0) ? (WSEPD_WIDTH / 8) : (WSEPD_WIDTH / 8 + 1);
    set_addr_window(x, y, x + w - 1, y + h - 1);
    set_cursor(x, y);
    write_command(WRITE_RAM);

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < width; j++)
        {
            addr = j + i * screen_width + x / 8 + ((y) * screen_width);
            write_data(screen_buffer[addr]);
        }
    }

    turn_on_display_part();
}

void setPartialUpdate()
{
    write_command(WRITE_LUT_REGISTER);
    for(uint8_t i = 0; i < 30; i++)
    {
        write_data(lut_partial_update[i]);
    }
}

void clear_screen_buffer()
{
  memset(screen_buffer, 0xFF, sizeof(screen_buffer));
}

void clear_screen()
{
  fill_screen();
  turn_on_display_part();
}

void fill_screen()
{
    memset(screen_buffer, 0xFF, sizeof(screen_buffer));
    uint16_t width;
    uint32_t addr = 0;
    uint16_t i, j;

    width = (WSEPD_WIDTH % 8 == 0) ? (WSEPD_WIDTH / 8) : (WSEPD_WIDTH / 8 + 1);
    set_addr_window(0, 0, WSEPD_WIDTH - 1, WSEPD_HEIGHT - 1);
    set_cursor(0, 0);

    write_command(WRITE_RAM);
    for(i = 0; i < WSEPD_HEIGHT; i++)
    {
        for(j = 0; j < width; j++)
        {
            write_data(0xFF);
        }
    }
}

//  Laget en funksjon for å tegne bitmaps på skjermen, gjort for å kunne spare opp til 16 ganger plass i minne
//  Gammel funksjon for å tegne bitmaps brukte 16-bits per piksel, er benyttes kun 1-bit
void draw_bitmap(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* buff)
{
  uint16_t screen_width = (WSEPD_WIDTH % 8 == 0) ? (WSEPD_WIDTH / 8) : (WSEPD_WIDTH / 8 + 1);
  uint16_t w = (width % 8 == 0) ? (width / 8) : (width / 8 + 1);
  uint16_t addr, buff_addr;
  for(uint16_t i = 0; i < height; i++)
  { 
    for(uint16_t j = 0; j < w; j++)
    {
      //__LOG(LOG_SRC_APP, LOG_LEVEL_INFO, "buff[%d] = 0x%02x\n", i*w+j, (uint8_t)buff[i*w+j]);

      for(uint8_t k = 0; k < 8; k++)
      {
        wsepd154_pixel_draw(width - (j*8 - k) + x - 8, (height - i) + y - 1, (buff[i*w+j]) & (1 << k));
      }
    }
  }
}

//  Funksjon brukt for å fjerne en svart strek som ble med på bildene vi laget
void delete_bitmap_line(uint16_t width, uint16_t height, uint8_t* buff)
{
  uint16_t w = (width % 8 == 0) ? (width / 8) : (width / 8 + 1);
  for(uint16_t i = 0; i < height; i++)
  { 
    for(uint16_t j = w - 1; j < w; j++)
    {
      buff[i*w+j] |= 0x07;
    }
  }
}

void wsepd154_rotation_set(nrf_lcd_rotation_t rotation)
{
    // dummy function - actions are taken in the drawing primitives
}

void wsepd154_display_invert(bool invert)
{
    // dummy function - doesn't have feature built in
}

// Can only draw an image the size of screen or larger
//  Kan bare skrive ut bilder som er like stor som skjermen eller større, derfor ble det opprettet en egen bitmap-funksjon
void wsepd154_draw_monobmp(const uint8_t *image_buffer)
{
    uint16_t x, y;
    uint32_t addr = 0;
    uint8_t byte_width = (WSEPD_WIDTH % 8 == 0) ? (WSEPD_WIDTH / 8) : (WSEPD_WIDTH / 8 + 1);

    for (y = 0; y < WSEPD_HEIGHT; y++) {
        for (x = 0; x < byte_width; x++) 
        {//8 pixel =  1 byte
            addr = x + y * byte_width;
            screen_buffer[addr] = (uint8_t)image_buffer[addr];
        }
    }
}


