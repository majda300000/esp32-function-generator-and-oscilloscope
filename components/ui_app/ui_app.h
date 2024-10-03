/**
 * @file ui_app.h
 *
 * @brief See the source file.
 *
 * COPYRIGHT NOTICE: (c) 2024 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

#ifndef __UI_APP_H__
#define __UI_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Starts application
 *
 */
void app_start(void);

/**
 * @brief Turns off adc reading while function generator is active
 *
 */
void ui_turn_off_oscilloscope(void);

/**
 * @brief Turns adc back on while function generator stops
 *
 */
void ui_turn_on_oscilloscope(void);

#ifdef __cplusplus
}
#endif

#endif // __UI_APP_H__
