/*****************************************************************************
File name: ft6236.h
Description: ft6236触摸屏的驱动，API兼容IDF组件库
Author: LO_StacNet
Date: 2024/8/18
*****************************************************************************/
#ifndef FT6236_H
#define FT6236_H
#include "stdint.h"
#include "driver/i2c.h"

//the max point that support
#define TOUCH_MAX_POINT_NUMBER (2)

typedef enum {
    /* @---> X
       |
       Y
    */
    TOUCH_DIR_LRTB,   /**< From left to right then from top to bottom, this consider as the original direction of the touch panel*/

    /*  Y
        |
        @---> X
    */
    TOUCH_DIR_LRBT,   /**< From left to right then from bottom to top */

    /* X <---@
             |
             Y
    */
    TOUCH_DIR_RLTB,   /**< From right to left then from top to bottom */

    /*       Y
             |
       X <---@
    */
    TOUCH_DIR_RLBT,   /**< From right to left then from bottom to top */

    /* @---> Y
       |
       X
    */
    TOUCH_DIR_TBLR,   /**< From top to bottom then from left to right */

    /*  X
        |
        @---> Y
    */
    TOUCH_DIR_BTLR,   /**< From bottom to top then from left to right */

    /* Y <---@
             |
             X
    */
    TOUCH_DIR_TBRL,   /**< From top to bottom then from right to left */

    /*       X
             |
       Y <---@
    */
    TOUCH_DIR_BTRL,   /**< From bottom to top then from right to left */

    TOUCH_DIR_MAX
} touch_panel_dir_t;

typedef enum {
    TOUCH_EVT_PRESS   = 0x0,  /*!< Press event */
    TOUCH_EVT_RELEASE = 0x1,  /*!< Release event */
    TOUCH_EVT_CONTACT = 0x2,  /*!< Contact event */
    TOUCH_EVT_NONE = 0x3,  /*!< no event */
} touch_panel_event_t;

typedef enum {
    TOUCH_GES_NONE = 0x00,  /*!< No Gesture */
    TOUCH_GES_MOVE_UP = 0x10,  /*!< Move up */
    TOUCH_GES_MOVE_RIGHT = 0x14,  /*!< Move right */
    TOUCH_GES_MOVE_DOWN = 0x18,  /*!< Move down */
    TOUCH_GES_MOVE_LEFT = 0x1C, /*!< Move left */
    TOUCH_GES_ZOOM_IN   = 0x48, /*!< Zoom in */
    TOUCH_GES_ZOOM_OUT  = 0x49, /*!< Zoom out */
} touch_panel_gesture;

typedef struct {
    struct {
        uint8_t addr;                              /*!< dev's address */
        i2c_port_t i2c_num;                        /*!< i2c port to use */
        uint8_t i2c_scl;                           /*!< i2c scl pin */
        uint8_t i2c_sda;                           /*!< i2c sda pin */
    }i2c;                                          /*!< i2c bsp config */
    int8_t pin_num_int;                            /*!< Interrupt pin of touch panel. NOTE: You can set to -1 for no connection with hardware. If PENIRQ is connected, set this to pin number. */
    int8_t pin_num_rst;                            /*!< rst pin  */
    touch_panel_dir_t direction;                   /*!< Rotate direction */
    uint16_t width;                                /*!< touch panel width */
    uint16_t height;                               /*!< touch panel height */
} touch_panel_config_t;

typedef struct {
    uint8_t point_num;           /*!< Touch point number */
    touch_panel_gesture gesture; /*!< Gesture of touch */
    touch_panel_event_t event[TOUCH_MAX_POINT_NUMBER];   /*!< Event of touch */
    uint8_t weight[TOUCH_MAX_POINT_NUMBER];             /*!< weight of touch */
    uint16_t curx[TOUCH_MAX_POINT_NUMBER];            /*!< Current x coordinate */
    uint16_t cury[TOUCH_MAX_POINT_NUMBER];            /*!< Current y coordinate */
} touch_panel_points_t;

uint8_t ft6236_init(touch_panel_config_t *config);
void ft6236_deinit(void);
uint8_t ft6236_set_direction(touch_panel_dir_t dir);
uint8_t ft6236_is_press(void);
uint8_t ft6236_get_points(touch_panel_points_t *info);
#endif
