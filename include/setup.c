

#include "setup.h"
#include "m5stack_camera.h"
#include "sensor.h"

#include "bgSubtraction.h"
#include "sleepHandler.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;

static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
    .xclk_freq_hz = 16500000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_HD,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12, //0-63 lower number means higher quality
    .fb_count = 1,       //if more than one, i2s runs in continuous mode. Use only with JPEG
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
};

wifi_config_t WIFI_CFG_1 = {
    .sta = {
        .ssid = WIFI_SSID_1,
        .password = WIFI_PW_1,
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        .pmf_cfg = {
            .capable = true,
            .required = false
        },
    },
};

wifi_config_t WIFI_CFG_2 = {
    .sta = {
        .ssid = WIFI_SSID_2,
        .password = WIFI_PW_2,
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        .pmf_cfg = {
            .capable = true,
            .required = false
        },
    },
};

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 10) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


void wifi_init_sta()
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &event_handler,
                    NULL,
                    &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    IP_EVENT_STA_GOT_IP,
                    &event_handler,
                    NULL,
                    &instance_got_ip));
    
    ESP_ERROR_CHECK(esp_wifi_start() );
}

esp_err_t wifi_connect(wifi_config_t* cfg) {
    ESP_LOGI(TAG, "Trying to connect to SSID: %s", cfg->sta.ssid);
    s_retry_num = 0;

    esp_err_t ret = ESP_OK;
    s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, cfg) );
	
    esp_err_t connect_resp = esp_wifi_connect();
    ESP_LOGI(TAG, "Connect response: %i", connect_resp);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */
	if (bits & WIFI_CONNECTED_BIT) {
		ESP_LOGI(TAG, "connected to ap SSID:%s",
				 cfg->sta.ssid);
	} else if (bits & WIFI_FAIL_BIT) {
		ESP_LOGI(TAG, "Failed to connect to SSID: %s",
				 cfg->sta.ssid);
        ret = ESP_FAIL;
	} else {
		ESP_LOGE(TAG, "UNEXPECTED EVENT");
        ret = ESP_ERR_INVALID_STATE;
	}

  return ret;
}


esp_err_t init_camera()
{
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }
    return ESP_OK;
}

void blink(int times)  {

    m5_camera_led_set_brightness(0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    bool on = false;
    for (int i = 0; i < times * 2; ++i) {
        if (!on) {
            m5_camera_led_set_brightness(50);
            on = true;
        } 
        else {
            m5_camera_led_set_brightness(0);
            on = false;
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void fade_led(uint8_t brightness, int time_ms) {
    uint32_t duty = (uint32_t)((brightness / 100.0) * 8192.0);
    m5_camera_led_start_with_fade_time(duty, time_ms);
}

void setup() {
	printf("%s", M5STACK_LOGO);
    // initialize LED, Button, BAT, BM8563
    m5_camera_init();
    // Power on
    // m5_camera_battery_hold_power();
    // Check battery voltage
    // m5_camera_battery_hold_power();
    // Check battery voltage
    // ESP_LOGI(TAG, "Battery voltage: %dmV", m5_camera_battery_voltage());
    
    // LED
    m5_camera_led_set_brightness(0);
    blink(1);
    fade_led(50, 2000);

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    wifi_init_sta();

    esp_err_t wifi_connection_status = ESP_OK;
    wifi_connection_status = wifi_connect(&WIFI_CFG_1);

    if (wifi_connection_status != ESP_OK) {
        ESP_LOGI(TAG, "Could not connect to first access point.");
        wifi_connection_status = wifi_connect(&WIFI_CFG_2);
    }

    if (wifi_connection_status != ESP_OK) {
        ESP_LOGE(TAG, "Could not connect to any of the available access points, restarting ESP...");
        // Go to sleep to wait for wifi
        go_to_sleep(60 * 30);
    }
    
    // camera_config.frame_size = (framesize_t)SIZE;
    //camera_config.frame_size = (framesize_t)FRAMESIZE_SVGA;
		//camera_config.frame_size = (framesize_t)FRAMESIZE_UXGA;
		//camera_config.frame_size = (framesize_t)FRAMESIZE_QXGA;
		
    init_camera();

    // flip the camera fream
    sensor_t *s = esp_camera_sensor_get();
    s->set_vflip(s, 1);

		sleep_handler_setup();

    // Success! Blink 3 times and the low brightness
    blink(3);
    m5_camera_led_set_brightness(5);

}
	


