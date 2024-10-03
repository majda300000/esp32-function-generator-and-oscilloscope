set(COMPONENT_SRCS "gui.c" "ui_app.c" "osc_chart/osc_chart.c"
                    "squareline/ui_helpers.c"
                    "squareline/ui.c"
                    "squareline/ui_events.c"
                    "squareline/components/ui_comp_hook.c"

                    #SquareLine images
                    "squareline/images/ui_img_653708034.c" 
                    "squareline/images/ui_img_bl_logo_light_png.c"
                    "squareline/images/ui_img_1091903730.c"
                    "squareline/images/ui_img_bl_logo_symbol_png.c"
                    "squareline/images/ui_img_humidity_15px_png.c"
                    "squareline/images/ui_img_temp_15px_png.c"
                    "squareline/images/ui_img_wifi_iconmini_png.c"
                    "squareline/images/ui_img_1501662095.c"




                    #SquareLine screens 
                    "squareline/screens/ui_functiongenscr.c" 
                    "squareline/screens/ui_homescr.c" 
                    "squareline/screens/ui_oscilloscopescr.c"


                    #Squareline components
                    "squareline/components/ui_comp_freqlabel.c"
                    "squareline/components/ui_comp_hook.c"
                    "squareline/components/ui_comp.c")
set(COMPONENT_ADD_INCLUDEDIRS ""  "squareline/" "osc_chart/")
set(COMPONENT_PRIV_REQUIRES lvgl lvgl_esp32_drivers esp_timer button joystick led function_generator potentiometer oscilloscope temp_sensor_sht31)

register_component()
