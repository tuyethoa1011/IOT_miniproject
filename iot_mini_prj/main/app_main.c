/**
  ***************************************************************************************************************
  ***************************************************************************************************************
  ***************************************************************************************************************
  File:	      app_main.c
  Modifier:   Ngo Le Tuyet Hoa
  Updated:    4th JUNE 2023
  ***************************************************************************************************************
  Copyright (C) 2023 https://github.com/tuyethoa1011
  This is a free software under the GNU license, you can redistribute it and/or modify it under the terms
  of the GNU General Public License version 3 as published by the Free Software Foundation.
  This software library is shared with public for educational purposes, without WARRANTY and Author is not liable for any damages caused directly
  or indirectly by this software, read more about this on the GNU General Public License.
  ***************************************************************************************************************
*/

//----- LIBRARY INCLUDE -----
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
//------I2C LCD lib -----
#include "esp_log.h"
#include "driver/i2c.h"
#include "ssd1306.h"
//------Sensor lib (DHT22, cảm biến lượng mưa) -----
#include "dht.h"
//----- WiFi (MQTT) lib -----
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "mqtt_client.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"


static const char *TAG = "IOT_MINI_PROJECT";
#define EXAMPLE_TAG "IOT_MINI_PROJECT"

//----- LIBRARY INCLUDE -----
//-----  VARIABLES DECALATION -----
uint8_t gpio_dht = GPIO_NUM_25;
char  h_buf[50], t_buf[50], rain_lv_buf[50];
static esp_mqtt_client_handle_t client ;
static int msg_id;
//công thức tính lượng mưa: dựa vào mực nước thu được từ cảm biến mưa
//-----  VARIABLES DECALATION -----
//----- DEFINITIONS -----
//----- I2C LCD define -----
#define CONFIG_SSD1306_ADDR 0x3C
#define CONFIG_SSD1306_OPMODE 0x3C

#define I2C_MASTER_SCL_IO 4            /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 5              /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM 1     /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000        /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */

#define DELAY_TIME_BETWEEN_ITEMS_MS 1000 /*!< delay time between different test items */

#define SSD1306_OLED_ADDR   CONFIG_SSD1306_ADDR  /*!< slave address for OLED SSD1306 */
#define SSD1306_CMD_START CONFIG_SSD1306_OPMODE   /*!< Operation mode */
#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
//----- I2C LCD define -----
//----- Cảm biến môi trường define -----
#if defined(CONFIG_EXAMPLE_TYPE_DHT11)
#define SENSOR_TYPE DHT_TYPE_DHT11
#endif
#if defined(CONFIG_EXAMPLE_TYPE_AM2301)
#define SENSOR_TYPE DHT_TYPE_AM2301
#endif
#if defined(CONFIG_EXAMPLE_TYPE_SI7021)
#define SENSOR_TYPE DHT_TYPE_SI7021
#endif

//----- Cảm biến môi trường define -----

//----- DEFINITIONS -----

//----- FUNCTIONS -----
//----- I2C LCD essential function -----
/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}
//----- I2C LCD essential function -----
uint8_t count = 10;
//----- WIFI FUNCTIONS -----
static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    client = event->client;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .host = "mqtt.flespi.io",
        .port = 1883,
        .username = "YERVPkXy4fbkQuav4MjCqa6FwE9bp0UesAX7KKtDPsNtKLPFnW7BU16et6gJUYLc",
        .password = "",
        .client_id = "hoa_esp32",
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void mqtt_init(void)
{   

    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

}

//----- WIFI FUNCTIONS -----


//----- FUNCTIONS -----
//----- TASKS -----

SemaphoreHandle_t sem1; //khởi tạo biến semaphore

void dht_task(void *pvParameters) //Task dùng để lấy giá trị từ cảm biến môi trường (độ ẩm, nhiệt độ, lượng mưa,...)
{
    static float temperature, humidity;
#ifdef CONFIG_EXAMPLE_INTERNAL_PULLUP
    gpio_set_pull_mode(gpio_dht, GPIO_PULLUP_ONLY);
#endif
    
    while (1)
    {   
        if (dht_read_float_data(SENSOR_TYPE, CONFIG_EXAMPLE_DATA_GPIO, &humidity, &temperature) == ESP_OK) {
            //printf("Humidity: %.1f%% Temp: %.1fC\n", humidity, temperature);

            snprintf(h_buf, 50, "Humidity\n%.1f%%", humidity);
            snprintf(t_buf, 50, "\n\nTemperature\n%.1fC", temperature);
        }
        else
            printf("Could not read data from sensor\n");

        // If you read the sensor data too often, it will heat up
        // http://www.kandrsmith.org/RJS/Misc/Hygrometers/dht_sht_how_fast.html
        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

void displayoled_sendsv_task(void *pvParameters) //Task hiển thị giá trị đọc được từ cảm biến ra màn hình OLED và gửi lên server data với chu kỳ 10s
{
    ssd1306_init(I2C_MASTER_NUM);
 
    task_ssd1306_display_clear(I2C_MASTER_NUM);
    while(1) //forever loop để chạy task
    {   
        xSemaphoreTake(sem1, portMAX_DELAY);
        task_ssd1306_display_text(h_buf,I2C_MASTER_NUM);
        //in value do am
        task_ssd1306_display_text(t_buf,I2C_MASTER_NUM);
        //in value nhiet do
        //task_ssd1306_display_text(rain_lv_buf,I2C_MASTER_NUM);
        //in value luong mua
        vTaskDelay(100/portTICK_PERIOD_MS);  
        //xử lý gửi dữ liệu cho server theo chu kỳ 10s 1 lần
        //messy print terminal from the start but then get in stable state after first run
        xSemaphoreGive(sem1);
        
        xSemaphoreTake(sem1, portMAX_DELAY);
        count = 10;
        while (count != 0)
        {   
            printf("Sent data in %d\n",count);
            --count;
                
            if(count == 0) {
                msg_id = esp_mqtt_client_publish(client, "sensor/humidity",(const char*)h_buf, 0, 1, 0);
                ESP_LOGI(TAG, "sent publish humidity successful, msg_id=%d", msg_id);

                msg_id = esp_mqtt_client_publish(client, "sensor/temperature",(const char*)t_buf, 0, 1, 0);
                ESP_LOGI(TAG, "sent publish temperature successful, msg_id=%d", msg_id);

                //msg_id = esp_mqtt_client_publish(client, "sensor/rain_ammount", "data_3", 0, 1, 0);
                //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
                //vTaskDelay(10000/portTICK_PERIOD_MS);
            }
            vTaskDelay(1000/portTICK_PERIOD_MS);
        }
        xSemaphoreGive(sem1);
    }
    vTaskDelete(NULL);
}


//----- TASKS -----
void app_main(void)
{   
    sem1 = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK( i2c_master_init());

    xTaskCreate(dht_task, "dht_task_0", configMINIMAL_STACK_SIZE * 3, NULL, 1, NULL);
    xTaskCreate(displayoled_sendsv_task,"displayoled_sendsv_task_1",1024*2,(void *)0,1, NULL);  

    mqtt_init();
    ESP_ERROR_CHECK(example_connect());
    mqtt_app_start();
}



