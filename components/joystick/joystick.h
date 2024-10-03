/**
* @file joystick.h

* @brief See the source file.

* @par
*
* COPYRIGHT NOTICE: (c) 2022 Byte Lab Grupa d.o.o.
* All rights reserved.
*/

#ifndef __JOYSTICK_H__
#define __JOYSTICK_H__

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------- INCLUDES ----------------------------------
#include <stdio.h>
#include "potentiometer.h"
//---------------------------------- MACROS -----------------------------------
#define JOYSTICK_MAX_POSITION (1000U)
//-------------------------------- DATA TYPES ---------------------------------
typedef enum
{
    JOYSTICK_1, /* The only one on the BL Dev Kit*/

    JOYSTICK_COUNT
} joystick_id_t;

typedef enum
{
    JOYSTICK_ERR_NONE = 0,

    JOYSTICK_ERR                  = -1,
    JOYSTICK_ERR_CREATE           = -2,
    JOYSTICK_ERR_UNKNOWN_INSTANCE = -3,
} joystick_err_t;

typedef struct
{
    uint32_t position_x;
    uint32_t position_y;

} joystick_position_t;

typedef enum
{
    JOYSTICK_MIDDLE,
    JOYSTICK_UP,
    JOYSTICK_LEFT,
    JOYSTICK_RIGHT,
    JOYSTICK_DOWN,

    JOYSTICK_POS_COUNT
} joystick_discrete_position_t;

//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------
/**
 * @brief The function creates the joystick object.
 *
 * @param [in] joystick_id The id of the joystick to create.
 *
 * @return The status of creation.
 */
joystick_err_t joystick_create(joystick_id_t joystick_id);

/**
 * @brief This function returns the joysticks position as a structure containing 2 positions
 * (for x and y axis). Both of the positions can be in the interval [0, JOYSTICK_MAX_POSITION].
 *
 * @param [in] joystick_id The id of the joystick which is being checked.
 *
 * @return Joysticks position structure (for x and y axis positions).
 */
joystick_position_t joystick_position_get(joystick_id_t joystick_id);

/**
 * @brief This function returns the joysticks position as a discrete poition of MIDDLE, UP, DOWN, LEFT or RIGHT
 *
 */
joystick_discrete_position_t joystick_discrete_position_get(joystick_id_t joystick_id);

#ifdef __cplusplus
}
#endif

#endif // __JOYSTICK_H__
