#include "ft6236.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG ="ft6236";
//register list
#define FT6236_REG_DEV_MODE     0x00
#define FT6236_REG_GEST_ID      0x01
#define FT6236_REG_TD_STATUS    0x02
#define FT6236_REG_P1_XH        0x03
#define FT6236_REG_P1_XL        0x04
#define FT6236_REG_P1_YH        0x05
#define FT6236_REG_P1_YL        0x06
#define FT6236_REG_P1_WEIGHT    0x07
#define FT6236_REG_P1_MISC      0x08
#define FT6236_REG_P2_XH        0x09
#define FT6236_REG_P2_XL        0x0A
#define FT6236_REG_P2_YH        0x0B
#define FT6236_REG_P2_YL        0x0C
#define FT6236_REG_P2_WEIGHT    0x0D
#define FT6236_REG_P2_MISC      0x0E

#define FT6236_REG_TH_GROUP     0x80
#define FT6236_REG_TH_DIFF      0x85
#define FT6236_REG_CTRL         0x86
#define FT6236_REG_TIMEENTERM   0x87

#define FT6236_REG_CIPHER       0xA3
#define FT6236_REG_G_MODE       0xA4
#define FT6236_REG_PWR_MODE     0xA5
#define FT6236_REG_FIRMID       0xA6

#define FT6236_REG_STATE        0xBC

typedef struct {
    int8_t int_pin;
    int8_t rst_pin;
    struct {
        uint8_t sda_pin;
        uint8_t scl_pin;
        uint8_t addr;
        i2c_port_t i2c_num;
    }i2c;//存储bsp相关数据

    touch_panel_dir_t direction;
    uint16_t width;
    uint16_t height;
} ft6236_dev_t;

static ft6236_dev_t g_dev;

//init the bsp interface
static uint8_t ft6236_interface_init(ft6236_dev_t *dev)
{
    esp_err_t err=ESP_OK;

    i2c_config_t config={
            .mode=I2C_MODE_MASTER,
            .sda_io_num=dev->i2c.sda_pin,
            .scl_io_num=dev->i2c.scl_pin,
            .scl_pullup_en=GPIO_PULLUP_ENABLE,
            .sda_pullup_en=GPIO_PULLUP_ENABLE,
            .master.clk_speed=400000,
            .clk_flags=0
    };
    err |= i2c_param_config(dev->i2c.i2c_num,&config);
    err |= i2c_driver_install(dev->i2c.i2c_num,config.mode,0,0,0);

    //int_pin
    if(dev->int_pin != -1) {
        gpio_set_direction(dev->int_pin, GPIO_MODE_INPUT);
        gpio_set_pull_mode(dev->int_pin, GPIO_PULLUP_ONLY);
    }
    //rst_pin
    if(dev->rst_pin != -1) {
        gpio_set_direction(dev->rst_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(dev->rst_pin, 0);
        vTaskDelay(400 / portTICK_PERIOD_MS);//at last 300ms
        gpio_set_level(dev->rst_pin, 1);
    }

    return err;
}

static void ft6236_interface_deinit(ft6236_dev_t *dev)
{

    i2c_driver_delete(dev->i2c.i2c_num);
    if(dev->int_pin != -1)
        gpio_reset_pin(dev->int_pin);
    if(dev->rst_pin != -1)
        gpio_reset_pin(dev->rst_pin);
}
//base reg func
static uint8_t ft6236_write_one_reg(ft6236_dev_t *dev, uint8_t start_addr, uint8_t data)
{
    esp_err_t err=ESP_OK;
    i2c_port_t i2c_num=dev->i2c.i2c_num;
    uint8_t _data[2]={start_addr,data};

    i2c_cmd_handle_t handle=i2c_cmd_link_create();
    if(handle==NULL) return 1;
    i2c_master_start(handle);
    i2c_master_write_byte(handle, (dev->i2c.addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(handle, _data, 2, true);
    i2c_master_stop(handle);
    err = i2c_master_cmd_begin(i2c_num, handle, 1000*portTICK_PERIOD_MS);
    i2c_cmd_link_delete(handle);

    return err==ESP_OK ? 0:1;
}

static uint8_t ft6236_read_reg(ft6236_dev_t *dev,uint8_t start_addr,uint8_t read_num,uint8_t *data_buf)
{
    esp_err_t err=ESP_OK;
    i2c_port_t i2c_num=dev->i2c.i2c_num;

    i2c_cmd_handle_t handle=i2c_cmd_link_create();
    if(handle==NULL) return 1;
    //First, set the reg address
    err|=i2c_master_start(handle);
    err|=i2c_master_write_byte(handle, (dev->i2c.addr << 1) | I2C_MASTER_WRITE, true);
    err|=i2c_master_write_byte(handle, start_addr, true);
    err|=i2c_master_stop(handle);
    err|=i2c_master_cmd_begin(i2c_num, handle, 1000/portTICK_PERIOD_MS);
    i2c_cmd_link_delete(handle);
    if(err!=ESP_OK) return 1;//return if error

    //then read data
    handle=i2c_cmd_link_create();
    if(handle==NULL) return 1;
    err|=i2c_master_start(handle);
    err|=i2c_master_write_byte(handle, (dev->i2c.addr << 1) | I2C_MASTER_READ, true);
    err|=i2c_master_read(handle, data_buf, read_num,I2C_MASTER_LAST_NACK);
    err|=i2c_master_stop(handle);
    err|=i2c_master_cmd_begin(i2c_num, handle, 1000/portTICK_PERIOD_MS);
    i2c_cmd_link_delete(handle);

    return err==ESP_OK ? 0:1;
}

//dev init
uint8_t ft6236_init(touch_panel_config_t *config)
{
    uint8_t ret=0;
    uint8_t cipher=0;//must be 0x64
    uint8_t firmid=0;//Firmware Version
    uint8_t data=0;

    g_dev.i2c.addr=config->i2c.addr;
    g_dev.i2c.i2c_num=config->i2c.i2c_num;
    g_dev.i2c.scl_pin=config->i2c.i2c_scl;
    g_dev.i2c.sda_pin=config->i2c.i2c_sda;
    g_dev.int_pin=config->pin_num_int;
    g_dev.rst_pin=config->pin_num_rst;
    g_dev.width=config->width;
    g_dev.height=config->height;
    ft6236_set_direction(config->direction);
    //bsp initialize and test
    ret += ft6236_interface_init(&g_dev);
    ret += ft6236_read_reg(&g_dev,FT6236_REG_FIRMID,1,&firmid);
    ret += ft6236_read_reg(&g_dev,FT6236_REG_CIPHER,1,&cipher);
    if(ret!=0 ||cipher!=0x64)//chip id changes with chips
    {
        ESP_LOGI(TAG,"Initialize interface fail.cipher=%x firmware version=%x.",cipher,firmid);
        return 1;
    }
    ESP_LOGI(TAG,"cipher=%x firmware version=%x.",cipher,firmid);
    //set the interrupt mod to polling(continuous low level)
    data=0;
    ret += ft6236_write_one_reg(&g_dev, FT6236_REG_G_MODE, data);
    //set the touch detect threshold(lower is sensitive)
    data=22;
    ret += ft6236_write_one_reg(&g_dev, FT6236_REG_TH_GROUP, data);

    return ret;
}

void ft6236_deinit(void)
{
    ft6236_interface_deinit(&g_dev);
}

uint8_t ft6236_set_direction(touch_panel_dir_t dir) {

    if (TOUCH_DIR_MAX < dir) {
        dir >>= 5;
    }
    g_dev.direction = dir;
    return 0;
}

/**
 * confirm if there is touch event
 * @return 1:touched 0:nothing
 */
uint8_t ft6236_is_press(void)
{
/**
 * @note There are two ways to determine weather the touch panel is pressed
 * 1. Read the IRQ line of touch controller
 * 2. Read number of points touched in the register
 */
    if (-1 != g_dev.int_pin) {
        return !gpio_get_level((gpio_num_t)g_dev.int_pin);
    }
    uint8_t num;
    ft6236_read_reg(&g_dev,FT6236_REG_TD_STATUS,1,&num);
    if((num&0x07) ==0)
        return 0;
    return 1;
}

static void ft6236_apply_rotate(uint16_t *x, uint16_t *y)
{
    uint16_t _x = *x;
    uint16_t _y = *y;

    switch (g_dev.direction) {
        case TOUCH_DIR_LRTB:
            *x = _x;
            *y = _y;
            break;
        case TOUCH_DIR_LRBT:
            *x = _x;
            *y = g_dev.height - _y;
            break;
        case TOUCH_DIR_RLTB:
            *x = g_dev.width - _x;
            *y = _y;
            break;
        case TOUCH_DIR_RLBT:
            *x = g_dev.width - _x;
            *y = g_dev.height - _y;
            break;
        case TOUCH_DIR_TBLR:
            *x = _y;
            *y = _x;
            break;
        case TOUCH_DIR_BTLR:
            *x = _y;
            *y = g_dev.width - _x;
            break;
        case TOUCH_DIR_TBRL:
            *x = g_dev.height - _y;
            *y = _x;
            break;
        case TOUCH_DIR_BTRL:
            *x = g_dev.height - _y;
            *y = g_dev.width - _x;
            break;

        default:
            break;
    }
}

uint8_t ft6236_get_points(touch_panel_points_t *info)
{
    uint8_t data_buf[14];
    uint8_t point_index=0;

    ft6236_read_reg(&g_dev,FT6236_REG_GEST_ID,14,data_buf);

    info->point_num =data_buf[1] &0x07;//get TD_STATUS
    if (info->point_num > 0 && info->point_num <= TOUCH_MAX_POINT_NUMBER) {
        info->gesture=data_buf[0];//get the gesture ID

        if(data_buf[4]>>4!=0x0F)//Touch ID is valid
        {
            info->event[point_index] = data_buf[2] >> 6;
            info->curx[point_index] = (((uint16_t)(data_buf[2]&0x0F) << 8) | data_buf[3]);
            info->cury[point_index] = (((uint16_t)(data_buf[4]&0x0F) << 8) | data_buf[5]);
            info->weight[point_index] = data_buf[6];
            ft6236_apply_rotate(&info->curx[point_index],&info->cury[point_index]);

            point_index++;
        }
        if(data_buf[10]>>4!=0x0F)//Touch ID is valid
        {
            info->event[point_index] = data_buf[8] >> 6;
            info->curx[point_index] = (((uint16_t)(data_buf[8]&0x0F) << 8) | data_buf[9]);
            info->cury[point_index] = (((uint16_t)(data_buf[10]&0x0F) << 8) | data_buf[11]);
            info->weight[point_index] = data_buf[12];
            ft6236_apply_rotate(&info->curx[point_index],&info->cury[point_index]);

            point_index++;
        }

        if(point_index==1){//clean the unused info
            info->curx[1] = 0;
            info->cury[1] = 0;
        }
//        ESP_LOGI(TAG,"Touch ID--1:%x,2:%x",data_buf[4],data_buf[10]);
        return info->point_num;
    } else {
        info->curx[0] = 0;
        info->cury[0] = 0;
        info->curx[1] = 0;
        info->cury[1] = 0;
    }

    return 0;
}