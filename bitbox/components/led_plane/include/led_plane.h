/*****************************************************************************
File name: led_plane.h
Description: 灯板驱动的头文件，定义了一些设置与API
Author: LO_StacNet
Date: 2024/8/18
*****************************************************************************/
#ifndef LED_PLANE_H
#define LED_PLANE_H

#include "stdint.h"

#define LED_STRIP_RESOLUTION_HZ 10000000
#define LED_STRIP_GPIO_NUM 0
#define LED_PLANE_X 18
#define LED_PLANE_Y 12

void LEDPlaneInit(void);
void LEDPlaneFlush(void);
uint8_t LEDPlaneDrawRGB(uint8_t x,uint8_t y,uint8_t r,uint8_t g,uint8_t b);
uint8_t LEDPlaneDraw(uint8_t x,uint8_t ,uint16_t color);
void LEDPlaneFill(uint16_t color);

#endif
