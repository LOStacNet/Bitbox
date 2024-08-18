#include "bitbox.h"

static const char* TAG = "Menu";

static InputEvent menu_event=INPUT_NO_EVENT,touch_x,touch_y;
static uint8_t SelectAppID=1;
static uint8_t counter=0;//counter for SelectAppID=0

static void menu_init(void);
static void menu_run(void);
static void menu_input_event_handle(InputEvent e,void *data);

uint8_t enter_menu(void)
{
    input_event_handle=menu_input_event_handle;//reset the input handle
    app_run=menu_run;//set the loop function
    menu_init();

    return 1;
}

static void menu_input_event_handle(InputEvent e,void *data)
{
    switch (e) {
        case SWITCH_DOWN_EVENT:
            menu_event=SWITCH_DOWN_EVENT;
            break;
        case ENCODER_CW_EVENT:
            menu_event=ENCODER_CW_EVENT;
            break;
        case ENCODER_CCW_EVENT:
            menu_event=ENCODER_CCW_EVENT;
            break;
        case TOUCH_EVENT:
            menu_event=TOUCH_EVENT;
            touch_x=((touch_panel_points_t*)data)->curx[0];
            touch_y=((touch_panel_points_t*)data)->cury[0];
            break;
        case SWITCH_LONG_DOWN_EVENT:
            menu_event=SWITCH_LONG_DOWN_EVENT;
            break;
        default:
            menu_event=INPUT_NO_EVENT;
            break;
    }
}

static void menu_init(void)
{
    SelectAppID=1;
    menu_event=INPUT_NO_EVENT;
    LEDPlaneFill(0);
    LEDPlaneShow();
}

static void menu_run(void)
{
    //draw app image
    LEDPlaneFill(0);
    if(SelectAppID==0){//draw developer's Name
        for(uint8_t i=0;i<LED_PLANE_X;i++){
            LEDPlaneDrawRGB(i,0,10,10,10);
            LEDPlaneDrawRGB(i,LED_PLANE_Y-1,10,10,10);
        }
        for(uint8_t i=1;i<LED_PLANE_Y-1;i++){
            LEDPlaneDrawRGB(0,i,10,10,10);
            LEDPlaneDrawRGB(LED_PLANE_X-1,i,10,10,10);
        }
        //draw LO
        for(uint8_t i=3;i<10;i++){
            LEDPlaneDrawRGB(3,i,0,0,100);
        }
        for(uint8_t i=3;i<7;i++){
            LEDPlaneDrawRGB(i,9,0,0,100);
        }
        for(uint8_t i=3;i<10;i++){
            LEDPlaneDrawRGB(10,i,0,0,100);
            LEDPlaneDrawRGB(14,i,0,0,100);
        }
        for(uint8_t i=10;i<14;i++){
            LEDPlaneDrawRGB(i,3,0,0,100);
            LEDPlaneDrawRGB(i,9,0,0,100);
        }
        //draw move point
        if(counter<17) LEDPlaneDrawRGB(counter,0,10,0,0);
        else if(counter<28) LEDPlaneDrawRGB(17,counter-17,10,0,0);
        else if(counter<45) LEDPlaneDrawRGB(17-(counter-28),11,10,0,0);
        else LEDPlaneDrawRGB(0,11-(counter-45),10,0,0);

        if(counter<55){
            counter++;
        } else counter=0;
    }
    else{//draw select screen
        //draw print app
        for(uint8_t i=0;i<9;i++){
            LEDPlaneDrawRGB(i,0,100,90,50);
            LEDPlaneDrawRGB(i,8,100,90,50);
        }
        for(uint8_t i=1;i<8;i++){
            LEDPlaneDrawRGB(0,i,100,90,50);
            LEDPlaneDrawRGB(8,i,100,90,50);
        }
        LEDPlaneDrawRGB(2,2,200,190,11);
        LEDPlaneDrawRGB(3,2,200,190,11);
        LEDPlaneDrawRGB(2,3,200,190,11);
        LEDPlaneDrawRGB(3,3,200,190,11);
        LEDPlaneDrawRGB(3,4,200,190,11);
        LEDPlaneDrawRGB(4,3,200,190,11);
        LEDPlaneDrawRGB(4,4,200,190,11);
        LEDPlaneDrawRGB(4,5,200,190,11);
        LEDPlaneDrawRGB(5,4,200,190,11);
        LEDPlaneDrawRGB(5,5,150,150,150);
        LEDPlaneDrawRGB(6,6,150,150,150);
        LEDPlaneDrawRGB(7,5,55,200,173);
        LEDPlaneDrawRGB(7,4,55,200,173);
        LEDPlaneDrawRGB(6,3,55,200,173);
        LEDPlaneDrawRGB(7,2,55,200,173);
        //draw brick app
        for(uint8_t i=9;i<18;i++){
            LEDPlaneDrawRGB(i,0,50,90,100);
            LEDPlaneDrawRGB(i,8,50,90,100);
        }
        for(uint8_t i=1;i<8;i++){
            LEDPlaneDrawRGB(9,i,50,90,100);
            LEDPlaneDrawRGB(17,i,50,90,100);
        }
        for(uint8_t i=10;i<17;i++){
            LEDPlaneDrawRGB(i,1,0,100,0);
            LEDPlaneDrawRGB(i,2,0,100,0);
        }
        LEDPlaneDrawRGB(10,2,0,0,0);
        LEDPlaneDrawRGB(14,2,0,0,0);
        LEDPlaneDrawRGB(13,5,100,0,0);
        for(uint8_t i=11;i<15;i++){
            LEDPlaneDrawRGB(i,7,0,0,100);
        }
        //draw select display
        if(SelectAppID==1){
            for(uint8_t i=0;i<9;i++)
                for(uint8_t j=9;j<11;j++){
                    LEDPlaneDrawRGB(i,j,0,100,0);
                }
        }
        else if(SelectAppID==2){
            for(uint8_t i=9;i<18;i++)
                for(uint8_t j=9;j<11;j++){
                    LEDPlaneDrawRGB(i,j,0,100,0);
                }
        }
    }
    LEDPlaneShow();

    //process input
    switch (menu_event) {
        case SWITCH_DOWN_EVENT://enter the selected app
            if(SelectAppID!=0)
                APPExit(SelectAppID);
            else{
                SelectAppID=1;
            }
            menu_event=INPUT_NO_EVENT;
            break;
        case SWITCH_LONG_DOWN_EVENT://show the developer(select id=0)
            SelectAppID=0;
            menu_event=INPUT_NO_EVENT;
            break;
        case ENCODER_CW_EVENT://appid + 1
            if(SelectAppID!=0)
                SelectAppID = SelectAppID==MAX_APPID-1 ? 1:SelectAppID+1;
            else{
                SelectAppID=1;
            }
            menu_event=INPUT_NO_EVENT;
            break;
        case ENCODER_CCW_EVENT://appid - 1
            if(SelectAppID!=0)
                SelectAppID = SelectAppID==1 ? MAX_APPID-1:SelectAppID-1;
            else{
                SelectAppID=1;
            }
            menu_event=INPUT_NO_EVENT;
            break;
        case TOUCH_EVENT://set appid based on touchX
            if(SelectAppID!=0){
                SelectAppID = touch_x>8 ? 2:1;
            }
            else{
                SelectAppID=1;
            }
            menu_event=INPUT_NO_EVENT;
            break;
        default:
            menu_event=INPUT_NO_EVENT;
            break;
    }
    vTaskDelay(100/portTICK_PERIOD_MS);
}