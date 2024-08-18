#include "led_plane.h"
#include "esp_log.h"
#include "led_strip_encoder.h"
#include "string.h"
#include "driver/rmt_tx.h"

static const char* TAG = "LED_Plane";

rmt_channel_handle_t chantx=NULL;
rmt_encoder_handle_t led_encoder=NULL;
rmt_transmit_config_t tx_config;
//form right to left,high to low
static uint8_t led_plane_pixels[LED_PLANE_X][LED_PLANE_Y][3];

/**
 * just initial the rmt and set all led to 0
 */
void LEDPlaneInit(void)
{
    //install tx channel
    rmt_tx_channel_config_t  chconfig={
            .gpio_num=LED_STRIP_GPIO_NUM,
            .clk_src=RMT_CLK_SRC_DEFAULT,
            .mem_block_symbols=64,
            .resolution_hz=LED_STRIP_RESOLUTION_HZ,
            .trans_queue_depth=4,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&chconfig,&chantx));
    //install encoder
    led_strip_encoder_config_t encoder_config={
            .resolution=LED_STRIP_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(rmt_new_led_strip_encoder(&encoder_config,&led_encoder));
    //enable rmt
    ESP_ERROR_CHECK(rmt_enable(chantx));
    //prepare txconfig
    tx_config.loop_count = 0; // no transfer loop

    //set the led to 0
    memset(led_plane_pixels,0,sizeof(led_plane_pixels));
    ESP_ERROR_CHECK(rmt_transmit(chantx, led_encoder, led_plane_pixels, sizeof(led_plane_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(chantx, 0xFFFFFFFF));
}

/**
 * update the data to led
 */
void LEDPlaneFlush(void)
{
    ESP_ERROR_CHECK(rmt_transmit(chantx, led_encoder, led_plane_pixels, sizeof(led_plane_pixels), &tx_config));
    ESP_ERROR_CHECK(rmt_tx_wait_all_done(chantx, 0xFFFFFFFF));
}

static void extractRGB565(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (color & 0xF800) >> 11;  // 提取红色分量
    *g = (color & 0x07E0) >> 5;   // 提取绿色分量
    *b = color & 0x001F;          // 提取蓝色分量

    *r = *r << 3;  // 将红色分量扩展到 8 位
    *g = *g << 2;  // 将绿色分量扩展到 8 位
    *b = *b << 3;  // 将蓝色分量扩展到 8 位
}

/**
 * Draw a point on the plane,not flush
 * @param x
 * @param y
 * @param r red component
 * @param g green component
 * @param b blue component
 * @return 0:success 1:failure
 */
uint8_t LEDPlaneDrawRGB(uint8_t x,uint8_t y,uint8_t r,uint8_t g,uint8_t b)
{
    uint8_t _x,_y;

    if(x>=LED_PLANE_X || y>=LED_PLANE_Y) return 1;

    _x=LED_PLANE_X-1-x;_y=y;
    led_plane_pixels[_x][_y][0]=g;
    led_plane_pixels[_x][_y][1]=r;
    led_plane_pixels[_x][_y][2]=b;

    return 0;
}

/**
 * Draw a point on the plane,not flush
 * @param x
 * @param y
 * @param color color in RGB565
 * @return 0:success 1:failure
 */
uint8_t LEDPlaneDraw(uint8_t x,uint8_t y,uint16_t color)
{
    uint8_t _x,_y;
    uint8_t r,g,b;

    if(x>=LED_PLANE_X || y>=LED_PLANE_Y) return 1;

    _x=LED_PLANE_X-1-x;_y=y;
    extractRGB565(color, &r, &g, &b);
    led_plane_pixels[_x][_y][0]=g;
    led_plane_pixels[_x][_y][1]=r;
    led_plane_pixels[_x][_y][2]=b;

    return 0;
}

/**
 * fill the plane with one color
 * @param color
 */
void LEDPlaneFill(uint16_t color)
{
    uint8_t r,g,b;

    extractRGB565(color, &r, &g, &b);
    for(uint8_t _x=0;_x<LED_PLANE_X;_x++)
        for(uint8_t _y=0;_y<LED_PLANE_Y;_y++){
            led_plane_pixels[_x][_y][0]=g;
            led_plane_pixels[_x][_y][1]=r;
            led_plane_pixels[_x][_y][2]=b;
        }
}
