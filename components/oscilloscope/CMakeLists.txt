
idf_component_register(SRCS "oscilloscope.c" "platform/src/timer.c"
                  INCLUDE_DIRS "platform/inc" "."
                  REQUIRES driver esp_timer joystick)
