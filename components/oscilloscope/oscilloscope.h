/**
 * @file oscilloscope.h
 *
 * @brief See the source file.
 *
 * COPYRIGHT NOTICE: (c) 2024 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

#ifndef __OSCILLOSCOPE_H__
#define __OSCILLOSCOPE_H__

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------
#include "potentiometer.h"
#include "timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

//---------------------------------- MACROS -----------------------------------

#define OSCILLOSCOPE_TIMER_PERIOD_US (250) // period of callback timer, how long it takes between adc readings
#define OSCILLOSCOPE_WINDOW_MS       (50)  // how long will it do adc readings
#define OSCILLOSCOPE_SAMPLE_NUMBER \
    (OSCILLOSCOPE_WINDOW_MS * 1000 / OSCILLOSCOPE_TIMER_PERIOD_US) // how many samples its going to take in that time

//-------------------------------- DATA TYPES ---------------------------------
struct _oscilloscope_t;
typedef struct _oscilloscope_t oscilloscope_t;

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Creates new instance of oscilloscope that records signal on channel 
 * 
 * @param channel_number channel to record signal from
 * @return oscilloscope_t* handle of oscilloscope
 */
oscilloscope_t *oscilloscope_create(int pin, int channel_number);


/**
 * @brief Starts timer that reads adc values and fills up buffer
 * 
 * @param p_osc 
 */
void oscilloscope_start(oscilloscope_t *p_osc);

/**
 * @brief Stops timer that reads adc values
 * 
 * @param p_osc 
 */
void oscilloscope_stop(oscilloscope_t *p_osc);

/**
 * @brief Copies most recent adc reading buffer to data
 * 
 * @param p_osc Oscilloscope handler which to read from
 * @param data [out] Pointer to empy data
 */
void oscilloscope_send_new_data(oscilloscope_t *p_osc, int *data);

void oscilloscope_print(oscilloscope_t *p_osc);

#ifdef __cplusplus
}
#endif

#endif // __OSCILLOSCOPE_H__
