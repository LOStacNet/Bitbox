#include "bitbox.h"
#include "math.h"

static const char* TAG = "Print";

static InputEvent print_event=INPUT_NO_EVENT,touch_x,touch_y;
static uint16_t pen_color=0XFFFF;
static int16_t pen_colors[3]={200,200,200};//0-r 1-g 2-b
static uint8_t select_color=0;//0-r 1-g 2-b

static void print_init(void);
static void print_input_event_handle(InputEvent e,void *data);
static void print_run(void);

uint8_t enter_print(void)
{
    input_event_handle=print_input_event_handle;
    app_run=print_run;
    print_init();

    return 1;
}

static void print_init(void)
{
    pen_color=0XFFFF;
    print_event=INPUT_NO_EVENT;
    LEDPlaneFill(0);
    LEDPlaneShow();
}

static void print_input_event_handle(InputEvent e,void *data)
{
    switch (e) {
        case SWITCH_DOWN_EVENT:
            print_event=SWITCH_DOWN_EVENT;
            break;
        case ENCODER_CW_EVENT:
            print_event=ENCODER_CW_EVENT;
            break;
        case ENCODER_CCW_EVENT:
            print_event=ENCODER_CCW_EVENT;
            break;
        case TOUCH_EVENT:
            print_event=TOUCH_EVENT;
            touch_x=((touch_panel_points_t*)data)->curx[0];
            touch_y=((touch_panel_points_t*)data)->cury[0];
            break;
        default:
            print_event=INPUT_NO_EVENT;
            break;
    }
}

static void print_run(void)
{
    //draw screen
    //clean the bottom
    for(uint8_t i=0;i<18;i++)
        for(uint8_t j=10;j<12;j++){
            LEDPlaneDraw(i,j,0);
        }
    //draw select notice
    for(uint8_t i=0+select_color*6;i<6+select_color*6;i++)
        LEDPlaneDraw(i,10,pen_color);
    //draw color notice
    uint8_t color_num;
    color_num=(uint8_t)round(pen_colors[0]*6.0/200);
    for(uint8_t i=0;i<color_num;i++)
        LEDPlaneDrawRGB(i,11,100,0,0);
    color_num=(uint8_t)round(pen_colors[1]*6.0/200);
    for(uint8_t i=6;i<6+color_num;i++)
        LEDPlaneDrawRGB(i,11,0,100,0);
    color_num=(uint8_t)round(pen_colors[2]*6.0/200);
    for(uint8_t i=12;i<12+color_num;i++)
        LEDPlaneDrawRGB(i,11,0,0,100);
    LEDPlaneShow();

    //process input
    switch (print_event) {
        case SWITCH_DOWN_EVENT://clean the paint
            LEDPlaneFill(0);
            print_event=INPUT_NO_EVENT;
            break;
        case ENCODER_CW_EVENT://the selected color + 10
            if(pen_colors[select_color]>=190){
                pen_colors[select_color]=200;
            }else{
                pen_colors[select_color]=pen_colors[select_color]+10;
            }
            pen_color=(uint16_t)pen_colors[0]<<11|(uint16_t)pen_colors[1]<<5|(uint16_t)pen_colors[2]<<0;

            print_event=INPUT_NO_EVENT;
            break;
        case ENCODER_CCW_EVENT://the selected color - 10
            if(pen_colors[select_color]<=10){
                pen_colors[select_color]=0;
            }else{
                pen_colors[select_color]=pen_colors[select_color]-10;
            }
            pen_color=(uint16_t)pen_colors[0]<<11|(uint16_t)pen_colors[1]<<5|(uint16_t)pen_colors[2]<<0;

            print_event=INPUT_NO_EVENT;
            break;
        case TOUCH_EVENT://change selected color or draw
            if(touch_y==11||touch_y==10){//touch on bottom
                if(touch_x<=5){//select R
                    select_color=0;
                }
                else if(touch_x<=11){//select G
                    select_color=1;
                }
                else if(touch_x<=17){//select B
                    select_color=2;
                }
            }
            else{//draw a point
                LEDPlaneDraw(touch_x,touch_y,pen_color);
            }

            print_event=INPUT_NO_EVENT;
            break;
        default:
            print_event=INPUT_NO_EVENT;
            break;
    }

    vTaskDelay(100/portTICK_PERIOD_MS);
}