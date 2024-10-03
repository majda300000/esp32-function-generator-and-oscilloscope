/**
 * @file timer.c
 *
 * @brief Esp timer wrapper class
 *
 * COPYRIGHT NOTICE: (c) 2024 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "timer.h"
#include "esp_timer.h"
#include <unistd.h>
#include "esp_log.h"
#include <stdlib.h>

//---------------------------------- MACROS -----------------------------------
static const char *TAG = "osc_timer";
//-------------------------------- DATA TYPES ---------------------------------
struct _osc_timer_t
{
    timer_callback     _cb;
    int                _period_uc;
    esp_timer_handle_t _periodic_timer;
};

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------

//------------------------- STATIC DATA & CONSTANTS ---------------------------

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------

osc_timer_t *osc_timer_create(timer_callback cb, void *args, int period_uc)
{

    osc_timer_t *_t     = malloc(sizeof(osc_timer_t));
    _t->_cb             = cb;
    _t->_period_uc      = period_uc;
    _t->_periodic_timer = NULL;

    // Create periodic timer
    const esp_timer_create_args_t periodic_timer_args = { .callback = cb, .arg = args };
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &_t->_periodic_timer));

    return _t;
}

/**
 * @brief Starts timer
 *
 * @param timer Instance of timer to start
 */
void osc_timer_start(osc_timer_t *p_timer)
{
    esp_err_t ret = esp_timer_start_periodic(p_timer->_periodic_timer, p_timer->_period_uc);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "OSC timer stop failed: %s", esp_err_to_name(ret));
    }
}

/**
 * @brief Stops timer
 *
 * @param timer Instance of timer to stop
 */
void osc_timer_stop(osc_timer_t *p_timer)
{
    esp_err_t ret = esp_timer_stop(p_timer->_periodic_timer);
    if(ret != ESP_OK)
    {
        ESP_LOGE(TAG, "OSC timer stop failed: %s", esp_err_to_name(ret));
    }
}

void osc_timer_delete(osc_timer_t *p_timer)
{
    free(p_timer);
}
//---------------------------- PRIVATE FUNCTIONS ------------------------------

//---------------------------- INTERRUPT HANDLERS -----------------------------
