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

//---------------------------------- MACROS -----------------------------------

//-------------------------------- DATA TYPES ---------------------------------
typedef void (*timer_callback)(void *arg);

struct _osc_timer_t;
typedef struct _osc_timer_t osc_timer_t;


//---------------------- PUBLIC FUNCTION PROTOTYPES --------------------------

/**
 * @brief Creates new instance of esp timer
 * 
 * @param cb The timer callback function
 * @param period_uc Period for timer callback in microseconds
 * @param args Callback function arguments
 * @return osc_timer_t* Pointer to the new timer
 */
osc_timer_t *osc_timer_create(timer_callback cb,  void* args, int period_uc);

/**
 * @brief Starts timer
 * 
 * @param timer Instance of timer to start
 */
void osc_timer_start(osc_timer_t *p_timer);

/**
 * @brief Stops timer
 * 
 * @param timer Instance of timer to stop
 */
void osc_timer_stop(osc_timer_t *p_timer);

/**
 * @brief Deletes timer instance and frees memory
 * 
 * @param timer Timer to delete
 */
void osc_timer_delete(osc_timer_t *p_timer);


#ifdef __cplusplus
}
#endif

#endif // __TIMER_H__
