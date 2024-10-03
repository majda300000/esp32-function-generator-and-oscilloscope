/**
 * @file ui_app.c
 *
 * @brief System init
 *
 * COPYRIGHT NOTICE: (c) 2024 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "ui_app.h"
#include "fn_gen.h"
#include "gui.h"
#include "ui.h"
#include "oscilloscope.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_sleep.h"

#include "button.h"
#include "joystick.h"
#include "led.h"
#include "temp_sensor_sht31.h"
#include "osc_chart.h"

//---------------------------------- MACROS -----------------------------------
#define JOYSTICK_TASK_PERIOD_MS (250)
#define UI_TEMP_TASK_PERIOD_MS  (3000)

#define JOYSTICK_TASK_STACK_SIZE (1024)
#define UI_TEMP_TASK_STACK_SIZE  (1024 * 2)

#define UI_ADC1_CHAN_A_PIN (33)
#define UI_ADC1_CHANNEL_A  (5)

#define UI_ADC1_CHAN_B_PIN (32)
#define UI_ADC1_CHANNEL_B  (4)

#define UI_TEMP_SHUTOWN_THRESH_C (32)
//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------

/**
 * @brief Periodically reads and sends new yojstick info to gui task
 *
 * @param p_param
 */
static void _joystick_update_task(void *p_param);

/**
 * @brief Periodically reads sensor data and sneds them to gui
 *
 * @param param
 */
static void _temp_read_task(void *param);

/**
 * @brief Sets temperature and humidity labels to values given
 * 
 * @param temp Temperature value
 * @param hum Humidity value
 */
void static _set_temp_hum_text(float temp, float hum);

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static const char *TAG = "ui_app";
//------------------------------- GLOBAL DATA ---------------------------------
oscilloscope_t *p_osc;       // Channel 1
oscilloscope_t *p_osc_other; // Channel 2

//------------------------------ PUBLIC FUNCTIONS -----------------------------
void app_start(void)
{
    // Joystick initialization
    if(JOYSTICK_ERR_NONE != joystick_create(JOYSTICK_1))
        return;

    // LEDs initialization
    if(LED_ERR_NONE != led_create(LED_RED))
        return;
    if(LED_ERR_NONE != led_create(LED_BLUE))
        return;
    if(LED_ERR_NONE != led_create(LED_GREEN))
        return;

    // Device powered on
    led_pattern_run(LED_GREEN, LED_PATTERN_KEEP_ON, NO_TIMEOUT);


    // Initialize function generatotor unit
    fn_gen_init();

    // Create instances of oscilloscopes
    p_osc       = oscilloscope_create(UI_ADC1_CHAN_A_PIN, UI_ADC1_CHANNEL_A);
    p_osc_other = oscilloscope_create(UI_ADC1_CHAN_B_PIN, UI_ADC1_CHANNEL_B);

    // Start reading data from oscilloscopes
    oscilloscope_start(p_osc);
    oscilloscope_start(p_osc_other);

    // Initilize gui
    gui_init();

    // Start LED pattern that signalizes oscilloscope working
    led_pattern_run(LED_RED, LED_PATTERN_KEEP_ON, NO_TIMEOUT);


    // Wait for gui to do initialize the chart
    while(NULL == ui_Chart2)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    if(ESP_OK != osc_chart_init(ui_Chart2, p_osc, p_osc_other)){
        ESP_LOGE(TAG, "Chart not successfully initialized!");
    }

    ESP_LOGI(TAG, "System initialized!");

    BaseType_t task_ret_val;

    /* Create task thats sends joystick data to gui */
    static TaskHandle_t _joystick_task_handle = NULL;

    task_ret_val = xTaskCreatePinnedToCore(
        _joystick_update_task, "Joystick update task", JOYSTICK_TASK_STACK_SIZE, NULL, 0, &_joystick_task_handle, 0);

    if((NULL == _joystick_task_handle) || (task_ret_val != pdPASS))
    {
        ESP_LOGE(TAG, "Joystick task not created");
    }

    /* Create task that periodically measures temperature and sends to gui */
    static TaskHandle_t _temp_task_handle = NULL;

    task_ret_val
        = xTaskCreatePinnedToCore(_temp_read_task, "Temperature read task", UI_TEMP_TASK_STACK_SIZE, NULL, 0, &_temp_task_handle, 0);

    if((NULL == _temp_task_handle) || (task_ret_val != pdPASS))
    {
        ESP_LOGE(TAG, "Temperature read task not created");
    }
}

void ui_turn_off_oscilloscope(void)
{
    oscilloscope_stop(p_osc);
    oscilloscope_stop(p_osc_other);
    ESP_LOGI(TAG, "Turning oscilloscope off");
}

void ui_turn_on_oscilloscope(void)
{
    oscilloscope_start(p_osc);
    oscilloscope_start(p_osc_other);
    ESP_LOGI(TAG, "Turning oscilloscope on");
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static void _joystick_update_task(void *p_param)
{
    (void)p_param;

    joystick_discrete_position_t pos = JOYSTICK_MIDDLE;

    for(;;)
    {
        // Get new joystick position and send it to gui
        pos = joystick_discrete_position_get(JOYSTICK_1);
        gui_receive_joystick_pos(pos);
        vTaskDelay(pdMS_TO_TICKS(JOYSTICK_TASK_PERIOD_MS));
    }
}

static void _temp_read_task(void *param)
{
    float     temperature, humidity;
    esp_err_t err;

    err = temp_sensor_init();
    if(err != ESP_OK)
    {
        ESP_LOGE(TAG, "Temperature sensor initialization failed");
        vTaskDelete(NULL);
    }

    for(;;)
    {
        // Read temperature and humidity from the sensor
        err = temp_sensor_read(&temperature, &humidity);
        if(err == ESP_OK)
        {
            ESP_LOGI(TAG, "Temperature: %.2f°C, Humidity: %.2f%%", temperature, humidity);
            _set_temp_hum_text(temperature, humidity);
        }
        else
        {
            ESP_LOGE(TAG, "Failed to read data from SHT31 sensor");
        }

        if(temperature >= UI_TEMP_SHUTOWN_THRESH_C)
        {
            // Show too hot label
            lv_obj_set_style_bg_color(ui_Panel1, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(ui_Panel4, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(ui_Panel3, lv_color_hex(0xff0000), LV_PART_MAIN | LV_STATE_DEFAULT);

            lv_obj_set_style_opa(ui_deviceTooHotLabel1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_opa(ui_deviceTooHotLabel2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_opa(ui_deviceTooHotLabel, 255, LV_PART_MAIN | LV_STATE_DEFAULT);


            ESP_LOGE(TAG, ".");
            ESP_LOGE(TAG, ".");
            ESP_LOGE(TAG, ".");
            ESP_LOGE(TAG, "Device is getting to hot, shutting down.");
            ESP_LOGE(TAG, ".");
            ESP_LOGE(TAG, ".");
            ESP_LOGE(TAG, ".");

            vTaskDelay(pdMS_TO_TICKS(300));

            // Shut down device if it got too hot
            esp_deep_sleep_start();
        }

        vTaskDelay(pdMS_TO_TICKS(UI_TEMP_TASK_PERIOD_MS));
    }
}

void static _set_temp_hum_text(float temp, float hum)
{
    if(ui_tempText == NULL || NULL == ui_tempText2 || NULL == ui_tempText3)
        return;
    char text1[10];
    sprintf(text1, "%.2f°C", temp);

    char text2[10];
    sprintf(text2, "%.2f%%", hum);

    lv_label_set_text(ui_tempText, text1);
    lv_label_set_text(ui_tempText2, text1);
    lv_label_set_text(ui_tempText3, text1);

    lv_label_set_text(ui_humText, text2);
    lv_label_set_text(ui_humText2, text2);
    lv_label_set_text(ui_humText3, text2);
}

//---------------------------- INTERRUPT HANDLERS -----------------------------
