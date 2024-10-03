/**
* @file led.c

* @brief    High level LED driver

* @par
*
* COPYRIGHT NOTICE: (c) 2022 Byte Lab Grupa d.o.o.
* All rights reserved.
*/

//--------------------------------- INCLUDES ----------------------------------
#include "led.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h" // Include semaphore (for mutex)

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------
struct _led_t
{
    led_gpio_t   *p_led_gpio;
    led_pattern_t pattern;
    led_state_t   state;
    uint32_t      time_since_state_change_ms;
    uint32_t      time_since_pattern_change_ms;
    uint32_t      timeout_ms;
};
//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------

/**
 * @brief Timer callback function that updated led's states according to their pattern
 *
 * @param timer timer handle structure
 */
static void _led_pattern_control_timer_callback(TimerHandle_t timer);

/**
 * @brief Creates FreeRTOS timer instance
 *
 */
static void _led_timer_create();

/**
 * @brief Toggles the state of the led
 *
 * @param led the LED to be toggled
 */
static void _led_toggle_state(led_t *led);

//------------------------- STATIC DATA & CONSTANTS ---------------------------

static const char *TAG = "LED";

static const uint8_t _led_info[LED_COUNT] = {
    // GPIO LEDS
    26U, /* LED_RED */
    27U, /* LED_GREEN */
    14U, /* LED_BLUE */
};

static led_t _led_instances[LED_COUNT] = {};

// Timer to control led patterns
static TimerHandle_t  _led_pattern_control_timer = NULL;
static const uint32_t _led_timer_period_ms       = 10;

// Mutex to protect shared resources
static SemaphoreHandle_t _led_mutex = NULL;

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------

led_err_t led_create(led_name_t led)
{
    // Validate LED name
    if(LED_COUNT <= led)
    {
        return LED_ERR_INVALID_LED;
    }

    led_gpio_t *p_led = led_gpio_create(_led_info[led]);

    if(NULL == p_led)
    {
        return LED_ERR_CREATE;
    }

    led_t _led          = { .p_led_gpio                   = p_led,
                            .pattern                      = LED_PATTERN_NONE,
                            .state                        = LED_OFF,
                            .time_since_state_change_ms   = 0,
                            .time_since_pattern_change_ms = 0,
                            .timeout_ms                   = 0 };
    _led_instances[led] = _led;

    // Create the mutex upon first LED creation
    if(_led_mutex == NULL)
    {
        _led_mutex = xSemaphoreCreateMutex();
        if(_led_mutex == NULL)
        {
            ESP_LOGE(TAG, "Failed to create mutex");
            return LED_ERR_MUTEX_FAIL;
        }
    }

    // Create timer upon first LED creation
    if(NULL == _led_pattern_control_timer)
    {
        _led_timer_create();
    }
    return LED_ERR_NONE;
}

led_err_t led_pattern_run(led_name_t led, led_pattern_t led_pattern, uint32_t timeout_ms)
{
    // Validate LED name
    if(LED_COUNT <= led)
    {
        return LED_ERR_INVALID_LED;
    }

    // Validate LED pattern
    if(LED_PATTERN_COUNT <= led_pattern)
    {
        return LED_ERR_INVALID_PATTERN;
    }

    // Take mutex before accessing shared resource
    if(xSemaphoreTake(_led_mutex, portMAX_DELAY) == pdTRUE)
    {
        _led_instances[led].pattern                      = led_pattern;
        _led_instances[led].timeout_ms                   = timeout_ms;
        _led_instances[led].time_since_pattern_change_ms = 0;

        // Give back the mutex after accessing the resource
        xSemaphoreGive(_led_mutex);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to take mutex");
        return LED_ERR_MUTEX_FAIL;
    }

    return LED_ERR_NONE;
}

led_err_t led_pattern_reset(led_name_t led)
{
    // Validate LED name
    if(LED_COUNT <= led)
    {
        return LED_ERR_INVALID_LED;
    }

    // Take mutex before accessing shared resource
    if(xSemaphoreTake(_led_mutex, portMAX_DELAY) == pdTRUE)
    {
        _led_instances[led].pattern                      = LED_PATTERN_NONE;
        _led_instances[led].time_since_pattern_change_ms = 0;

        // Give back the mutex
        xSemaphoreGive(_led_mutex);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to take mutex");
        return LED_ERR_MUTEX_FAIL;
    }

    return LED_ERR_NONE;
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static void _led_pattern_control_timer_callback(TimerHandle_t timer)
{

    // Take the mutex before accessing shared data
    if(xSemaphoreTake(_led_mutex, portMAX_DELAY) == pdTRUE)
    {
        // Go through all LEDs and update state according to current pattern
        for(int led_i = 0; led_i < LED_COUNT; led_i++)
        {
            // skip non existing ones
            if(NULL == _led_instances[led_i].p_led_gpio)
                continue;

            // update time since pattern changed
            _led_instances[led_i].time_since_pattern_change_ms += _led_timer_period_ms;

            // check timeouts
            if(_led_instances[led_i].time_since_pattern_change_ms >= _led_instances[led_i].timeout_ms
               && _led_instances[led_i].timeout_ms != 0)
            {
                _led_instances[led_i].pattern                      = LED_PATTERN_NONE;
                _led_instances[led_i].time_since_pattern_change_ms = 0;
            }

            // skip LEDs with pattern NONE and state OFF
            if((LED_PATTERN_NONE == _led_instances[led_i].pattern) && (LED_OFF == _led_instances[led_i].state))
                continue;
            // skip LEDs with pattern KEEP_ON and state ON
            if((LED_PATTERN_KEEP_ON == _led_instances[led_i].pattern) && (LED_ON == _led_instances[led_i].state))
                continue;

            // update LEDs state age
            _led_instances[led_i].time_since_state_change_ms += _led_timer_period_ms;

            // deal with others
            switch(_led_instances[led_i].pattern)
            {
                case LED_PATTERN_NONE:
                    led_gpio_off(_led_instances[led_i].p_led_gpio);
                    _led_instances[led_i].state                      = LED_OFF;
                    _led_instances[led_i].time_since_state_change_ms = 0;

                    break;
                case LED_PATTERN_KEEP_ON:
                    led_gpio_on(_led_instances[led_i].p_led_gpio);
                    _led_instances[led_i].state                      = LED_ON;
                    _led_instances[led_i].time_since_state_change_ms = 0;

                    break;
                case LED_PATTERN_SLOWBLINK:
                    if(LED_SLOWBLINK_HALF_PERIOD_MS <= _led_instances[led_i].time_since_state_change_ms)
                    {
                        _led_toggle_state(&_led_instances[led_i]);
                    }
                    break;
                case LED_PATTERN_FASTBLINK:
                    if(LED_FASTBLINK_HALF_PERIOD_MS <= _led_instances[led_i].time_since_state_change_ms)
                    {
                        _led_toggle_state(&_led_instances[led_i]);
                    }
                    break;

                default:
                    break;
            }
        }

        // Give back the mutex after accessing shared data
        xSemaphoreGive(_led_mutex);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to take mutex in timer callback");
    }
}

static void _led_timer_create()
{
    _led_pattern_control_timer
        = xTimerCreate("LED pattern controller", pdMS_TO_TICKS(_led_timer_period_ms), pdTRUE, NULL, _led_pattern_control_timer_callback);

    if(NULL == _led_pattern_control_timer)
    {
        ESP_LOGE(TAG, "Timer not created");
    }
    else
    {
        if(pdPASS != xTimerStart(_led_pattern_control_timer, 0))
        {
            ESP_LOGE(TAG, "Timer could not be set to active state");
        }
    }

    ESP_LOGI(TAG, "Timer created");
}

static void _led_toggle_state(led_t *p_led)
{
    if(p_led->state == LED_ON)
        led_gpio_off(p_led->p_led_gpio);
    else
        led_gpio_on(p_led->p_led_gpio);
    p_led->state                      = (p_led->state == LED_ON) ? LED_OFF : LED_ON;
    p_led->time_since_state_change_ms = 0;
}

//---------------------------- INTERRUPT HANDLERS -----------------------------
