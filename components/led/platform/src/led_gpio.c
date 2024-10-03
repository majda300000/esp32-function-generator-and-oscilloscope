/**
* @file led_gpio.c

* @brief    Platform implementation od LED driver

* @par
*
* COPYRIGHT NOTICE: (c) 2022 Byte Lab Grupa d.o.o.
* All rights reserved.
*/

//--------------------------------- INCLUDES ----------------------------------
#include <stdio.h>
#include "led_gpio.h"
#include "driver/gpio.h"
#include "esp_log.h"

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

struct _led_gpio_t
{
    uint8_t pin;
};

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
/**
 * @brief It configures the GPIO pin as output
 *
 * @param [in] p_led a pointer to the led_gpio_t structure.
 * @return Status of creation
 */
static esp_err_t _led_create(led_gpio_t *p_led);

/**
 * @brief Allocate memory for a led_gpio_t structure and return a pointer to it
 *
 * @return led_gpio_t*
 */
static led_gpio_t *_led_alloc(void);

/**
 * @brief This function frees the memory allocated for the led_gpio_t structure.
 *
 * @param p_led A pointer to the led_gpio_t structure that was created by the _led_create()
 * function.
 */
static void _led_free(led_gpio_t *p_led);

/**
 * @brief Sets the GPIO pin's state to parameter state
 *
 * @param pin GPIO pin number of the led
 * @param state State to which it will set it as
 */
static void _led_set_state(uint8_t pin, led_state_t state);

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static const char *TAG = "LED_GPIO";
//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------

led_gpio_t *led_gpio_create(uint8_t pin)
{
    led_gpio_t *p_led = _led_alloc();

    if(NULL == p_led)
    {
        return NULL;
    }

    p_led->pin = pin;

    if(ESP_OK != _led_create(p_led))
    {
        // Delete led.
        _led_free(p_led);
        return NULL;
    }

    gpio_set_level(pin, LED_OFF);

    return p_led;
}

void led_gpio_delete(led_gpio_t *p_led)
{
    if(NULL != p_led)
    {
        _led_free(p_led);
    }
    return;
}

void led_gpio_on(led_gpio_t *p_led)
{
    if(NULL != p_led)
    {
        _led_set_state(p_led->pin, LED_ON);
    }

    return;
}

void led_gpio_off(led_gpio_t *p_led)
{
    if(NULL != p_led)
    {
        _led_set_state(p_led->pin, LED_OFF);
    }

    return;
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static esp_err_t _led_create(led_gpio_t *p_led)
{
    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask     = (1ULL << p_led->pin),
        .mode             = GPIO_MODE_OUTPUT,
        .pull_up_en       = GPIO_PULLUP_DISABLE,
        .pull_down_en     = GPIO_PULLDOWN_DISABLE,
        io_conf.intr_type = GPIO_INTR_DISABLE,
    };

    return gpio_config(&io_conf);
}

static led_gpio_t *_led_alloc(void)
{
    return (led_gpio_t *)malloc(sizeof(led_gpio_t));
}

static void _led_free(led_gpio_t *p_led)
{
    free(p_led);
}

static void _led_set_state(uint8_t pin, led_state_t state)
{
    gpio_set_level(pin, state);
}
//---------------------------- INTERRUPT HANDLERS -----------------------------
