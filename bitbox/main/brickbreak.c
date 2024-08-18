#include "bitbox.h"

#define PADDLE_LEN (5)
static const char* TAG = "Brick";

typedef enum{
    GAME_RUN,
    GAME_STOP
}Game_Status;
static Game_Status game_status;

static uint8_t paddleX;//paddle's rightest pixel x coordinate
static uint8_t ballX,ballY;//the coordinate of ball
static uint8_t ballVX,ballVY;//the speed of ball
static uint8_t bricks[18][12];//bricks list
static InputEvent brick_event=INPUT_NO_EVENT,touch_x,touch_y;

static void brick_init(void);
static void brick_input_event_handle(InputEvent e,void *data);
static void brick_run(void);

/**
 * need to be called when first enter
 */
uint8_t enter_brick(void)
{

    input_event_handle=brick_input_event_handle;//reset the input handle
    app_run=brick_run;//set the loop function
    brick_init();

    return 1;
}

/**
 * just initialize the game
 */
static void brick_init(void)
{
    /* Set initial value */
    ballX=8;ballY=9;
    ballVX=1;ballVY=-1;
    paddleX=0;
    for (uint8_t i = 0; i < 18; i++) //set the top 3 lines
        for (uint8_t j = 0; j < 12; j++) {
            bricks[i][j] = j<3 ? 1:0;
        }
    /* clean led plane */
    LEDPlaneFill(0);
    LEDPlaneShow();
    brick_event=INPUT_NO_EVENT;
    game_status=GAME_RUN;
}

static void brick_input_event_handle(InputEvent e,void *data)
{
    switch (e) {
        case SWITCH_DOWN_EVENT:
            brick_event=SWITCH_DOWN_EVENT;
            break;
        case ENCODER_CW_EVENT:
            brick_event=ENCODER_CW_EVENT;
            break;
        case ENCODER_CCW_EVENT:
            brick_event=ENCODER_CCW_EVENT;
            break;
        case TOUCH_EVENT:
            brick_event=TOUCH_EVENT;
            touch_x=((touch_panel_points_t*)data)->curx[0];
            touch_y=((touch_panel_points_t*)data)->cury[0];
            break;
        default:
            brick_event=INPUT_NO_EVENT;
            break;
    }
}

static void brick_run(void)
{
    if(game_status==GAME_RUN) {
        // move the ball
        ballX += ballVX;
        ballY += ballVY;

        // Check for collision with walls,reverse move direction
        if (ballX == 0 || ballX == LED_PLANE_X - 1) {
            ballVX *= -1;
        }
        if (ballY == 0) {
            ballVY *= -1;
        }

        // Check for collision with paddle,reverse Y speed
        if (ballY == LED_PLANE_Y - 2  && ballX >= paddleX && ballX <= paddleX + PADDLE_LEN-1) {
            ballVY *= -1;
            //change ballX randomly
            if(ballX>1 && ballX < LED_PLANE_X-2) {// not change X to 0 or MAX
                uint8_t t=(uint8_t) esp_random() % 150;

                if(t<50) ballX=ballX-1;
                else if(t<100) ballX=ballX+1;
                else ballX=ballX+0;
            }
        }

        //check for collision with bricks,reverse Y speed
        if (ballY>=0 && ballY<=11 && bricks[ballX][ballY]) {
            bricks[ballX][ballY] = 0;
            if(ballY!=0) // reverse when not touch wall(very important or game will end wrongly)
                ballVY *= -1;
        }

        //print the game image
        LEDPlaneFill(0);
        //draw bricks
        for (uint8_t i = 0; i < 18; i++) //draw all plane
            for (uint8_t j = 0; j < 12; j++) {
                if (bricks[i][j]) {
                    LEDPlaneDrawRGB(i, j, 0, 100, 0);
                }
            }
        //draw ball
        LEDPlaneDrawRGB(ballX, ballY, 100, 0, 0);
        //draw paddle,on the bottom
        for (int i = 0; i < PADDLE_LEN; i++) {
            LEDPlaneDrawRGB(paddleX + i, LED_PLANE_Y - 1, 0, 0, 100);
        }
        LEDPlaneShow();

        // check touch and move paddle
        switch (brick_event) {
            case TOUCH_EVENT:
                //paddle X must be limited,for it will be out of plane if not.
                paddleX = touch_x <= (LED_PLANE_X-PADDLE_LEN) ? touch_x : LED_PLANE_X-PADDLE_LEN;
                brick_event=INPUT_NO_EVENT;
                break;
            case ENCODER_CW_EVENT:
                //paddle X add 3
                paddleX = paddleX+3 <= (LED_PLANE_X-PADDLE_LEN) ? paddleX+3 : LED_PLANE_X-PADDLE_LEN;
                brick_event=INPUT_NO_EVENT;
                break;
            case ENCODER_CCW_EVENT:
                //paddle X sub 3
                paddleX = paddleX < 3 ? paddleX : paddleX-3;
                brick_event=INPUT_NO_EVENT;
                break;
            default:
                break;
        }

        // if ball hits bottom, game over
        if (ballY >= LED_PLANE_Y) {
            //play animation
            for (uint8_t i = 0; i < 5; i++) {
                LEDPlaneFill(0);
                LEDPlaneShow();
                vTaskDelay(100 / portTICK_PERIOD_MS);
                LEDPlaneFill(4453);
                LEDPlaneShow();
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
            game_status=GAME_STOP;
        }

        // if all bricks are gone,player win
        uint8_t brick_sum=0;
        for (uint8_t i = 0; i < 18; i++) //add all bricks
            for (uint8_t j = 0; j < 12; j++) {
                brick_sum+=bricks[i][j];
            }
        if(brick_sum==0){//no brick,win
            //play animation
            for (uint8_t i = 0; i < 5; i++) {
                LEDPlaneFill(0);
                LEDPlaneShow();
                vTaskDelay(100 / portTICK_PERIOD_MS);
                LEDPlaneFill(31264);
                LEDPlaneShow();
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
            game_status=GAME_STOP;
        }

        vTaskDelay(100/ portTICK_PERIOD_MS);
    }
    else if(game_status==GAME_STOP){// in stop mode,restart when button down
        if(brick_event==SWITCH_DOWN_EVENT){
            brick_event=INPUT_NO_EVENT;
            brick_init();
        }
    }
}
