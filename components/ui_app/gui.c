/**
* @file gui.c

* @brief This file is an example for how to use the LVGL library.
*
* COPYRIGHT NOTICE: (c) 2022 Byte Lab Grupa d.o.o.
* All rights reserved.
*/

//--------------------------------- INCLUDES ----------------------------------
#include "gui.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"

/* Littlevgl specific */
#include "lvgl.h"
#include "lvgl_helpers.h"

#include "squareline/ui.h"

#include "button.h"
#include "joystick.h"
#include "led.h"

//---------------------------------- MACROS -----------------------------------
#define LV_TICK_PERIOD_MS (1U)
#define DEBOUNCE_THRESH   (5)

//-------------------------------- DATA TYPES ---------------------------------

/**
 * @brief Struct to hold gui config variables
 *
 */
struct
{
    // button and joystick used as input devices
    button_id_t   physical_btn;
    joystick_id_t joystick;

    // flag for button press, used in indev callback
    bool physical_btn_pressed_flag;

    // current position of joystick and changed status
    joystick_discrete_position_t j_pos;
    bool                         j_pos_changed;

    // focusable object groups for all screens
    lv_group_t *focus_scr_0; // Home screen
    lv_group_t *focus_scr_1; // Function generator screen
    lv_group_t *focus_scr_2; // Oscilloscope screen

    // mutexes
    // semaphore to handle physical button
    SemaphoreHandle_t btn_mutex;

} gui_io;

//---------------------- PRIVATE FUNCTION PROTOTYPES --------------------------
/**
 * @brief It creates a new demo application.
 */
static void _create_application(void);

/**
 * @brief Lv's timer callback function.
 *
 * @param [in] p_arg The argument of the timer.
 */
static void _lv_tick_timer(void *p_arg);

/**
 * @brief Starts GUI task.
 *
 * @param [in] p_parameter Parameter that is passed to the task.
 */
static void _gui_task(void *p_parameter);

/**
 * @brief Initializes joystick and buttons as input devices
 *
 */
static void _gui_indev_init(void);

/**
 * @brief LVGL input device callback function for joystick and buttons, focuses buttons on screen according to joystick position and clicks
 * them according to button pressed status
 *
 * @param drv Initialized by the user and registered by 'lv_indev_add()'
 * @param data Data structure passed to an input driver to fill
 */
static void _gui_indev_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data);

/**
 * @brief Callback function for button 1 press.
 *
 * @param [in] p_arg the argument passed to the ISR when it is registered.
 */
static void _ui_btn_1_callback_isr(void *p_arg);

/**
 * @brief Switches input drivers focus group to current screen's group
 *
 */
static void _gui_switch_indev_group_to_active_screen(void);

//------------------------- STATIC DATA & CONSTANTS ---------------------------
static SemaphoreHandle_t p_gui_semaphore;
static QueueHandle_t     _gui_joystick_queue;

static const char *TAG = "GUI";

//------------------------------- GLOBAL DATA ---------------------------------

// For selecting focused buttons using the joystick and clicking on them with a button
lv_indev_drv_t io_drv;
lv_indev_t    *indev_io;

//------------------------------ PUBLIC FUNCTIONS -----------------------------
void gui_init()
{
    gui_io.joystick     = JOYSTICK_1;
    gui_io.physical_btn = BUTTON_1;
    gui_io.btn_mutex    = xSemaphoreCreateBinary();
    if(gui_io.btn_mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create mutex for button!");
        return;
    }
    xSemaphoreGive(gui_io.btn_mutex);

    // Button 1 initialization
    if(BUTTON_ERR_NONE != button_create(BUTTON_1, _ui_btn_1_callback_isr))
        return;

    /* The ESP32 MCU has got two cores - Core 0 and Core 1, each capable of running tasks independently.
    We want the GUI to run smoothly, without Wi-Fi, Bluetooth and any other task taking its time and therefor
    slowing it down. That's why we need to "pin" the GUI task to it's own core, Core 1.
    Doing so, we reduce the risk of resource conflicts, race conditions and other potential issues.
    * NOTE: When not using Wi-Fi nor Bluetooth, you can pin the GUI task to Core 0.*/
    xTaskCreatePinnedToCore(_gui_task, "gui", 4096 * 2, NULL, 0, NULL, 1);
}

void gui_receive_joystick_pos(int pos)
{
    gui_io.j_pos         = pos;
    gui_io.j_pos_changed = true;
}

//---------------------------- PRIVATE FUNCTIONS ------------------------------

static void _create_application(void)
{
    ui_init();
}

static void _lv_tick_timer(void *p_arg)
{
    (void)p_arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
}

static void _gui_task(void *p_parameter)
{

    (void)p_parameter;
    p_gui_semaphore = xSemaphoreCreateMutex();

    lv_init();

    /* Initialize SPI or I2C bus used by the drivers */
    lvgl_driver_init();

    lv_color_t *p_buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(NULL != p_buf1);

    /* Use double buffered when not working with monochrome displays */
    lv_color_t *p_buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(NULL != p_buf2);
    static lv_disp_draw_buf_t disp_draw_buf;
    uint32_t                  size_in_px = DISP_BUF_SIZE;

    /* Initialize the working buffer */
    lv_disp_draw_buf_init(&disp_draw_buf, p_buf1, p_buf2, size_in_px);

    static lv_disp_drv_t disp_drv;
    disp_drv.hor_res = LV_HOR_RES_MAX;
    disp_drv.ver_res = LV_VER_RES_MAX;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = disp_driver_flush;

    disp_drv.draw_buf = &disp_draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* Register an input device */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = touch_driver_read;
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    lv_indev_drv_register(&indev_drv);

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = { .callback = &_lv_tick_timer, .name = "periodic_gui" };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    /* Create joystick position receive queue */
    _gui_joystick_queue = xQueueCreate(1, sizeof(int));

    // Check if queue  is created successfully.
    if(NULL == _gui_joystick_queue)
    {
        ESP_LOGE(TAG, "Queue not created successfully!");
    }
    /* Create the  application */
    _create_application();

    /* Create input device groups */
    gui_io.focus_scr_0 = lv_group_create();
    gui_io.focus_scr_1 = lv_group_create();
    gui_io.focus_scr_2 = lv_group_create();

    /* Screen 0 focusable objects */
    lv_group_add_obj(gui_io.focus_scr_0, ui_oscbutton1);
    lv_group_add_obj(gui_io.focus_scr_0, ui_fngen1);
    lv_group_add_obj(gui_io.focus_scr_0, ui_wifiBtn);

    /* Screen 1 focusable objects */
    lv_group_add_obj(gui_io.focus_scr_1, ui_signalTypeDropdown);
    lv_group_add_obj(gui_io.focus_scr_1, ui_dutyCycleDropdown);
    lv_group_add_obj(gui_io.focus_scr_1, ui_freqarc1);
    lv_group_add_obj(gui_io.focus_scr_1, ui_amplArc);
    lv_group_add_obj(gui_io.focus_scr_1, ui_startButton);
    lv_group_add_obj(gui_io.focus_scr_1, ui_savePresetBtn);
    lv_group_add_obj(gui_io.focus_scr_1, ui_presetDropdown);
    lv_group_add_obj(gui_io.focus_scr_1, ui_backBtn2);

    /* Screen 2 focusable objects */
    lv_group_add_obj(gui_io.focus_scr_2, ui_ch1Btn);
    lv_group_add_obj(gui_io.focus_scr_2, ui_ch1Btn1);
    lv_group_add_obj(gui_io.focus_scr_2, ui_togglemsBtn);
    lv_group_add_obj(gui_io.focus_scr_2, ui_togglemVBtn);
    lv_group_add_obj(gui_io.focus_scr_2, ui_backbtn);

    // Initialize joystick as gui input device
    _gui_indev_init();

    for(;;)
    {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));

        /* Try to take the semaphore, call lvgl related function on success */
        if(pdTRUE == xSemaphoreTake(p_gui_semaphore, portMAX_DELAY))
        {
            lv_task_handler();
            xSemaphoreGive(p_gui_semaphore);
        }
    }

    /* A task should NEVER return */
    free(p_buf1);
    free(p_buf2);
    vTaskDelete(NULL);
}

static void _gui_indev_init()
{
    lv_indev_drv_init(&io_drv);                       // Initialize input device driver
    io_drv.type    = LV_INDEV_TYPE_KEYPAD;            // Set the input type to keypad-like (for up, down, left, right navigation)
    io_drv.read_cb = _gui_indev_read_cb;              // Set the read callback for joystick
    indev_io       = lv_indev_drv_register(&io_drv);  // Register the joystick as an input device
    lv_indev_set_group(indev_io, gui_io.focus_scr_0); // Register joystick as input device for group
}

static void _gui_indev_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{

    _gui_switch_indev_group_to_active_screen();

    BaseType_t task = pdFALSE;

    // Check Button 1 press
    BaseType_t higher_priority_task_woken = pdFALSE;
    xSemaphoreTakeFromISR(gui_io.btn_mutex, portMAX_DELAY);
    if(true == gui_io.physical_btn_pressed_flag)
    {
        data->key                        = LV_KEY_ENTER;           // Simulate the ENTER (click) key
        data->state                      = LV_INDEV_STATE_PRESSED; // Mark as pressed
        gui_io.physical_btn_pressed_flag = false;                  // Reset flag
        ESP_LOGI(TAG, "Button 1 pressed");
        xSemaphoreGive(gui_io.btn_mutex);
        return;
    }
    xSemaphoreGiveFromISR(gui_io.btn_mutex, &higher_priority_task_woken);

    // Perform a context switch if necessary
    portYIELD_FROM_ISR(higher_priority_task_woken);

    // Read the joystick state
    static joystick_discrete_position_t pos = JOYSTICK_MIDDLE;
    if(!gui_io.j_pos_changed)
        return;
    gui_io.j_pos_changed = false;
    pos                  = gui_io.j_pos;

    // Map joystick movement to LVGL keys
    switch(pos)
    {
        case JOYSTICK_UP:
            data->key   = LV_KEY_UP;
            data->state = LV_INDEV_STATE_PRESSED;
            ESP_LOGI(TAG, "joystick UP");
            break;
        case JOYSTICK_DOWN:
            data->key   = LV_KEY_DOWN;
            data->state = LV_INDEV_STATE_PRESSED;
            ESP_LOGI(TAG, "joystick DOWN");

            break;
        case JOYSTICK_LEFT:
            data->key   = LV_KEY_PREV;
            data->state = LV_INDEV_STATE_PRESSED;
            ESP_LOGI(TAG, "joystick LEFT");

            break;
        case JOYSTICK_RIGHT:
            data->key   = LV_KEY_NEXT;
            data->state = LV_INDEV_STATE_PRESSED;
            ESP_LOGI(TAG, "joystick RIGHT");

            break;
        default:
            data->state = LV_INDEV_STATE_RELEASED; // No movement
            break;
    }

    return;
}

static void _gui_switch_indev_group_to_active_screen(void)
{
    lv_obj_t *active_screen = lv_scr_act(); // Get active screen
    if(ui_homescr == active_screen)
    {
        lv_indev_set_group(indev_io, gui_io.focus_scr_0); // Switch indev focus group to homescreen
    }
    else if(ui_functiongenscr == active_screen)
    {
        lv_indev_set_group(indev_io, gui_io.focus_scr_1); // Switch indev focus group to oscilloscope screen
    }
    else if(ui_oscilloscopescr == active_screen)
    {
        lv_indev_set_group(indev_io, gui_io.focus_scr_2); // Switch indev focus group to function generator screen
    }
}

//---------------------------- INTERRUPT HANDLERS -----------------------------

static void IRAM_ATTR _ui_btn_1_callback_isr(void *p_arg)
{
    // Debouncing
    static int64_t time_ms = 0;
    if(esp_timer_get_time() - time_ms < DEBOUNCE_THRESH)
        return;
    time_ms = esp_timer_get_time();

    BaseType_t higher_priority_task_woken = pdFALSE;

    // Protect flag change with mutex
    xSemaphoreTakeFromISR(gui_io.btn_mutex, portMAX_DELAY);
    gui_io.physical_btn_pressed_flag = true;
    xSemaphoreGiveFromISR(gui_io.btn_mutex, &higher_priority_task_woken);

    // Perform a context switch if necessary
    portYIELD_FROM_ISR(higher_priority_task_woken);
}
