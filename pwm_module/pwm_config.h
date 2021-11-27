/**
 * @file pwm_config.h
 * @brief This file is used to define the default config for nrfx PWM driver.
 *
 * Note: IDK what is better to use: macro definiton or definition in main.c.
 */

#include "nrfx_pwm.h"

/* defines common for all boards */
#define PWM_RGB_TOP_VALUE                     255
#define PWM_RGB_CYCLES_FOR_ONE_STEP           100
#define PWM_INDICATOR_TOP_VALUE               1024
#define PWM_INDICATOR_CYCLES_FOR_ONE_STEP     2

/* I think that we can add another board after that if needed */
#ifdef BOARD_PCA10059
#include "pca10059.h"

#ifdef NRFX_PWM_DEFAULT_CONFIG_OUT0_PIN
#undef NRFX_PWM_DEFAULT_CONFIG_OUT0_PIN
#define NRFX_PWM_DEFAULT_CONFIG_OUT0_PIN          (LED_1 | NRFX_PWM_PIN_INVERTED)
#endif /* NRFX_PWM_DEFAULT_CONFIG_OUT0_PIN */

#ifdef NRFX_PWM_DEFAULT_CONFIG_OUT1_PIN
#undef NRFX_PWM_DEFAULT_CONFIG_OUT1_PIN
#define NRFX_PWM_DEFAULT_CONFIG_OUT1_PIN          (LED_2 | NRFX_PWM_PIN_INVERTED)
#endif /* NRFX_PWM_DEFAULT_CONFIG_OUT1_PIN */

#ifdef NRFX_PWM_DEFAULT_CONFIG_OUT2_PIN
#undef NRFX_PWM_DEFAULT_CONFIG_OUT2_PIN
#define NRFX_PWM_DEFAULT_CONFIG_OUT2_PIN          (LED_3 | NRFX_PWM_PIN_INVERTED)
#endif /* NRFX_PWM_DEFAULT_CONFIG_OUT2_PIN */

#ifdef NRFX_PWM_DEFAULT_CONFIG_OUT3_PIN
#undef NRFX_PWM_DEFAULT_CONFIG_OUT3_PIN
#define NRFX_PWM_DEFAULT_CONFIG_OUT3_PIN          (LED_4 | NRFX_PWM_PIN_INVERTED)
#endif /* NRFX_PWM_DEFAULT_CONFIG_OUT3_PIN */

#ifdef NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY
#undef NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY
#define NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY      4
#endif /* NRFX_PWM_DEFAULT_CONFIG_IRQ_PRIORITY */

#ifdef NRFX_PWM_DEFAULT_CONFIG_BASE_CLOCK
#undef NRFX_PWM_DEFAULT_CONFIG_BASE_CLOCK
#define NRFX_PWM_DEFAULT_CONFIG_BASE_CLOCK     NRF_PWM_CLK_250kHz
#endif /* NRFX_PWM_DEFAULT_CONFIG_BASE_CLOCK */

#ifdef NRFX_PWM_DEFAULT_CONFIG_COUNT_MODE
#undef NRFX_PWM_DEFAULT_CONFIG_COUNT_MODE
#define NRFX_PWM_DEFAULT_CONFIG_COUNT_MODE     NRF_PWM_MODE_UP
#endif /* NRFX_PWM_DEFAULT_CONFIG_COUNT_MODE */

#ifdef NRFX_PWM_DEFAULT_CONFIG_TOP_VALUE
#undef NRFX_PWM_DEFAULT_CONFIG_TOP_VALUE
#define NRFX_PWM_DEFAULT_CONFIG_TOP_VALUE      PWM_RGB_TOP_VALUE
#endif /* NRFX_PWM_DEFAULT_CONFIG_TOP_VALUE */

#ifdef NRFX_PWM_DEFAULT_CONFIG_LOAD_MODE
#undef NRFX_PWM_DEFAULT_CONFIG_LOAD_MODE
#define NRFX_PWM_DEFAULT_CONFIG_LOAD_MODE      NRF_PWM_LOAD_INDIVIDUAL
#endif /* NRFX_PWM_DEFAULT_CONFIG_LOAD_MODE */

#ifdef NRFX_PWM_DEFAULT_CONFIG_STEP_MODE
#undef NRFX_PWM_DEFAULT_CONFIG_STEP_MODE
#define NRFX_PWM_DEFAULT_CONFIG_STEP_MODE      NRF_PWM_STEP_AUTO
#endif /* NRFX_PWM_DEFAULT_CONFIG_STEP_MODE */

#endif /* BOARD_PCA10059 */

/**
 * @brief macro for sequence config definition
 * @param seq_values uint16_t array with @ref NRF_PWM_CHANNEL_COUNT size
 *
 * Note: Maybe it's better to place this macro into pwm_module.h
 *  but I don't want to use pwm_module at all while using pwm_driver.
 */
#define PWM_INDIVIDUAL_SEQ_DEFAULT_CONFIG(seq_values)                  \
{                                                                      \
    .values.p_individual = &(seq_values),                              \
    .length              = (sizeof(seq_values) / sizeof(uint16_t)),    \
    .repeats             = 0,                                          \
    .end_delay           = 0                                           \
}
