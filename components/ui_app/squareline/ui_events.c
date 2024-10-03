// This file was generated by SquareLine Studio
// SquareLine Studio version: SquareLine Studio 1.4.2
// LVGL version: 8.3.6
// Project name: mashina_sq_fn_gen_screen_almost_work

#include "ui.h"
#include "../ui_app.h"
#include "../osc_chart/osc_chart.h"
#include "fn_gen.h"
#include "led.h"
#include "esp_log.h"

#define DUTY_CYCLE_INCREMENT (5)

void ui_signal_type_dropdown_cb(lv_event_t *p_e)
{
    lv_obj_t *dropdown = lv_event_get_current_target(p_e);

    // Get the index of the selected option
    // Selected index corresponds to enum index
    int selected_index = lv_dropdown_get_selected(dropdown);

    fn_gen_set_signal_type(selected_index);
}

void ui_freq_arc_cb(lv_event_t *p_e)
{
    lv_obj_t *arc = lv_event_get_target(p_e);

    // Get the current value of the arc (its angle or progress)
    int value = lv_arc_get_value(arc);

    fn_gen_set_frequency(value);
}
void ui_ampl_arc_cb(lv_event_t *p_e)
{
    lv_obj_t *arc = lv_event_get_target(p_e);

    // Get the current value of the arc (its angle or progress)
    int value = lv_arc_get_value(arc);

    fn_gen_set_amplitude(value);
}

void ui_start_btn_checked_cb(lv_event_t *p_e)
{

    // Start outputting signal and stop oscilloscope
    fn_gen_signal_start_task();
    led_pattern_run(LED_RED, LED_PATTERN_FASTBLINK, NO_TIMEOUT); // Pattern for FG
    ui_turn_off_oscilloscope();
}

void ui_start_btn_unchecked_cb(lv_event_t *p_e)
{
    // Stop outputting signal and stae oscilloscope
    fn_gen_signal_stop_task();
    led_pattern_reset(LED_RED);
    led_pattern_run(LED_RED, LED_PATTERN_KEEP_ON, NO_TIMEOUT);
    ui_turn_on_oscilloscope();
}

void ui_duty_cycle_dropdown_cb(lv_event_t *p_e)
{
    lv_obj_t *dropdown = lv_event_get_current_target(p_e);

    // Get the index of the selected option
    // Selected index corresponds to enum index
    int selected_index = lv_dropdown_get_selected(dropdown);

    fn_gen_set_duty_cycle(selected_index * DUTY_CYCLE_INCREMENT);
}

void ch1_show(lv_event_t *e)
{
    osc_chart_ch1_show();
}

void ch1_hide(lv_event_t *e)
{
    osc_chart_ch1_hide();
}

void ch2_show(lv_event_t *e)
{
    osc_chart_ch2_show();
}

void ch2_hide(lv_event_t *e)
{
    osc_chart_ch2_hide();
}

void set_div_10ms(lv_event_t *e)
{

    ui_set_div_10ms();
}

void set_div_1ms(lv_event_t *e)
{

    ui_set_div_1ms();
}

void set_div_500mV(lv_event_t *e)
{

    ui_set_div_500mV();
}

void set_div_100mV(lv_event_t *e)
{
    ui_set_div_100mV();
}

void ui_save_preset(lv_event_t *e)
{

    fn_signal_config_t config = { .amplitude_mV          = lv_arc_get_value(ui_amplArc),
                                  .frequency_Hz          = lv_arc_get_value(ui_freqarc1),
                                  .signal                = lv_dropdown_get_selected(ui_signalTypeDropdown),
                                  .duty_cycle_percentage = lv_dropdown_get_selected(ui_dutyCycleDropdown) * 5 };

    int preset_num = lv_dropdown_get_selected(ui_presetDropdown);

    fn_gen_set_preset(config, preset_num);
}



void ui_init_preset(lv_event_t * e)
{
	ui_load_preset(NULL);
}

void ui_load_preset(lv_event_t *p_e)
{
    int preset_num = lv_dropdown_get_selected(ui_presetDropdown);

    fn_signal_config_t conf;
    fn_gen_get_preset(&conf, preset_num);

    lv_dropdown_set_selected(ui_signalTypeDropdown, conf.signal);
    lv_dropdown_set_selected(ui_dutyCycleDropdown, conf.duty_cycle_percentage / 5);
    lv_arc_set_value(ui_freqarc1, conf.frequency_Hz);
    lv_arc_set_value(ui_amplArc, conf.amplitude_mV);

    char labelF[10];
    char labelA[10];

    sprintf(labelF, "%d", conf.frequency_Hz);
    lv_label_set_text(ui_freqLabel1, labelF);

    sprintf(labelA, "%d", conf.amplitude_mV);
    lv_label_set_text(ui_amplLabel, labelA);

    fn_gen_load_preset(preset_num);

    ESP_LOGI("EVENTS:",
             "Loading preset:\n Frequency = %d\n Amplitude = %d\n Duty cycle = %d\n Signal type = %d\n amplabel = %s\n freqlabel = %s\n",
             conf.frequency_Hz,
             conf.amplitude_mV,
             conf.duty_cycle_percentage,
             conf.signal,
             labelA,
             labelF);
}


