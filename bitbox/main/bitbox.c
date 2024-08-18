#include "bitbox.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "math.h"

static const char* TAG = "Main";

uint8_t Global_AppID=0;
uint8_t (*entrance[MAX_APPID])(void);//app init function
void (*app_run)(void);//app function in loop

TaskHandle_t ledtask;
QueueHandle_t led_refresh_req=NULL;// transfer screen refresh request
void LEDPlaneTask(void *pvParameters);

TaskHandle_t inputtask;
void InputTask(void *pvParameters);
void (*input_event_handle)(InputEvent e,void *data);//input handle
void default_input_event_handle(InputEvent e,void *data);

void app_main(void)
{
    vTaskDelay(2000/portTICK_PERIOD_MS);
    ESP_LOGI(TAG,"Start app_main...");

    //Led plane Task
    xTaskCreate(LEDPlaneTask,"LEDPlane",2048,NULL,1,&ledtask);
    xTaskCreate(InputTask,"InputTask",2048,NULL,1,&inputtask);

    vTaskDelay(500/portTICK_PERIOD_MS);//wait all things done
    /* Register Apps */
    entrance[0]=enter_menu;
    entrance[1]=enter_print;
    entrance[2]=enter_brick;
    /* Start default App */
    APPEnter(0);
    ESP_LOGI(TAG,"app_main initial end.");
    while(1)
    {
        app_run();
//        ESP_LOGI(TAG,"log in while.");
        vTaskDelay(10/portTICK_PERIOD_MS);//must add to avoid watch dog
    }
}

/**
 * Flush the LEDPlane in the background
 * @param pvParameters
 */
void LEDPlaneTask(void *pvParameters)
{
    uint8_t ret;
    led_refresh_req=xQueueCreate(1,sizeof(uint8_t));//length is 1,send anything for refresh
    LEDPlaneInit();
    while (1){
        if(pdTRUE== xQueueReceive(led_refresh_req,&ret,100/portTICK_PERIOD_MS)){
            LEDPlaneFlush();
        }
    }
}

/**
 * send a req for refresh
 */
void LEDPlaneShow(void)
{
    uint8_t msg=1;
    xQueueOverwrite(led_refresh_req,&msg);
}

/**
 * Handel touch plane
 * @param pvParameters
 */
void InputTask(void *pvParameters)
{
    uint8_t ret=0;
    touch_panel_points_t info={0};

    //initialize encoder
    ret+=encoder_init();

    //initialize touch
    touch_panel_config_t config={
            .direction=TOUCH_DIR_TBRL,
            .pin_num_rst=5,
            .pin_num_int=4,
            .height=480,
            .width=320,
            .i2c.addr=0x38,
            .i2c.i2c_num=0,
            .i2c.i2c_sda=6,
            .i2c.i2c_scl=7
    };
    ret+=ft6236_init(&config);

    //set default handle
    input_event_handle=default_input_event_handle;

    if(ret != 0) ESP_LOGI(TAG,"input initial error.");
    else ESP_LOGI(TAG,"input initial ok.");

    while (1){
        //encoder event
        Encoder_Event enc= encoder_read(0);
        if(enc!=ENCODER_NONE){
            if(enc==ENCODER_SW_LONG && Global_AppID!=0){//reserve long down event,to enter default app 0
                APPExit(0);
            }else {
                input_event_handle(enc, NULL);
            }
        }
        //touch event
        if(ft6236_is_press()){
            ft6236_get_points(&info);
            if(info.point_num!=0) {
                TouchToPlane(&info.curx[0], &info.cury[0]);
                input_event_handle(TOUCH_EVENT, &info);
            }
        }
        vTaskDelay(10/portTICK_PERIOD_MS);//make time for other task
    }
}

uint8_t TouchToPlane(uint16_t *x,uint16_t *y)
{
    if(*x>=480 || *y>=320) return 1;

    *x=(uint16_t)round(1.0* *x * (LED_PLANE_X-1)/480);
    *y=(uint16_t)round(1.0* *y * (LED_PLANE_Y-1)/320);
    return 0;
}

void APPEnter(uint8_t app_id)
{
    if(app_id>=MAX_APPID)
        return;
    Global_AppID=app_id;
    entrance[app_id]();
}

void APPExit(uint8_t app_id)
{
    if(Global_AppID==app_id || app_id>=MAX_APPID)
        return;
    APPEnter(app_id);
}

void default_input_event_handle(InputEvent e,void *data)
{
    portNOP();
}



