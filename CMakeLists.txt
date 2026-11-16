idf_component_register(SRCS "my_guitar_fx_effects.c" "dbfs_meter.c" "tuner.c" "delay.c" "distortion.c" "src/tuner_ui.c"
                    INCLUDE_DIRS "include"
                    REQUIRES my_guitar_fx_ui)