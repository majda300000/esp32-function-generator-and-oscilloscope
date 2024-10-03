
idf_component_register(SRCS "fn_gen.c" "platform/src/dac.c" "platform/src/timer.c"
                  INCLUDE_DIRS "platform/inc" "."
                  REQUIRES driver)
