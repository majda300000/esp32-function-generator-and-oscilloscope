/**
 * @file fn_gen.h
 *
 * @brief See the source file.
 *
 * COPYRIGHT NOTICE: (c) 2024 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

#ifndef __FN_GEN_H__
#define __FN_GEN_H__

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------
#include <stdint.h>
#include <stdbool.h>
//---------------------------------- MACROS -----------------------------------
#define FN_GEN_POINT_ARR_LEN 200 // Length of points array
#define FN_GEN_TIMER_INTR_US 30  // Execution time of each ISR interval in micro-seconds
#define FN_GEN_PRESET_NUMBER 5   // Number of signal presets

#define VDD     3300 // VDD is 3.3V, 3300mV
#define AMP_DAC 255  // Amplitude of DAC voltage. If it's more than 256 will causes dac_output_voltage() output 0.

//-------------------------------- DATA TYPES ---------------------------------

typedef enum
{
    FN_GEN_ERR_NONE = 0,

    FN_GEN_ERR                = -1,
    FN_GEN_ERR_UNKNOWN_SIGNAL = -2,
    FN_GEN_ERR_CREATE         = -3,
} fn_gen_error_t;

typedef enum
{
    FN_SIGNAL_SINE,
    FN_SIGNAL_SQUARE,
    FN_SIGNAL_TRIANGLE,
    FN_SIGNAL_SAWTOOTH,

    FN_SIGNAL_COUNT
} fn_signal_type_t;

typedef struct _fn_signal_config_t
{
    fn_signal_type_t signal;
    int              frequency_Hz;
    int              amplitude_mV; // Peak to peak
    int              duty_cycle_percentage;
} fn_signal_config_t;

typedef struct _fn_generator_t
{
    fn_signal_config_t _config;
    int                _raw_val[FN_GEN_POINT_ARR_LEN];  // Used to store raw values
    int                _volt_val[FN_GEN_POINT_ARR_LEN]; // Used to store voltage values(in mV)
    int                _period_index;                   // Time point in window frame of signal
    bool               _is_running;                     // True if the timer is running
    fn_signal_config_t presets[FN_GEN_PRESET_NUMBER];   // Signal presets
} fn_generator_t;

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Initializes function generator module
 *
 */
void fn_gen_init();

/**
 * @brief Sets parameters of signal to be generated
 *
 * @param config Signal parameters
 * @return fn_gen_err_t
 */
fn_gen_error_t fn_gen_set_signal_config(fn_signal_config_t config);

/**
 * @brief Sets signal's type
 *
 * @param type enum type fn_signal_type_t
 * @return fn_gen_error_t
 */
fn_gen_error_t fn_gen_set_signal_type(fn_signal_type_t type);

/**
 * @brief Sets signal's frequency
 *
 * @param frequency_Hz frequency in Hz from 0 to 3000 Hz
 * @return fn_gen_error_t
 */
fn_gen_error_t fn_gen_set_frequency(int frequency_Hz);

/**
 * @brief Sets signal's amplitude
 *
 * @param amplitude_mV_pp Aplitude in mV from 0 to 3300 mV
 * @return fn_gen_error_t
 */
fn_gen_error_t fn_gen_set_amplitude(int amplitude_mV_pp);

/**
 * @brief Sets square wave's duty cycle
 *
 * @param duty_cycle_percentage from 0% to 100%
 * @return fn_gen_error_t
 */

fn_gen_error_t fn_gen_set_duty_cycle(int duty_cycle_percentage);

/**
 * @brief Saves config for preset number preset_num
 *
 * @param config config to be saved
 * @param preset_num number of preset
 * @return fn_gen_error_t
 */
fn_gen_error_t fn_gen_set_preset(fn_signal_config_t config, int preset_num);

/**
 * @brief Returns config at preset with index preset_num
 * 
 * @param conf Pointer to place the config
 * @param preset_num index
 * @return fn_gen_error_t 
 */
fn_gen_error_t fn_gen_get_preset(fn_signal_config_t *conf, int preset_num);

/**
 * @brief Sets signal config to preset with the index of preset_num
 *
 * @param preset_num
 * @return fn_gen_error_t
 */
fn_gen_error_t fn_gen_load_preset(int preset_num);

/**
 * @brief Starts outputing signal to DAC
 *
 * @return fn_gen_err_t
 */
fn_gen_error_t fn_gen_signal_start_task();

/**
 * @brief Stops outputing signal to DAC
 *
 * @return fn_gen_err_t
 */
fn_gen_error_t fn_gen_signal_stop_task();

#ifdef __cplusplus
}
#endif

#endif // __FN_GEN_H__
