/**
* @file osc_chart.h
*
* @brief See the source file.
* 
* COPYRIGHT NOTICE: (c) 2024 Byte Lab Grupa d.o.o.
* All rights reserved.
*/

#ifndef __OSC_CHART_H__
#define __OSC_CHART_H__

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------

#include "oscilloscope.h"
#include "ui.h"
//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------
typedef struct {

    // Two oscilloscopes serving as two channels
    oscilloscope_t *p_chan_1;
    oscilloscope_t *p_chan_2;

    // Series for the two channels
    lv_chart_series_t *p_ser1;
    lv_chart_series_t *p_ser2;

    // Voltage and time divisions
    int div_mV;
    int div_ms;

    // Number of points shown on chart
    int  data_length;

    // Pointer to data to display
    int *data_1;
    int *data_2;

    // lvgl chart object
    lv_obj_t *chart;



} osc_chart_t;
//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Initializes char unit
 * 
 * @param chart Pointer to lvgl chart object
 * @param p_osc1 pointer to oscilloscope object for channel A
 * @param p_osc2 pointer to oscilloscope object for channel B
 * @return esp_err_t 
 */
esp_err_t osc_chart_init(lv_obj_t *chart, oscilloscope_t *p_osc1, oscilloscope_t *p_osc2);

/**
 * @brief Displays channel 1 on chart
 * 
 */
void osc_chart_ch1_show(void);

/**
 * @brief Hides channel 2 from chart
 * 
 */
void osc_chart_ch1_hide(void);

/**
 * @brief Displays channel 2 on chart
 * 
 */
void osc_chart_ch2_show(void);

/**
 * @brief Hides channel 2 from chart
 * 
 */
void osc_chart_ch2_hide(void);

/**
 * @brief Sets shart divX to 10ms
 * 
 */
void ui_set_div_10ms(void);

/**
 * @brief Sets shart divX to 1ms
 * 
 */
void ui_set_div_1ms(void);

/**
 * @brief Sets shart divY to 500mV
 * 
 */
void ui_set_div_500mV(void);

/**
 * @brief Sets shart divY to 100mV
 * 
 */
void ui_set_div_100mV(void);




#ifdef __cplusplus
}
#endif

#endif // __OSC_CHART_H__
