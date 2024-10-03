/**
 * @file timer.h
 *
 * @brief See the source file.
 *
 * COPYRIGHT NOTICE: (c) 2024 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------
#include "driver/gptimer.h"

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Initializes esp general purpose timer for singal generation
 *
 * @param alarm_count_us Execution time of each ISR interval in micro-seconds
 * @param cb Timer callback function
 */
void timer_init(int alarm_count_us, gptimer_alarm_cb_t cb);

void timer_stop();
void timer_start();

#ifdef __cplusplus
}
#endif

#endif // __TIMER_H__
