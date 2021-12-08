#ifndef _G_CONTEXT_H
#define _G_CONTEXT_H

#include "nrfx_pwm.h"
#include "nrfx_gpiote.h"
#include "app_timer.h"
#include "hsv_to_rgb.h"
#include "pwm_config.h"

#define GET_RGB_PWM_CONF(g_context)                 ((g_context).app_config.pwm_rgb_config)
#define GET_INDICATOR_PWM_CONF(g_context)           ((g_context).app_config.pwm_indicator_config)

typedef struct g_pwm_config_s
{
  nrfx_pwm_config_t config;
  nrfx_pwm_t  instance;
  nrf_pwm_sequence_t sequence;
  nrf_pwm_values_individual_t sequence_values;
} g_pwm_config_t;

typedef struct g_app_flags_s /* contains only flags */
{
  bool app_is_running;        /* true if application should change value depending on mode, else false */
  bool fst_click_occurred;    /* true if first click was occurred */
  bool btn_pressed;           /* false if released, true if pressed now */
  bool btn_is_disabled;       /* true if btn won't do anything except @ref btn_state changing, else false */
} g_app_flags_t;

typedef struct g_app_data_s
{
  g_app_flags_t flags;
  hsv_params_t current_hsv;
  uint8_t current_led_mode; /* current LED mode */
} g_app_data_t;

extern g_app_data_t app_data;

static const uint16_t step_list[] =
{
    0,
    16,
    64,
    PWM_INDICATOR_TOP_VALUE
};


#endif /* _G_CONTEXT_H */