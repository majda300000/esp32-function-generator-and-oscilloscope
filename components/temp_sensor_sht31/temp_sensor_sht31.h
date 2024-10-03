/**
* @file temp_sensor_sht31.h
*
* @brief See the source file.
* 
* COPYRIGHT NOTICE: (c) 2024 Byte Lab Grupa d.o.o.
* All rights reserved.
*/

#ifndef __TEMP_SENSOR_SHT31_H__
#define __TEMP_SENSOR_SHT31_H__

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------
#include "esp_system.h"
#include "esp_log.h"
//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Initialize the SHT31 sensor
 *
 * Sends the high precision measurement command to the SHT31 sensor.
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Failure
 */
esp_err_t temp_sensor_init(void);

/**
 * @brief Read temperature and humidity from SHT31 sensor
 *
 * Reads the measurement data from the SHT31 sensor and converts the raw values
 * into temperature (°C) and humidity (%).
 *
 * @param[out] temperature Pointer to store the temperature value
 * @param[out] humidity Pointer to store the humidity value
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Failure
 */
esp_err_t temp_sensor_read(float *temperature, float *humidity);

#ifdef __cplusplus
}
#endif

#endif // __TEMP_SENSOR_SHT31_H__
