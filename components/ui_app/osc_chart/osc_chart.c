/**
 * @file osc_chart.c
 *
 * @brief   Class to handle lvgl chart used as oscilloscope
 *
 * COPYRIGHT NOTICE: (c) 2024 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "osc_chart.h"
#include "../ui_app.h"
#include "fn_gen.h"
#include "../gui.h"
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
#include "esp_err.h"

//---------------------------------- MACROS -----------------------------------
#define CHART_TASK_PERIOD_MS  (250)
#define CHART_TASK_STACK_SIZE (1024 * 4)

#define CHART_SER_A_COLOR (0xff99ff)
#define CHART_SER_B_COLOR (0x5bc6ca)

#define CHART_DIV_1_MS (10)
#define CHART_DIV_2_MS (1)

#define CHART_DIV_1_MV (500)
#define CHART_DIV_2_MV (100)

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------

/**
 * @brief Gets latest value from oscilloscope and sets chart values
 *
 * @param p_param
 */
static void _chart_update_task(void *p_param);

//------------------------- STATIC DATA & CONSTANTS ---------------------------

static const char *TAG = "osc_chart";

//------------------------------- GLOBAL DATA ---------------------------------

static osc_chart_t _chart;

//------------------------------ PUBLIC FUNCTIONS -----------------------------
esp_err_t osc_chart_init(lv_obj_t *chart, oscilloscope_t *p_osc1, oscilloscope_t *p_osc2)
{

    _chart.chart       = chart;
    _chart.p_chan_1    = p_osc1;
    _chart.p_chan_2    = p_osc2;
    _chart.p_ser1      = lv_chart_add_series(_chart.chart, lv_color_hex(CHART_SER_A_COLOR), LV_CHART_AXIS_PRIMARY_Y);
    _chart.p_ser2      = lv_chart_add_series(_chart.chart, lv_color_hex(CHART_SER_B_COLOR), LV_CHART_AXIS_PRIMARY_Y);
    _chart.div_ms      = CHART_DIV_1_MS;
    _chart.div_mV      = CHART_DIV_1_MV;
    _chart.data_length = OSCILLOSCOPE_SAMPLE_NUMBER;

    _chart.data_1 = (int *)malloc(_chart.data_length * sizeof(int));
    if(_chart.data_1 == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for data_1");
        return ESP_FAIL;
    }
    _chart.data_2 = (int *)malloc(_chart.data_length * sizeof(int));
    if(_chart.data_2 == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for data_2");
        free(_chart.data_1); // Clean up previously allocated memory
        return ESP_FAIL;
    }

    // Set lvgl chart to our point number
    lv_chart_set_point_count(_chart.chart, _chart.data_length);

    // Initialize series to 0
    for(int i = 0; i < _chart.data_length; i++)
    {
        lv_chart_set_next_value(_chart.chart, _chart.p_ser1, 0);
        lv_chart_set_next_value(_chart.chart, _chart.p_ser2, 0);
    }

    /* Create task that refreshes chart data absed on oscilloscope readings */
    static TaskHandle_t task__hndl = NULL;
    BaseType_t          task_ret_val;

    task_ret_val = xTaskCreatePinnedToCore(_chart_update_task, "Chart update task", CHART_TASK_STACK_SIZE, NULL, 0, &task__hndl, 1);

    if((NULL == task__hndl) || (task_ret_val != pdPASS))
    {
        ESP_LOGE(TAG, "Chart update task not created");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void osc_chart_ch1_show()
{
    lv_chart_hide_series(_chart.chart, _chart.p_ser1, false);
    oscilloscope_start(_chart.p_chan_1);
}

void osc_chart_ch1_hide()
{
    lv_chart_hide_series(_chart.chart, _chart.p_ser1, true);
    oscilloscope_stop(_chart.p_chan_1);
}

void osc_chart_ch2_show()
{
    lv_chart_hide_series(_chart.chart, _chart.p_ser2, false);
    oscilloscope_start(_chart.p_chan_2);
}

void osc_chart_ch2_hide()
{
    lv_chart_hide_series(_chart.chart, _chart.p_ser2, true);
    oscilloscope_stop(_chart.p_chan_2);
}

void ui_set_div_10ms(void)
{

    _chart.div_ms = 10;
            ESP_LOGI(TAG, "Set divY to 10ms");

}

void ui_set_div_1ms(void)
{
    _chart.div_ms = 1;
        ESP_LOGI(TAG, "Set divY to 1ms");

}

void ui_set_div_500mV(void)
{
    _chart.div_mV = 500;
}

void ui_set_div_100mV(void)
{
    _chart.div_mV = 100;
    ESP_LOGI(TAG, "Set divY to 100mV");
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static void _chart_update_task(void *p_param)
{
    (void)p_param;
    ESP_LOGI(TAG, "Update chart task created!");

    // Wait for other initializations
    vTaskDelay(pdMS_TO_TICKS(5000));

    for(;;)
    {
        // Receive new data from oscilloscope
        oscilloscope_send_new_data(_chart.p_chan_1, _chart.data_1);
        oscilloscope_send_new_data(_chart.p_chan_2, _chart.data_2);

        // Shorted data buff is divX is smaller
        if(CHART_DIV_2_MS == _chart.div_ms)
        {
            lv_chart_set_point_count(_chart.chart, _chart.data_length / (CHART_DIV_1_MS / CHART_DIV_2_MS));
        }
        else if(CHART_DIV_1_MS == _chart.div_ms)
        {
            lv_chart_set_point_count(_chart.chart, _chart.data_length);
        }

        for(int i = 0; i < _chart.data_length ; i++)
        {
            // Add the new value to the chart series
            int data_to_add_1 = _chart.data_1[i];
            int data_to_add_2 = _chart.data_2[i];

            // Scale accordingly if divY is smaller
            if(CHART_DIV_2_MV == _chart.div_mV)
            {
                data_to_add_1 -= (VDD / 2);
                data_to_add_1 *= (CHART_DIV_1_MV / CHART_DIV_2_MV);
                data_to_add_1 += (VDD / 2);

                data_to_add_2 -= (VDD / 2);
                data_to_add_2 *= (CHART_DIV_1_MV / CHART_DIV_2_MV);
                data_to_add_2 += (VDD / 2);
            }

            // Skip if divX is
            if(CHART_DIV_2_MS == _chart.div_ms)
            {
                if(i > _chart.data_length / (CHART_DIV_1_MS / CHART_DIV_2_MS)) continue;
            }
            lv_chart_set_next_value(_chart.chart, _chart.p_ser1, data_to_add_1);
            lv_chart_set_next_value(_chart.chart, _chart.p_ser2, data_to_add_2);
        }

        // Refresh the chart to show the updated data
        lv_chart_refresh(_chart.chart);

        // Delay to control the update rate (convert milliseconds to ticks)
        vTaskDelay(pdMS_TO_TICKS(CHART_TASK_PERIOD_MS));
    }
}
//---------------------------- INTERRUPT HANDLERS -----------------------------
