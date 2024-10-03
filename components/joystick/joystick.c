/**
 * @file joystick.c
 *
 * @brief This file implements Joystick as two potentiometers
 *
 * COPYRIGHT NOTICE: (c) 2023 Byte Lab Grupa d.o.o.
 * All rights reserved.
 */

//--------------------------------- INCLUDES ----------------------------------
#include "joystick.h"
#include "stdbool.h"
//---------------------------------- MACROS -----------------------------------
#define JOYSTICK_THRESHOLD_CENTER_LOW  400 // Lower threshold for center position
#define JOYSTICK_THRESHOLD_CENTER_HIGH 600 // Upper threshold for center position
//-------------------------------- DATA TYPES ---------------------------------
typedef struct
{
    uint8_t  pin_x;
    uint8_t  chan_x;
    uint8_t  pin_y;
    uint8_t  chan_y;
    uint16_t max_voltage_mv; /* Max voltage in mV*/
    bool     b_is_reversed_x;
    bool     b_is_reversed_y;

    potentiometer_t *p_pot_x;
    potentiometer_t *p_pot_y;

} _joystick_config_t;
//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------

/**
 * @brief Decote coordinates to discrete joystick positions
 *
 * @param joystick_position Coordinates struct
 * @return joystick_discrete_position_t Discrete position
 */
static joystick_discrete_position_t _decode_joystick_position(joystick_position_t joystick_position);

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static _joystick_config_t _joystick_info[JOYSTICK_COUNT] = {
    // 2 Potentiometers
    { .pin_x = 34, .chan_x = 6, .pin_y = 35, .chan_y = 7, .max_voltage_mv = 3300, .b_is_reversed_x = true, .b_is_reversed_y = true },
};
//------------------------------- GLOBAL DATA ---------------------------------

//------------------------------ PUBLIC FUNCTIONS -----------------------------
joystick_err_t joystick_create(joystick_id_t joystick_id)
{

    /* Validate joystick name*/
    if(JOYSTICK_COUNT <= joystick_id)
    {
        return JOYSTICK_ERR_UNKNOWN_INSTANCE;
    }

    /* Create an object using the function from potentiometer.c (Two potentiometers for one joystick)*/
    _joystick_config_t _conf   = _joystick_info[joystick_id];
    potentiometer_t   *p_pot_x = potentiometer_create(_conf.pin_x, _conf.chan_x, _conf.max_voltage_mv, _conf.b_is_reversed_x);
    potentiometer_t   *p_pot_y = potentiometer_create(_conf.pin_y, _conf.chan_y, _conf.max_voltage_mv, _conf.b_is_reversed_y);

    /* Check if the object was created successfully*/
    if(NULL == p_pot_x || NULL == p_pot_y)
    {
        return JOYSTICK_ERR_CREATE;
    }

    /* Save the potentiometer pointers, might need to expand _joystick_config_t struct*/
    _joystick_info[joystick_id].p_pot_x = p_pot_x;
    _joystick_info[joystick_id].p_pot_y = p_pot_y;

    /* Return status of initialization*/

    return JOYSTICK_ERR_NONE;
}

joystick_position_t joystick_position_get(joystick_id_t joystick_id)
{

    joystick_position_t joystick_position;
    joystick_position.position_x = potentiometer_position_get(_joystick_info[joystick_id].p_pot_x);
    joystick_position.position_y = potentiometer_position_get(_joystick_info[joystick_id].p_pot_y);

    return joystick_position;
}

joystick_discrete_position_t joystick_discrete_position_get(joystick_id_t joystick_id)
{
    return _decode_joystick_position(joystick_position_get(joystick_id));
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static joystick_discrete_position_t _decode_joystick_position(joystick_position_t joystick_position)
{
    uint32_t x = joystick_position.position_x;
    uint32_t y = joystick_position.position_y;

    // Check Y-axis first (UP/DOWN)
    if(y > JOYSTICK_THRESHOLD_CENTER_HIGH)
    {
        return JOYSTICK_DOWN;
    }
    else if(y < JOYSTICK_THRESHOLD_CENTER_LOW)
    {
        return JOYSTICK_UP;
    }
    // Check X-axis (LEFT/RIGHT)
    else if(x < JOYSTICK_THRESHOLD_CENTER_LOW)
    {
        return JOYSTICK_RIGHT;
    }
    else if(x > JOYSTICK_THRESHOLD_CENTER_HIGH)
    {
        return JOYSTICK_LEFT;
    }

    // If X and Y are within the center range
    return JOYSTICK_MIDDLE;
}

//---------------------------- INTERRUPT HANDLERS -----------------------------
