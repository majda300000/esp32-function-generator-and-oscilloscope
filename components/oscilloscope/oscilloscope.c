/**
 * @file oscilloscope.c
 *
 * @brief   Class to capture signals from ADC
 *
 * COPYRIGHT NOTICE: (c) 2024 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "oscilloscope.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdbool.h>

//---------------------------------- MACROS -----------------------------------
#define _QUEUE_LEN (1u)
//-------------------------------- DATA TYPES ---------------------------------

struct _oscilloscope_t
{
    int      pin;
    int      chan;
    uint32_t adc_raw[OSCILLOSCOPE_SAMPLE_NUMBER];
    int      timer_tick;
    bool     is_running;

    QueueHandle_t    queue;
    potentiometer_t *p_pot;
    osc_timer_t     *p_tim;
};

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
void _adc_read_cb(void *arg);
//------------------------- STATIC DATA & CONSTANTS ---------------------------
static const char *TAG = "oscilloscope";
//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------

oscilloscope_t *oscilloscope_create(int pin, int channel_number)
{
    oscilloscope_t *p_osc = (oscilloscope_t *)malloc(sizeof(oscilloscope_t));

    p_osc->chan       = channel_number;
    p_osc->pin        = pin;
    p_osc->timer_tick = 0;
    p_osc->p_pot      = potentiometer_create(pin, channel_number, 3300, false);
    p_osc->p_tim      = osc_timer_create(_adc_read_cb, p_osc, OSCILLOSCOPE_TIMER_PERIOD_US);

    memset(p_osc->adc_raw, 0, sizeof(p_osc->adc_raw));

    p_osc->queue = xQueueCreate(_QUEUE_LEN, sizeof(p_osc->adc_raw));

    // Check if queue  is created successfully.
    if(NULL == p_osc->queue)
    {
        ESP_LOGE(TAG, "Queue not created successfully!");
    }

    return p_osc;
}

void oscilloscope_start(oscilloscope_t *p_osc)
{
    p_osc->is_running = true;
    osc_timer_start(p_osc->p_tim);
    ESP_LOGI(TAG, "Starting oscilloscope");
}

void oscilloscope_stop(oscilloscope_t *p_osc)
{
    p_osc->is_running = false;
    osc_timer_stop(p_osc->p_tim);
    ESP_LOGI(TAG, "Stopping oscilloscope");
}

void oscilloscope_send_new_data(oscilloscope_t *p_osc, int *data)
{
    if(p_osc->queue != NULL && p_osc->is_running)
    {
        xQueueReceive(p_osc->queue, data, portMAX_DELAY);
    }
}

void oscilloscope_print(oscilloscope_t *p_osc)
{
    for(int i = 0; i < OSCILLOSCOPE_SAMPLE_NUMBER; i++)
    {
        printf("%ld ", p_osc->adc_raw[i]);
    }
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

void IRAM_ATTR _adc_read_cb(void *arg)
{
    oscilloscope_t *p_osc                    = (oscilloscope_t *)arg;
    BaseType_t      xHigherPriorityTaskWoken = pdFALSE;

    // Read value
    p_osc->adc_raw[p_osc->timer_tick] = potentiometer_get_raw(p_osc->p_pot) * VDD / POTENTIOMETER_ADC_INT_RANGE;

    // If didnt succeed in reading adc use previous reading
    if(p_osc->adc_raw[p_osc->timer_tick] == -1)
    {
        // Don't get segmentation fault!
        if(p_osc->timer_tick >= 2)
        {
            p_osc->adc_raw[p_osc->timer_tick] = p_osc->adc_raw[p_osc->timer_tick - 1];
        }
        else
        {
            p_osc->adc_raw[p_osc->timer_tick] = 0;
        }
    }

    // Increase frame pointer
    p_osc->timer_tick++;

    // Frame full, send to queue and reset frame pointer
    if(p_osc->timer_tick >= OSCILLOSCOPE_SAMPLE_NUMBER)
    {
        if(p_osc->queue != NULL)
        {
            xQueueOverwriteFromISR(p_osc->queue, p_osc->adc_raw, &xHigherPriorityTaskWoken);
        }
        p_osc->timer_tick = 0;
    }
}
//---------------------------- INTERRUPT HANDLERS -----------------------------
