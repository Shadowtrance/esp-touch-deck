#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include <bb_captouch.h>
#include "BLECont.h"

// Graphics / Display settings
#define GFX_BL 1
Arduino_DataBus *bus = new Arduino_ESP32QSPI(45 /* CS */, 47 /* SCK */, 21 /* D0 */, 48 /* D1 */, 40 /* D2 */, 39 /* D3 */);
Arduino_GFX *g = new Arduino_AXS15231B(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */, false /* IPS */, 320 /* width */, 480 /* height */);
#define CANVAS
Arduino_Canvas *gfx = new Arduino_Canvas(320 /* width */, 480 /* height */, g, 0 /* output_x */, 0 /* output_y */, 0 /* rotation */);

// More Display Stuff
uint32_t bufSize;
lv_disp_draw_buf_t draw_buf;
lv_color_t *disp_draw_buf;
lv_disp_drv_t disp_drv;

// Touch Settings
BBCapTouch bbct;
#define TOUCH_SDA 4
#define TOUCH_SCL 8
#define TOUCH_INT 3
#define TOUCH_RST -1

// bluetooth variables
BLECont *bleCont;
QueueHandle_t msgQueue;
bool isConnected;

// lvgl items
LV_FONT_DECLARE(symbols)
static lv_obj_t *btnm;

// Button Matrix
//==================
#define APP_SYMBOL_MOVEWINDOW  "\xEF\x82\xB2" //f0b2
#define APP_SYMBOL_HEADPHONES  "\xEF\x80\xA5" //f025
#define APP_SYMBOL_SPEAKERS    "\xEF\x8B\x8E" //f2ce
#define APP_SYMBOL_NIGHT       "\xEF\x80\xA7" //f027
#define APP_SYMBOL_PLAYPAUSE   "\xEF\x81\x8B" //f04b
#define APP_SYMBOL_PREVIOUS    "\xEF\x81\x88" //f048
#define APP_SYMBOL_NEXT        "\xEF\x81\x91" //f051
#define APP_SYMBOL_MUTE        "\xEF\x9A\xA9" //f6a9
#define APP_SYMBOL_MIC         "\xEF\x84\xB1" //f131
#define APP_SYMBOL_POO         "\xEF\x8B\xBE" //f2fe
#define APP_SYMBOL_SPOTIFY     "\xEF\x86\xBC" //f1bc
#define APP_SYMBOL_SMILEY      "\xEF\x84\x98" //f118

static const char * btnm_map[] = {
  APP_SYMBOL_MOVEWINDOW,
  APP_SYMBOL_HEADPHONES,
  APP_SYMBOL_SPEAKERS,
  APP_SYMBOL_NIGHT, "\n",
  APP_SYMBOL_PLAYPAUSE,
  APP_SYMBOL_PREVIOUS,
  APP_SYMBOL_NEXT,
  APP_SYMBOL_MUTE, "\n",
  APP_SYMBOL_MIC,
  APP_SYMBOL_POO,
  APP_SYMBOL_SPOTIFY,
  APP_SYMBOL_SMILEY, ""
};
// Button Matrix
//==================

// These have to be here because...reasons.
void updateConnection(bool isOn);
void beginBleController();
void msgTask(void *pvParam);
void beginMsgTask();
static void makeGrid();
static void key_event_cb(lv_event_t * e);
void screen_startScreenTimer();

// Sleep / Wake Stuff... not working yet.
//==================
lv_timer_t *screen_onScreenOffTimer;
bool screen_touchFromPowerOff = false;
#define LCD_MIN_SLEEP_TIME 5
int screenOFFValue = 1; //1 Minute

void screen_wakeUp()
{
    lv_timer_reset(screen_onScreenOffTimer);
    screen_touchFromPowerOff = false;
    //loadScreen(0);
    digitalWrite(GFX_BL, HIGH);
}

void screen_onScreenOff(lv_timer_t *timer)
{
  if(lv_disp_get_inactive_time(NULL) > screenOFFValue)
  {
    //return;
    screen_startScreenTimer();
  }

  //if (screenOFFValue < LCD_MIN_SLEEP_TIME)
  //{
      //return;
  //}

  Serial.println("[SCREEN] Screen Off");
  digitalWrite(GFX_BL, LOW);
  screen_touchFromPowerOff = true;
}

void screen_setupScreenTimer()
{
    screen_onScreenOffTimer = lv_timer_create(screen_onScreenOff, screenOFFValue * 1000 * 60, NULL);
    lv_timer_pause(screen_onScreenOffTimer);
}

void screen_startScreenTimer()
{
    lv_timer_resume(screen_onScreenOffTimer);
}

//==================
// Sleep / Wake Stuff... not working yet.

void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);

  lv_disp_flush_ready(disp_drv);
}

void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
  TOUCHINFO ti;
  bool touched = bbct.getSamples(&ti);

  // if (touched && ti.count > 0)
  // {
  //     data->point.x = ti.x[0];
  //     data->point.y = ti.y[0];
  //     data->state = LV_INDEV_STATE_PRESSED;
  // }
  // else
  // {
  //   data->state = LV_INDEV_STATE_RELEASED;
  // }

  if (touched && ti.count > 0)
  {
    //Sleep / Wake Stuff... not working yet.
    lv_timer_reset(screen_onScreenOffTimer);
    if (screen_touchFromPowerOff)
    {
      screen_wakeUp();
      while (touched)
        ;
      return;
    }

    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = ti.x[0];
    data->point.y = ti.y[0];
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void setup()
{
  Serial.begin(115200);

  gfx->begin();
  gfx->setRotation(1);
  gfx->fillScreen(BLACK);

  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);

  bbct.init(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);
  bbct.setOrientation(90, gfx->height(), gfx->width());

  lv_init();

  bufSize = gfx->width() * 40;
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf)
  {
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_8BIT);
  }
  else
  {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, bufSize);

    // Initialize the display
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = gfx->width();
    disp_drv.ver_res = gfx->height();
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Initialize the (dummy) input device driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
  }

  // variable for bluetooth connection
  isConnected = false;

  // run lvgl to make display
  makeGrid();

  // start bluetooth
  beginMsgTask();
  beginBleController();

  //Sleep / Wake Stuff... not working yet.
  lv_timer_handler();
  screen_setupScreenTimer();
}

void loop()
{
  gfx->flush();
  bleCont->loop();
  lv_timer_handler();
  delay(5);
}

void updateConnection(bool isOn)
{
  if (isConnected != isOn)
  {
    isConnected = isOn;
  }
}

void beginBleController()
{
  bleCont = new BLECont();
  bleCont->bindCallback(updateConnection);
  bleCont->begin();
}

void msgTask(void *pvParam)
{
  int itemNum;
  while (1)
  {
    if (xQueueReceive(msgQueue, &itemNum, portMAX_DELAY) == pdTRUE)
    {
      bleCont->triggerTask(itemNum);
    }
    vTaskDelay(10);
  }
}

void beginMsgTask()
{
  msgQueue = xQueueCreate(10, sizeof(int));
  xTaskCreate(msgTask, "msgTask", 1024, NULL, 1, NULL);
}

static void makeGrid()
{
  btnm = lv_btnmatrix_create(lv_scr_act());
  lv_btnmatrix_set_map(btnm, btnm_map);
  lv_obj_set_style_text_font(btnm, &symbols, LV_PART_ITEMS);
  lv_obj_set_style_border_color(btnm, lv_color_hex(0xff129cbb), LV_PART_ITEMS);
  lv_obj_set_style_border_width(btnm, 2, LV_PART_ITEMS);
  lv_obj_set_size(btnm, 480, 320);
  lv_obj_align(btnm, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_event_cb(btnm, key_event_cb, LV_EVENT_ALL, NULL);
}

static void key_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target(e);
  if (code == LV_EVENT_VALUE_CHANGED)
  {
    uint32_t id = lv_btnmatrix_get_selected_btn(obj);
    const char *txt = lv_btnmatrix_get_btn_text(obj, id);
    LV_UNUSED(txt);
    LV_LOG_USER("%s was pressed\n", txt);
    Serial.print("button pressed: ");
    Serial.println(id);
    xQueueSend(msgQueue, &id, portMAX_DELAY);
  }
}
