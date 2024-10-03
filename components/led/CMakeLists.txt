set(COMPONENT_SRCS "led.c" "platform/src/led_gpio.c")
set(COMPONENT_ADD_INCLUDEDIRS "platform/inc" ".")
set(COMPONENT_REQUIRES driver)

register_component()
