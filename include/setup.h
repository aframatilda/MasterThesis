
#include "constants.h"
#include "secrets.h"

#include <stdio.h>
#include <esp_log.h>

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include <sys/param.h>
#include "esp_camera.h"
#include "m5stack_camera.h"

#include "FtpClient.h"
#include "preferences.h"
#include "bgSubtraction.h"
#include "sensor.h"
#include "sleepHandler.h"

// #define BOARD_ESP32CAM_AITHINKER 1

#ifdef BOARD_ESP32CAM_AITHINKER

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif


/* Test config for ESP32_M5CAM (Not working yet)
#define BOARD_ESP32_M5CAM 1 
#ifdef BOARD_ESP32_M5CAM 

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 27
#define CAM_PIN_SIOD 25
#define CAM_PIN_SIOC 23

#define CAM_PIN_D7 19
#define CAM_PIN_D6 36
#define CAM_PIN_D5 18
#define CAM_PIN_D4 39
#define CAM_PIN_D3 5
#define CAM_PIN_D2 34
#define CAM_PIN_D1 35
#define CAM_PIN_D0 17	
#define CAM_PIN_VSYNC 22
#define CAM_PIN_HREF 26
#define CAM_PIN_PCLK 21

#endif
*/
static EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "m5stack:wake-up";

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void wifi_init_sta();
esp_err_t wifi_connect(wifi_config_t* cfg);

esp_err_t init_camera();

void blink(int times);

void fade_led(uint8_t brightness, int time_ms);

void setup();
