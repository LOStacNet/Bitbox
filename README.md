# 像素盒子开发笔记

该项目是基于像素盒子开源项目硬件的简单再次开发，为其包含外设编写了驱动组件，并实现了一套简单的扩展框架，能够较为简单的添加新的功能APP。

本人制作的实物只包含了触摸屏，灯板和编码器。基于合宙ESP32C3精简版。

源开发环境为Arduino，这里开发环境是ESP-IDF。

![IMG_20240829_130032_new](README/lo.jpg)

![IMG_20240829_130043_new](README/menu.jpg)

## 开发环境

硬件：像素盒子[开源链接](https://oshwhub.com/shukkkk/led-chu-mo-ban)

软件：IDF-v5.1.1，CLion

## 功能描述

本项目在原实物条件下，使用ESP-IDF环境重新开发。

实现功能如下：

* 电子画板

电子画板功能。屏幕下方用RGB三色彩条调色，可双点绘画，短按编码器清零，长按退出。

* 打砖块游戏

修改原打砖块游戏。可触摸控制反弹板，短按编码器重新开始，长按退出。

* 选择菜单

开机后进入选择菜单。可以利用编码器(或触控)选择要使用的功能，短按编码器确认选择，长按编码器显示滚动字幕打印开发者，短按退出滚动字幕。

## 相关外设的使用

### RMT外设

详见博客文章。

### 灯板的驱动

[RMT的使用](https://lostacnet.top/post/32304/)

基于RMT外设，可以编写灯板的驱动。根据硬件的设置，这里使用一个数组包含全部的显示数据，使用一个task不断将其刷新到灯珠中。

```c
#define LED_PLANE_X 18
#define LED_PLANE_Y 12
//form right to left,high to low
static uint8_t led_plane_pixels[LED_PLANE_X][LED_PLANE_Y][3];
```

该数组的索引是以右上角为原点，向左为x轴正方向，向下为y轴正方向进行的坐标对应。

### IIC触摸屏

本项目使用FT6236触摸屏，具体驱动编写见相关[博客文章](https://lostacnet.top/post/43032/)。

### 编码器的使用

SIQ-02FVS3编码器，详见[博客文章](https://lostacnet.top/post/81306/)。

## 方向与坐标

这里规定，以**Type-C口朝下**为正方向。

此时以左上角为原点，向左为x轴正方向，向下为y轴正方向。

此时，屏幕的height为480，width为320。灯板的x轴坐标为0-17，y轴坐标为0-11。触摸板的x轴坐标为0-480，y轴坐标为0-320。

### 灯板的方向坐标

具体的驱动在上面的博客文章中给出了，这里主要说明一下方向坐标问题。

首先，需要添加一个用来画点的函数：
```c
static void extractRGB565(uint16_t color, uint8_t *r, uint8_t *g, uint8_t *b) {
    *r = (color & 0xF800) >> 11;  // 提取红色分量
    *g = (color & 0x07E0) >> 5;   // 提取绿色分量
    *b = color & 0x001F;          // 提取蓝色分量

    *r = *r << 3;  // 将红色分量扩展到 8 位
    *g = *g << 2;  // 将绿色分量扩展到 8 位
    *b = *b << 3;  // 将蓝色分量扩展到 8 位
}

/**
 * Draw a point on the plane,not flush
 * @param x
 * @param y
 * @param color color in RGB565
 * @return 0:success 1:failure
 */
uint8_t LEDPlaneDraw(uint8_t x,uint8_t y,uint16_t color)
{
    uint8_t _x,_y;
    uint8_t r,g,b;

    if(x>LED_PLANE_X || y>LED_PLANE_Y) return 1;

    _x=LED_PLANE_X-1-x;_y=y;
    extractRGB565(color, &r, &g, &b);
    led_plane_pixels[_x][_y][0]=g;
    led_plane_pixels[_x][_y][1]=r;
    led_plane_pixels[_x][_y][2]=b;

    return 0;
}

```

由于灯板的原始坐标(见外设章节)与实际使用的不一样，因此需要进行一次转化:
```c
 _x=LED_PLANE_X-1-x;_y=y;
```

### 触摸板的方向坐标

触摸板原始数据的坐标是以左上角为原点，右下角为最大点。且横为y，纵轴为x。

那么根据方向选择，应该选择`TOUCH_DIR_TBRL`方向进行转化。

### 坐标转化

将触摸板的坐标向灯板转换时，使用以下方法：

```c
uint8_t TouchToPlane(uint16_t *x,uint16_t *y)
{
    if(*x>=480 || *y>=320) return 1;

    *x=(uint16_t)round(1.0* *x * (LED_PLANE_X-1)/480);
    *y=(uint16_t)round(1.0* *y * (LED_PLANE_Y-1)/320);
    return 0;
}
```

## 工程结构

### 文件结构

```
->Bitbox
-->bitbox:IDF工程
-->datasheet:项目可能用到的参考手册
-->example:原项目例程(Arduino)
-->hardware:硬件原理图
```

### IDF工程结构

**components-组件**

* led_plane

该组件为WS2812灯板的驱动，基于ESP32C3的RMT外设，具体细节见实际代码，这里需要注意几个API。

```c
void LEDPlaneInit(void);//初始化函数，使用前需要调用
void LEDPlaneFlush(void);//刷新函数，将内存中的图像数据刷新到灯板上
uint8_t LEDPlaneDrawRGB(uint8_t x,uint8_t y,uint8_t r,uint8_t g,uint8_t b);//画点函数，RAWRGB，将点的数据写到内存中，不刷新
uint8_t LEDPlaneDraw(uint8_t x,uint8_t ,uint16_t color);//画点函数，使用RGB565格式
void LEDPlaneFill(uint16_t color);//刷屏函数，将所有点设为同一颜色。不刷新到灯板
```

* ft6236

触摸屏的驱动，基于ESP32C3的IIC外设。需要注意的API：

```c
uint8_t ft6236_init(touch_panel_config_t *config);//初始化函数，使用前调用
void ft6236_deinit(void);//反初始化
uint8_t ft6236_set_direction(touch_panel_dir_t dir);//设置屏幕方向，涉及坐标转换
uint8_t ft6236_is_press(void);//检测是否有触摸点
uint8_t ft6236_get_points(touch_panel_points_t *info);//获取触摸点信息
```

* encoder_gpio

编码器的驱动库。基于中断和队列传输数据。

```c
uint8_t encoder_init(void);//初始化函数，使用前调用
void encoder_deinit(void);
Encoder_Event encoder_read(uint8_t timeout);//读取。timeout为零时不阻塞，直接返回无输入事件
```

**main-主程序**

* `bitbox.h`

总头文件，声明了各种共用API和数据结构。

* `bitbox.c`

主程序。包括任务的创建，各种公用API的实现等等。

* `*.c`

APP文件，一个APP对应一个文件，其中菜单也是一个APP。

### 软件结构

本程序所有功能以APP为单位组成，菜单为一个特殊的APP。所有APP调用主程序提供接口获取输入信息，并使用主程序提供的LED刷新函数刷新显示。主程序提供的服务如下：

* APP的加载和切换

向APP程序提供基本框架，让APP程序能较为简单的添加到程序中，并能相互切换。该功能主要通过函数指针实现。`APPEnter()`和`APPExit（）`会转换APP，区别在于，后者会验证要转换的appid是否是当前正在执行的app。~~好像这样设计没啥用~~

* LED灯板的刷新管理

由于刷新一次灯板比较耗费时间，为了提高程序的响应速度，主程序将实际的刷新函数`LEDPlaneFlush()`放到了`LEDPlaneTask`中。当需要刷新时，APP只需要调用`LEDPlaneShow()`即可实现无阻塞刷新。

* 输入管理

为提高输入响应速度，屏蔽底层两个不同输入读取API的不同，同时让主程序也能在APP运行时响应特定输入，主程序使用了一个`InputTask`统一管理输入。该任务会不断轮询读取编码器和触摸屏输入，并在读取到输入后调用APP注册的回调函数传递输入数据。

* 触摸屏坐标和灯板坐标的转换

提供触摸屏原始数据转换为灯板坐标的API函数`TouchToPlane()`，APP一般不需要调用，因为`InputTask`会将转换好的数据传递给回调。

## 添加一个APP

当需要添加一个新的程序时，需要完成以下步骤：

1. 在`bitbox.h`中修改`MAX_APPID`为APP总数。
2. 实现`xxx_run()`函数和`xxx_input_event_handle()`函数。前者为APP运行在loop中的函数。会在`app_main()`的循环中执行；后者为输入事件处理回调函数，会在`InputTask`中调用执行。
3. 实现一个入口函数`enter_xxx()`，该函数会在程序切换时调用，用来初始化APP，并注册步骤2中的两个函数(给函数指针赋值)。
4. 修改`app_main()`中`Register Apps`部分，将上一步的入口函数添加到相应的数组中。
5. 修改`menu.c`菜单，将自己的APP添加到菜单中。
6. 需要绘制图像时，调用驱动中的画图函数，并用`LEDPlaneShow()`函数刷新到屏幕，**不要使用`LEDPlaneFlush()`**。
7. 所有输入通过`xxx_input_event_handle()`回调传递给app，建议在该函数中读取储存，在run函数中处理。
8. 使用`APPExit()`切换到其他APP。

具体的实现示例可以参考例程。

## 主程序的实现

### 屏幕刷新管理

屏幕刷新创建了一个task，该task负责初始化屏幕和执行`LEDPlaneFlush()`耗时刷新函数。该函数会检测一个`queue(led_refresh_req)`，当该队列有东西时，就会执行刷新函数。

`LEDPlaneShow()`就是向该队列放东西。

### 输入管理

主程序创建了一个`InputTask`用来管理触摸板和编码器，该task会负责初始化这些外设并在循环中读取输入，当读取到输入后，通过调用`input_event_handle`这个函数指针指向的函数传递结果。通过给该指针赋不同的值来设置不同APP的回调函数。当主程序需要对特定输入进行响应时，可以在回调函数前添加需要的代码。

本程序中，主程序会响应长按事件，用来在长按时返回菜单(app_id=0)。

### 应用切换

该功能主要利用了函数指针实现。`APPEnter`和`APPExit`到最后其实都是调用了`entrance`这个函数指针数组中的函数。

```c
uint8_t (*entrance[MAX_APPID])(void);//app init function
```

该数组在`app_main`中初始化，下标就是各个APP的appid：

```c
/* Register Apps */
entrance[0]=enter_menu;
entrance[1]=enter_print;
entrance[2]=enter_brick;
```

每个app的入口函数除了完成该app的初始化，还需要对`input_event_handle`和`app_run`这两个函数指针赋值，实现回调的注册。其中`app_run`会在`app_main`的循环中调用：

```c
while(1)
{
    app_run();
    ...
}
```

### 避免触发看门狗

ESPIDF中有一个看门狗任务，该任务负责给看门狗喂狗，当该任务一直无法得到调度，就会触发重启。

为避免触发，需要经常使用FreeRTOS的阻塞等待，最简单的方法就是在每个while中添加一个`vTaskDelay()`语句延迟适当的时间让出时间片给其他任务。

## APP的实现

### menu.c菜单的实现

目前菜单程序非常简陋，是没有扩展性，添加新APP可能需要较大改动。

进入菜单后，上方会等分为两个小区域分别显示现有的两个APP，下方用绿色方块指示选择了哪一个app，可以用编码器和触摸来选择app。短按编码器，切换到选中的app中；长按编码器，会进入开发者界面，显示开发者提供的图样。

该APP的特殊性在于，它被设定为APP_id=0；app0是默认进入的app且当编码器长按时，其他app都会切换到app0。

**输入回调**

```c
static void menu_input_event_handle(InputEvent e,void *data)
```

该回调会将传递来的输入信息存储到本地变量，以便之后在run中处理。

**入口函数**

```c
uint8_t enter_menu(void)
```

该函数先对两个函数指针进行赋值，然后调用函数初始化本地变量并清屏。

**menu_run**

该函数依次执行以下任务：

* 绘制界面

当选中的APPID==0(`SelectAppID==0`)时,绘制开发者界面，显示LO字幕和循环流转的灯。

否则的话，将会分别绘制两个APP的示意图标，并根据`SelectAppID`，在相应选中的APP图标下方绘制绿色长方形表示选中该APP。

* 处理输入事件

根据本地变量`menu_event`，当按下时，调用APP切换函数切换到选中APP；当长按时，切换到开发者界面(将SelectAppID=0)；当编码器转动时，增加或者减小`SelectAppID`的值来切换选中的app；当有触摸时，根据触摸的横坐标选择不同的app。

### print.c的实现

该程序是一个画板。进入程序后，上面为画布，下发为颜色选择，有两行显示。第一行表示当前选择了那个颜色分量进行调节，并且会实时显示当画笔颜色。第二行分为三个等分，分别为RGB颜色分量的指示，0-200的范围对应0-6格颜色条。直接触摸颜色分量就可以选中，选中后通过编码器调节颜色分量的大小。

**入口函数**

```c
uint8_t enter_print(void)
```

赋值回调后，初始化本地变量，包括画笔颜色和输入事件，并清屏。

**输入回调**

```c
static void print_input_event_handle(InputEvent e,void *data)
```

该回调会将传递来的输入信息存储到本地变量，以便之后在run中处理。

**print_run**

依次完成以下任务：

* 绘制选色条

根据当前颜色分量和画笔颜色绘制底部的选色条。

* 处理输入

根据本地变量`print_event`，当按下时，清屏一次；当编码器转动，根据选择增减相应颜色分量；当触模时，如果触摸点在上方，则画一个点，如果在底部，则切换选择的分量。

暂不支持多点触摸。

### brickbreak.c的实现

打砖块游戏，逻辑基本来自于例程。

开始后就是正常打砖块。当砖块全部消失，黄色闪屏，表示胜利，短按编码器重新开始；当求落出拍子，游戏失败，白色闪屏，短按编码器重新开始；使用触摸和编码器旋转都可以移动拍子。

该程序结构比较复杂这里只是简单说一下。

**brick_run**

依次完成以下任务：

在`GAME_RUN`状态下：

* 移动小球坐标
* 检查是否撞墙
* 检查是否撞拍子
* 检查是否撞砖块
* 绘制游戏图像
* 检查输入并移动拍子
* 检测是否游戏失败(落拍)
* 检查是否游戏胜利(没有砖块了)

在`GAME_STOP`状态下：

* 检查是否有短按输入，有则重新开始游戏

**要点**

1. APP接入点和初始化时两个函数，重新开始时值调用初始化函数。
2. 使用一个数组维护砖的数量和位置，当sum==0，则胜利。
3. 和拍子接触时，使用一个随机数让球横坐标改变，增加游戏随机性。
4. 和砖块接触，反转y轴速度时，注意判断y是否为0，**防止两次反转**导致球Y轴速度依然向上导致数据溢出触发失败条件。
5. 移动拍子时注意限位，防止移到外面去了。
6. 目前触摸点会设置为拍子的最左边而不是中间，这会导致玩起来很别扭，可以优化。
