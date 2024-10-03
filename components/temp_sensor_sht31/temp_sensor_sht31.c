/**
 * @file temp_sensor_sht31.c
 *
 * @brief Driver for the SHT31 temperature and humidity sensor using I2C protocol.
 *
 * COPYRIGHT NOTICE: (c) 2024 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "temp_sensor_sht31.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/i2c.h"

//---------------------------------- MACROS -----------------------------------

#define I2C_SCL_IO          21            ///< I2C SCL pin number
#define I2C_SDA_IO          22            ///< I2C SDA pin number
#define I2C_NUM             I2C_NUM_0     ///< I2C port number
#define I2C_TX_BUF_DISABLE  0             ///< I2C TX buffer disable
#define I2C_RX_BUF_DISABLE  0             ///< I2C RX buffer disable
#define I2C_FREQ_HZ         100000        ///< I2C clock frequency

#define SHT31_SENSOR_ADDR   0x44          ///< SHT31 sensor I2C address
#define SHT31_MEASURE_HIGH  0x2130        ///< Command for high precision measurement
#define SHT31_READ          0xE000        ///< Command to read measurement results

static const char *TAG = "SHT31"; 

//-------------------------------- DATA TYPES ---------------------------------

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
/**
 * @brief Initialize the I2C interface
 *
 * Configures and installs the I2C driver for communication with the SHT31 sensor.
 *
 * @return
 *    - ESP_OK: Success
 *    - Others: Failure
 */
static esp_err_t _temp_sensor_i2c_init(void);        

//------------------------- STATIC DATA & CONSTANTS ---------------------------

//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------

esp_err_t temp_sensor_init(void)
{
    esp_err_t err;

    // Initialize I2C interface
    err = _temp_sensor_i2c_init();
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C initialization failed");
        return err;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SHT31_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, (SHT31_MEASURE_HIGH >> 8) & 0xFF, true);
    i2c_master_write_byte(cmd, SHT31_MEASURE_HIGH & 0xFF, true);
    i2c_master_stop(cmd);
    
    err = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    return err;
}

esp_err_t temp_sensor_read(float *temperature, float *humidity)
{
    uint8_t raw_data[4]; // Buffer for raw sensor data
    esp_err_t err;

    // Send read command to SHT31
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SHT31_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, (SHT31_READ >> 8) & 0xFF, true);
    i2c_master_write_byte(cmd, SHT31_READ & 0xFF, true);
    i2c_master_stop(cmd);

    err = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (err != ESP_OK)
    {
        return err;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for measurement to complete

    // Read data from the sensor
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (SHT31_SENSOR_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, raw_data, 4, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    err = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (err != ESP_OK)
    {
        return err;
    }

    // Convert raw data to temperature and humidity
    *temperature = (float)(-45 + (175 * ((raw_data[0] << 8) | raw_data[1])) / 65535.0);
    *humidity = (float)(((raw_data[0] << 8) | raw_data[1]) * 100.0 / 65535.0);

    return ESP_OK;
}


//---------------------------- PRIVATE FUNCTIONS ------------------------------


static esp_err_t _temp_sensor_i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ
    };
    
    esp_err_t err = i2c_param_config(I2C_NUM, &conf);
    if (err != ESP_OK)
    {
        return err;
    }

    return i2c_driver_install(I2C_NUM, conf.mode, I2C_RX_BUF_DISABLE, I2C_TX_BUF_DISABLE, 0);
}

