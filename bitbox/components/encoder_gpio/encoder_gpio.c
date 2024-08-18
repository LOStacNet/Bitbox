#include "encoder.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/gpio_filter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static const char *TAG ="encoder_gpio";

typedef enum{
    KEY_STATUS_IDLE,     /* IDLE */
    KEY_STATUS_PRESSED,  /* detect press,wait to confirm */
    KEY_STATUS_DOWN,     /* Key down confirmed */
    KEY_STATUS_LONG      /* Key long down */
}KEY_Status;

static uint8_t key_count=0;//conut to 10 before next status
static KEY_Status key_status=KEY_STATUS_IDLE;
static gptimer_handle_t key_timer;
static QueueHandle_t encoder_gpio_event_queue=NULL;//Use for msg from ISR
static uint8_t edgecount=0,as1,as2,bs1,bs2;//statues of A and B in two check
static gpio_glitch_filter_handle_t gpiofilter_S,gpiofilter_A,gpiofilter_B;


static bool key_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{
    BaseType_t high_task_awoken = pdFALSE;
    uint8_t key_read;
    Encoder_Event msg;

    key_read= !gpio_get_level(ENCODER_S_PIN);//active low
    switch (key_status) {
        case KEY_STATUS_IDLE:
            if(key_read){
                key_status=KEY_STATUS_PRESSED;
            }
            else {
                key_status = KEY_STATUS_IDLE;
            }
            break;
        case KEY_STATUS_PRESSED:
            if(key_read){
                key_status=KEY_STATUS_DOWN;
            }else{
                key_status=KEY_STATUS_IDLE;
            }
            break;
        case KEY_STATUS_DOWN:
            if(key_read){
                key_count++;
                if(key_count==10){//active key long down event
                    key_count=0;
                    key_status=KEY_STATUS_LONG;
                    msg=ENCODER_SW_LONG;
                    xQueueOverwriteFromISR(encoder_gpio_event_queue,&msg,&high_task_awoken);
                } else{
                    key_status=KEY_STATUS_DOWN;
                }
            } else{//active key down event when realise
                key_status=KEY_STATUS_IDLE;
                msg=ENCODER_SW_DOWN;
                xQueueOverwriteFromISR(encoder_gpio_event_queue,&msg,&high_task_awoken);
            }
            break;
        case KEY_STATUS_LONG:
            if(key_read){
                key_status=KEY_STATUS_LONG;
            } else{
                key_status=KEY_STATUS_IDLE;
            }
            break;
        default:
            break;
    }

    return high_task_awoken==pdTRUE;
}

/**
 * the dir interrupt function
 * @param arg
 */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
//    uint32_t gpio_num = (uint32_t) arg;//not use
    Encoder_Event msg;//result msg

    if(edgecount==0){//first edge
        bs1= gpio_get_level(ENCODER_B_PIN);
        as1= gpio_get_level(ENCODER_A_PIN);
        edgecount=1;
    }
    else{
        bs2= gpio_get_level(ENCODER_B_PIN);
        as2= gpio_get_level(ENCODER_A_PIN);

        if((as1==1 && bs1==0 && as2==0 && bs2==1) || (as1==0 && bs1==1 && as2==1 && bs2==0)){
            msg=ENCODER_CW;
            xQueueOverwriteFromISR(encoder_gpio_event_queue,&msg,NULL);
        }
        else if((as1==1 && bs1==1 && as2==0 && bs2==0) || (as1==0 && bs1==0 && as2==1 && bs2==1)){
            msg=ENCODER_CCW;
            xQueueOverwriteFromISR(encoder_gpio_event_queue,&msg,NULL);
        }
        edgecount=0;
    }
}

/**
 * Initialize the pin
 * @return 0:SUCCESS 1:FAILURE
 */
uint8_t encoder_init(void)
{
    uint8_t ret=0;
    gpio_config_t config;
    gpio_pin_glitch_filter_config_t filter;

    /*create gpio filter */
    filter.clk_src=GLITCH_FILTER_CLK_SRC_DEFAULT;
    filter.gpio_num=ENCODER_S_PIN;
    ret|= gpio_new_pin_glitch_filter(&filter,&gpiofilter_S);
    ret|= gpio_glitch_filter_enable(gpiofilter_S);
    filter.gpio_num=ENCODER_A_PIN;
    ret|= gpio_new_pin_glitch_filter(&filter,&gpiofilter_A);
    ret|= gpio_glitch_filter_enable(gpiofilter_A);
    filter.gpio_num=ENCODER_B_PIN;
    ret|= gpio_new_pin_glitch_filter(&filter,&gpiofilter_B);
    ret|= gpio_glitch_filter_enable(gpiofilter_B);

    /*SW and B just input pull up,no interrupt */
    config.pin_bit_mask=(1ULL<<ENCODER_S_PIN)|(1ULL<<ENCODER_B_PIN);
    config.mode=GPIO_MODE_INPUT;
    config.intr_type=GPIO_INTR_DISABLE;
    config.pull_down_en=GPIO_PULLDOWN_DISABLE;
    config.pull_up_en=GPIO_PULLUP_ENABLE;
    ret|= gpio_config(&config);

    /* input A have interrupt */
    config.pin_bit_mask=(1ULL<<ENCODER_A_PIN);
    config.intr_type=GPIO_INTR_ANYEDGE;//double edge
    ret|= gpio_config(&config);

    /*install isr,not use gpio_isr_register(),for it will register a single handler for all pins*/
    ret|= gpio_install_isr_service(0);
    ret|= gpio_isr_handler_add(ENCODER_A_PIN,gpio_isr_handler,(void*)ENCODER_A_PIN);

    /*create timer and add interrupt*/
    gptimer_config_t timer= {
            .clk_src = GPTIMER_CLK_SRC_DEFAULT,
            .direction=GPTIMER_COUNT_UP,
            .resolution_hz=10000,//10000 Hz,1 tick =0.1 ms,must beyond  (80MHz/65535=)1221Hz.
    };
    ret|= gptimer_new_timer(&timer,&key_timer);
    gptimer_alarm_config_t alarm_config={
            .reload_count=0,//restart when alarm
            .alarm_count=1000,//100ms in 10000Hz
            .flags.auto_reload_on_alarm=true,//enable auto-reload
    };
    ret|= gptimer_set_alarm_action(key_timer,&alarm_config);
    gptimer_event_callbacks_t cbs={
            .on_alarm= key_timer_cb,//key status manager
    };
    ret|= gptimer_register_event_callbacks(key_timer,&cbs,NULL);
    ret|= gptimer_enable(key_timer);
    ret|= gptimer_start(key_timer);

    /*create the msg queue*/
    encoder_gpio_event_queue= xQueueCreate(1,sizeof(Encoder_Event));//length is 1,each element is Encoder_Event

    return ret;
}

void encoder_deinit(void)
{
    //del filter
    gpio_glitch_filter_disable(gpiofilter_S);
    gpio_del_glitch_filter(gpiofilter_S);
    gpio_glitch_filter_disable(gpiofilter_A);
    gpio_del_glitch_filter(gpiofilter_A);
    gpio_glitch_filter_disable(gpiofilter_B);
    gpio_del_glitch_filter(gpiofilter_B);
    //del timer
    gptimer_stop(key_timer);
    gptimer_disable(key_timer);
    gptimer_del_timer(key_timer);
    //del isr
    gpio_isr_handler_remove(ENCODER_A_PIN);
    gpio_uninstall_isr_service();
    //del queue
    vQueueDelete(encoder_gpio_event_queue);
}

/**
 * read encoder
 * @param timeout time in ms to wait,0 for no blocking
 * @return
 */
Encoder_Event encoder_read(uint8_t timeout)
{
    Encoder_Event ret;
    if(pdTRUE== xQueueReceive(encoder_gpio_event_queue,&ret,timeout/portTICK_PERIOD_MS)){
        ESP_LOGI(TAG,"Encoder:%d",ret);
        return ret;
    } else{//no msg in queue
        return ENCODER_NONE;
    }
}
