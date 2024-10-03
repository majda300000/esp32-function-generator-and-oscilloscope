/**
 * @file potentiometer.c
 *
 * @brief THe file implements behaviour of potentiometer.
 *
 * COPYRIGHT NOTICE: (c) 2023 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "potentiometer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


//---------------------------------- MACROS -----------------------------------

#define POTENTIOMETER_ADC_ATTEN     ADC_ATTEN_DB_11

//-------------------------------- DATA TYPES ---------------------------------
struct _potentiometer_t
{
    uint8_t  pin;
    bool     b_is_reversed;
    uint8_t  channel;
    uint16_t max_voltage_mv;
};
//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------

/**
 * @brief Allocate memory for a potentiometer_t structure and return a pointer to it
 *
 * @return potentiometer_t*
 */
static potentiometer_t *_potentiometer_alloc(void);

/**
 * @brief This function frees the memory allocated for the potentiometer_t structure.
 *
 * @param p_pot A pointer to the potentiometer_t structure
 */
static void _potentiometer_free(potentiometer_t *p_pot);

/**
 * @brief Create and initialize ADC for reading raw data from the potentiometer
 *
 * @param p_pot Potentiometer structure
 * @return esp_err_t
 */
static esp_err_t _adc_create(potentiometer_t *p_pot);

/**
 * @brief Configures ADC channels for each potentiometer
 *
 * @param p_pot Potentiometer structure for collecting channel number
 * @return esp_err_t
 */
static esp_err_t _adc_config_channel(potentiometer_t *p_pot);

/**
 * @brief Maps raw adc values from range [0 - POTENTIOMETER_ADC_INT_RANGE] to potentiometer position [0 - POTENTIOMETER_MAX_POSITION]
 *
 * @param raw_result value from the ADC
 * @return int  mapped value
 */
static int _potentiometer_map_values(int raw_result);

//------------------------- STATIC DATA & CONSTANTS ---------------------------

// Handle for ADC1
static adc_oneshot_unit_handle_t _adc1_handle = NULL;
static SemaphoreHandle_t adc_semaphore = NULL;  // Semaphore to protect ADC access

static const char *TAG = "Potentiometer";

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------

potentiometer_t *potentiometer_create(uint8_t pin, uint8_t channel, uint16_t max_voltage_mv, bool b_is_reversed)
{
    potentiometer_t *p_pot = _potentiometer_alloc();

    if(NULL == p_pot)
    {
        ESP_LOGE(TAG, "MALLOC FAILED");
        return NULL;
    }

    p_pot->pin            = pin;
    p_pot->channel        = channel;
    p_pot->b_is_reversed  = b_is_reversed;
    p_pot->max_voltage_mv = max_voltage_mv;

    if(NULL == _adc1_handle && ESP_OK != _adc_create(p_pot))
    {
        ESP_LOGE(TAG, "ADC CREATION FAILED");
        _potentiometer_free(p_pot);
        return NULL;
    }

    if(ESP_OK != _adc_config_channel(p_pot))
    {
        ESP_LOGE(TAG, "ADC CHANNEL CONFIG FAILED");
        return NULL;
    }

    // Create the semaphore if it doesn't already exist
    if (adc_semaphore == NULL)
    {
        adc_semaphore = xSemaphoreCreateBinary();
        if (adc_semaphore == NULL)
        {
            ESP_LOGE(TAG, "Failed to create semaphore");
            return NULL;
        }

        xSemaphoreGive(adc_semaphore);
    }

    return p_pot;
}



void potentiometer_delete(potentiometer_t *p_potentiometer)
{
    if(NULL != p_potentiometer)
    {
        _potentiometer_free(p_potentiometer);
        adc_oneshot_del_unit(_adc1_handle);
    }

    // Optionally delete semaphore if no longer needed
    if(adc_semaphore != NULL)
    {
        vSemaphoreDelete(adc_semaphore);
        adc_semaphore = NULL;
    }
    return;
}

uint32_t potentiometer_position_get(potentiometer_t *p_potentiometer)
{
        BaseType_t *pxHigherPriorityTaskWoken = pdFALSE;

    int raw_result;

    // Take the semaphore from ISR before accessing the ADC
    if (xSemaphoreTakeFromISR(adc_semaphore, pxHigherPriorityTaskWoken) == pdTRUE)
    {
        esp_err_t ret = adc_oneshot_read(_adc1_handle, p_potentiometer->channel, &raw_result);
        if (ret != ESP_OK) 
        {
            ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(ret));
            raw_result = 0; // Handle error case
        }

        // Give the semaphore back from ISR
        xSemaphoreGiveFromISR(adc_semaphore, pxHigherPriorityTaskWoken);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to take semaphore in ISR");
        return 0;
    }

    return _potentiometer_map_values(raw_result);
}

int potentiometer_get_raw(potentiometer_t *p_potentiometer)
{
    BaseType_t *pxHigherPriorityTaskWoken = pdFALSE;
    int raw_result;

    // Take the semaphore from ISR before accessing the ADC
    if (xSemaphoreTakeFromISR(adc_semaphore, pxHigherPriorityTaskWoken) == pdTRUE)
    {
        esp_err_t ret = adc_oneshot_read(_adc1_handle, p_potentiometer->channel, &raw_result);
        if (ret != ESP_OK) 
        {
            ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(ret));
            raw_result = 0; // Handle error case
        }

        // Give the semaphore back from ISR
        xSemaphoreGiveFromISR(adc_semaphore, pxHigherPriorityTaskWoken);
    }
    else
    {
        // ESP_LOGE(TAG, "Failed to take semaphore in ISR");
        return -1;
    }

    return raw_result;
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static int _potentiometer_map_values(int raw_result)
{
    return (raw_result) * (POTENTIOMETER_MAX_POSITION) / (POTENTIOMETER_ADC_INT_RANGE);
}

static potentiometer_t *_potentiometer_alloc(void)
{
    return (potentiometer_t *)malloc(sizeof(potentiometer_t));
}

static void _potentiometer_free(potentiometer_t *p_pot)
{
    free(p_pot);
}

static esp_err_t _adc_create(potentiometer_t *p_pot)
{
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &_adc1_handle));

    return ESP_OK;
}

static esp_err_t _adc_config_channel(potentiometer_t *p_pot)
{
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = POTENTIOMETER_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(_adc1_handle, p_pot->channel, &config));

    return ESP_OK;
}
