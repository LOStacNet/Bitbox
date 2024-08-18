/*****************************************************************************
File name: encoder.h
Description: 编码器SIQ-02FVS3的驱动头文件
Author: LO_StacNet
Date: 2024/8/18
*****************************************************************************/
#ifndef ENCODER_H
#define ENCODER_H

#include "stdint.h"

#define ENCODER_A_PIN 2
#define ENCODER_B_PIN 3
#define ENCODER_S_PIN 10

typedef enum {
    ENCODER_NONE=0, /* no event */
    ENCODER_CW,     /* revolve in CW dir */
    ENCODER_CCW,    /* revolve in CCW dir */
    ENCODER_SW_DOWN,   /* bottom is pressed */
    ENCODER_SW_LONG   /* bottom is long pressed */
}Encoder_Event;

uint8_t encoder_init(void);
void encoder_deinit(void);
Encoder_Event encoder_read(uint8_t timeout);

#endif
