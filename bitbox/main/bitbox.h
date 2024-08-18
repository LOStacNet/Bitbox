/*****************************************************************************
File name: bitbox.h
Description: BitBox工程的总头文件，包含共用的数据结构和类型以及各种API
Author: LO_StacNet
Date: 2024/8/18
*****************************************************************************/
#ifndef BITBOX_H_
#define BITBOX_H_

#include "esp_log.h"
#include "esp_random.h"
#include "ft6236.h"
#include "led_plane.h"
#include "encoder.h"

#define MAX_APPID 3 //number of apps

typedef enum {
    INPUT_NO_EVENT=0,
    ENCODER_CW_EVENT=1,
    ENCODER_CCW_EVENT=2,
    SWITCH_DOWN_EVENT=3,
    SWITCH_LONG_DOWN_EVENT=4,
    TOUCH_EVENT=5
}InputEvent;//Event for all apps

/**
 * The app id for device
 * 0:menu
 * 1:print plane
 * 2:brick break
 */
extern uint8_t Global_AppID;

/**
 * input event api.if want to use input device,app must set it
 * @param e
 * @param data
 */
extern void (*input_event_handle)(InputEvent e,void *data);

/**
 * the app function running in loop
 */
extern void (*app_run)(void);

/**
 * Enter the app by force
 * @param app_id app id
 */
void APPEnter(uint8_t app_id);

/**
 * exit app
 * @param app_id the next app to jump
 */
void APPExit(uint8_t app_id);

/**
 * converse touch plane coordinate to led plane's
 * @param x
 * @param y
 * @return
 */
uint8_t TouchToPlane(uint16_t *x,uint16_t *y);

/**
 * send a req for led plane refresh
 */
void LEDPlaneShow(void);

/*============================App Functions===================================*/
uint8_t enter_menu(void);//appid=0
uint8_t enter_print(void);//appid=1
uint8_t enter_brick(void);//appid=2

#endif