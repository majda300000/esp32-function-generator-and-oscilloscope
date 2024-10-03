/**
 * @file fn_gen.c
 *
 * @brief   Class to output various signals using DAC module
 *
 * COPYRIGHT NOTICE: (c) 2024 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "fn_gen.h"
#include "esp_log.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/dac.h"
#include "driver/gptimer.h"

#include "dac.h"
#include "timer.h"
//---------------------------------- MACROS -----------------------------------
#define FN_GEN_DEFAULT_SIGNAL FN_SIGNAL_SINE
#define FN_GEN_DEFAULT_FREQ   (1000)
#define FN_GEN_DEFAULT_AMPL   (1000)
#define FN_GEN_DEFAULT_DUTY   (30)   // *10%

#define OUTPUT_POINT_NUM(freq)   (int)(1000000 / (FN_GEN_TIMER_INTR_US * (freq)) + 0.5) // The number of output wave points.
#define APLITUDE_VOLTS_TO_DAC(v) (int)(255 * (v) / VDD)                                 // Turns amplitude in volts to dac input
#define CONST_PERIOD_2_PI        6.2832

#define _THREAD_STACK_SIZE (2048u)
#define _THREAD_PRIORITY   (tskIDLE_PRIORITY + 2u)

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
/**
 * @brief Timer callback function, used for precise time management in signal generation
 *
 * @param timer timer handle
 * @param edata
 * @param user_data
 */
static void _on_timer_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data);

/**
 * @brief Outputs values to DAC in timer intervals
 *
 * @param pvParameters
 */
void _singal_generator_task(void *pvParameters);

/**
 * @brief Fills raw value array with a period of the signal
 *
 * @param pnt_num Number of array fields used based on freq
 * @param signal Signal type
 * @param amplitude Signal amplitude
 * @param duty_cycle Duty cycle for square wave
 */
static void _prepare_data(int pnt_num, fn_signal_type_t signal, int amplitude, int duty_cycle);

//------------------------- STATIC DATA & CONSTANTS ---------------------------

static fn_generator_t     _fn;
static fn_signal_config_t _config_default = { .signal                = FN_GEN_DEFAULT_SIGNAL,
                                              .frequency_Hz          = FN_GEN_DEFAULT_FREQ,
                                              .amplitude_mV          = FN_GEN_DEFAULT_AMPL,
                                              .duty_cycle_percentage = FN_GEN_DEFAULT_DUTY };


static SemaphoreHandle_t _config_protect_mutex = NULL;

static const char *TAG = "function_generator";

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------

void fn_gen_init()
{
    _fn._config       = _config_default;
    _fn._period_index = 0;
    memset(_fn._raw_val, 0, FN_GEN_POINT_ARR_LEN * sizeof(int));

    // Initialize all presets to default config
    for(int i = 0; i < FN_GEN_PRESET_NUMBER; i++)
    {
        _fn.presets[i] = _config_default;
    }

    // Create mutexes
    if(NULL == _config_protect_mutex)
    {
        _config_protect_mutex = xSemaphoreCreateBinary();
        if(NULL == _config_protect_mutex)
            ESP_LOGE(TAG, "Failed to create config protect mutex");
        else
        {
            xSemaphoreGive(_config_protect_mutex);
        }
    }

    timer_init(FN_GEN_TIMER_INTR_US, _on_timer_alarm_cb);
    dac_init();

    ESP_LOGI(TAG, "Initailized function generator");
}

fn_gen_error_t fn_gen_set_signal_config(fn_signal_config_t config)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Checks parameters
    if(config.signal >= FN_SIGNAL_COUNT)
    {
        ESP_LOGE(TAG, "Unknown signal type!");
        return FN_GEN_ERR_UNKNOWN_SIGNAL;
    }
    if(OUTPUT_POINT_NUM(config.frequency_Hz) >= FN_GEN_POINT_ARR_LEN)
    {
        ESP_LOGE(TAG, "The frequency is too low and using too long buffer.");
        return FN_GEN_ERR_CREATE;
    }
    if(config.amplitude_mV > VDD)
    {
        ESP_LOGE(TAG, "Amplitude is higher than VDD");
        return FN_GEN_ERR_CREATE;
    }
    if((config.duty_cycle_percentage > 100) || (config.duty_cycle_percentage < 0))
    {
        ESP_LOGE(TAG, "Duty cycle is not a whole percentage between 0 and 100");
        return FN_GEN_ERR_CREATE;
    }

    xSemaphoreTakeFromISR(_config_protect_mutex, portMAX_DELAY);

    ESP_LOGI(TAG, "CONFIG: took semaphore");

    _fn._config       = config;
    _fn._period_index = 0;
    _prepare_data(OUTPUT_POINT_NUM(_fn._config.frequency_Hz),
                  _fn._config.signal,
                  APLITUDE_VOLTS_TO_DAC(_fn._config.amplitude_mV),
                  _fn._config.duty_cycle_percentage);

    xSemaphoreGiveFromISR(_config_protect_mutex, &xHigherPriorityTaskWoken);
    ESP_LOGI(TAG, "CONFIG: released semaphore");

    ESP_LOGI(TAG, "Set signal config");

    return FN_GEN_ERR_NONE;
}

fn_gen_error_t fn_gen_set_signal_type(fn_signal_type_t type)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Checks parameters
    if(type >= FN_SIGNAL_COUNT)
    {
        ESP_LOGE(TAG, "Unknown signal type!");
        return FN_GEN_ERR_UNKNOWN_SIGNAL;
    }

    // Protect from signal generating task
    xSemaphoreTakeFromISR(_config_protect_mutex, portMAX_DELAY);

    _fn._config.signal = type;
    _fn._period_index  = 0;
    _prepare_data(OUTPUT_POINT_NUM(_fn._config.frequency_Hz),
                  _fn._config.signal,
                  APLITUDE_VOLTS_TO_DAC(_fn._config.amplitude_mV),
                  _fn._config.duty_cycle_percentage);
    xSemaphoreGiveFromISR(_config_protect_mutex, &xHigherPriorityTaskWoken);
    return FN_GEN_ERR_NONE;
}

fn_gen_error_t fn_gen_set_frequency(int frequency_Hz)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Checks parameters
    if(OUTPUT_POINT_NUM(frequency_Hz) >= FN_GEN_POINT_ARR_LEN)
    {
        ESP_LOGE(TAG, "The frequency is too low and using too long buffer.");
        return FN_GEN_ERR_CREATE;
    }

    // Protect from signal generating task
    xSemaphoreTakeFromISR(_config_protect_mutex, portMAX_DELAY);

    _fn._config.frequency_Hz = frequency_Hz;
    _fn._period_index        = 0;
    _prepare_data(OUTPUT_POINT_NUM(_fn._config.frequency_Hz),
                  _fn._config.signal,
                  APLITUDE_VOLTS_TO_DAC(_fn._config.amplitude_mV),
                  _fn._config.duty_cycle_percentage);
    xSemaphoreGiveFromISR(_config_protect_mutex, &xHigherPriorityTaskWoken);
    return FN_GEN_ERR_NONE;
}

fn_gen_error_t fn_gen_set_amplitude(int amplitude_mV_pp)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Checks parameters
    if(amplitude_mV_pp > VDD)
    {
        ESP_LOGE(TAG, "Amplitude is higher than VDD");
        return FN_GEN_ERR_CREATE;
    }

    // Protect from signal generating task
    xSemaphoreTakeFromISR(_config_protect_mutex, portMAX_DELAY);

    _fn._config.amplitude_mV = amplitude_mV_pp;
    _fn._period_index        = 0;

    _prepare_data(OUTPUT_POINT_NUM(_fn._config.frequency_Hz),
                  _fn._config.signal,
                  APLITUDE_VOLTS_TO_DAC(_fn._config.amplitude_mV),
                  _fn._config.duty_cycle_percentage);
    xSemaphoreGiveFromISR(_config_protect_mutex, &xHigherPriorityTaskWoken);
    return FN_GEN_ERR_NONE;
}

fn_gen_error_t fn_gen_set_duty_cycle(int duty_cycle_percentage)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Checks parameters
    if((duty_cycle_percentage > 100) || (duty_cycle_percentage < 0))
    {
        ESP_LOGE(TAG, "Duty cycle is not a whole percentage between 0 and 100");
        return FN_GEN_ERR_CREATE;
    }
    // Protect from signal generating task
    xSemaphoreTakeFromISR(_config_protect_mutex, portMAX_DELAY);

    _fn._config.duty_cycle_percentage = duty_cycle_percentage;
    _fn._period_index                 = 0;
    _prepare_data(OUTPUT_POINT_NUM(_fn._config.frequency_Hz),
                  _fn._config.signal,
                  APLITUDE_VOLTS_TO_DAC(_fn._config.amplitude_mV),
                  _fn._config.duty_cycle_percentage);
    xSemaphoreGiveFromISR(_config_protect_mutex, &xHigherPriorityTaskWoken);
    return FN_GEN_ERR_NONE;
}

fn_gen_error_t fn_gen_set_preset(fn_signal_config_t config, int preset_num)
{

    if(preset_num >= FN_GEN_PRESET_NUMBER)
    {
        ESP_LOGE(TAG, "Preset with number %d doesn't exist!", preset_num);
        return FN_GEN_ERR_CREATE;
    }

    _fn.presets[preset_num] = config;

    return FN_GEN_ERR_NONE;
}

fn_gen_error_t fn_gen_get_preset(fn_signal_config_t *conf, int preset_num)
{
    if(preset_num >= FN_GEN_PRESET_NUMBER)
    {
        ESP_LOGE(TAG, "Preset with number %d doesn't exist!", preset_num);
        return FN_GEN_ERR_CREATE;
    }

    *conf = _fn.presets[preset_num];

    return FN_GEN_ERR_NONE;
}

fn_gen_error_t fn_gen_load_preset(int preset_num)
{
    return fn_gen_set_signal_config(_fn.presets[preset_num]);
}

fn_gen_error_t fn_gen_signal_start_task()
{
    timer_start();
    ESP_LOGI(TAG, "Starting signal timer");
    return FN_GEN_ERR_NONE;
}

fn_gen_error_t fn_gen_signal_stop_task()
{
    timer_stop();
    ESP_LOGI(TAG, "Stopping signal timer");
    return FN_GEN_ERR_NONE;
}
//---------------------------- PRIVATE FUNCTIONS ------------------------------

static void _prepare_data(int pnt_num, fn_signal_type_t signal, int amplitude, int duty_cycle)
{
    for(int i = 0; i < pnt_num; i++)
    {
        switch(signal)
        {
            case FN_SIGNAL_SINE:
                _fn._raw_val[i] = (int)((sin(i * CONST_PERIOD_2_PI / pnt_num) + 1) * (double)(amplitude) / 2 + 0.5);
                break;
            case FN_SIGNAL_TRIANGLE:
                _fn._raw_val[i] = (i > (pnt_num / 2)) ? (2 * amplitude * (pnt_num - i) / pnt_num) : (2 * amplitude * i / pnt_num);
                break;
            case FN_SIGNAL_SAWTOOTH:
                _fn._raw_val[i] = (i == pnt_num) ? 0 : (i * amplitude / pnt_num);
                break;
            case FN_SIGNAL_SQUARE:
                _fn._raw_val[i] = (i < (pnt_num * duty_cycle / 100)) ? amplitude : 0;
                break;
            default:
                break;
        }
    }
}
//---------------------------- INTERRUPT HANDLERS -----------------------------

/* Timer interrupt service routine */
static void IRAM_ATTR _on_timer_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Used to protect data when changing config
    if(NULL != _config_protect_mutex)
    {

        xSemaphoreTakeFromISR(_config_protect_mutex, portMAX_DELAY);

        // Reached end of period
        if(_fn._period_index >= OUTPUT_POINT_NUM(_fn._config.frequency_Hz))
        {
            _fn._period_index = 0;
        }

        dac_output(_fn._raw_val[_fn._period_index]);
        _fn._period_index++;

        xSemaphoreGiveFromISR(_config_protect_mutex, &xHigherPriorityTaskWoken);
    }
}
