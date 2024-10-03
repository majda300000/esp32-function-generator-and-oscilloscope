/**
 * @file dac.h
 *
 * @brief See the source file.
 *
 * COPYRIGHT NOTICE: (c) 2024 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

#ifndef __DAC_H__
#define __DAC_H__

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------
#include "driver/dac.h"
//---------------------------------- MACROS -----------------------------------

#define ESP_DAC_CHAN DAC_CHANNEL_1

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Enables esp DAC peripheral
 *
 */
void dac_init(void);

/**
 * @brief Outputs 8 bit value to DAC
 *
 * @param dac_value 8 bit value
 */
void dac_output(uint8_t dac_value);

#ifdef __cplusplus
}
#endif

#endif // __DAC_H__
