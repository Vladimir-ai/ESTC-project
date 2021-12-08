#include "pwm_module.h"
#include "tutor_bsp.h"
#include "nrf_assert.h"
#include "nrf_drv_systick.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "hsv_to_rgb.h"
#include "pwm_config.h"
#include "g_context.h"

/*pwm config */
static g_pwm_config_t pwm_rgb_config;
static g_pwm_config_t pwm_indicator_config;
static uint16_t pwm_indicator_period = 0;


void pwm_process_one_period(uint8_t led_idx, uint8_t duty_cycle)
{
  ASSERT(led_idx < LEDS_NUMBER);
  ASSERT(duty_cycle <= 100U);

  led_toggle(led_idx);
  nrfx_systick_delay_us(PWM_PERCENT_TIME_US * duty_cycle);

  led_toggle(led_idx);
  nrfx_systick_delay_us(PWM_PERCENT_TIME_US * (100 - duty_cycle));
}


static void rgb_pwm_handler(nrfx_pwm_evt_type_t event_type)
{
  rgb_params_t rgb;

  if (event_type == NRFX_PWM_EVT_FINISHED)
  {
    if (app_data.flags.app_is_running)
    {
      rgb = color_changing_machine(&app_data.current_hsv, COLOR_CHANGE_STEP, app_data.current_led_mode);

      pwm_rgb_config.sequence_values.channel_1 = rgb.red;
      pwm_rgb_config.sequence_values.channel_2 = rgb.green;
      pwm_rgb_config.sequence_values.channel_3 = rgb.blue;

      NRF_LOG_INFO("Current values:");
      NRF_LOG_INFO("h: %d, s: %d, v: %d", app_data.current_hsv.hue,
                                      app_data.current_hsv.saturation,
                                      app_data.current_hsv.brightness);
    }
  }
}

static void indicator_pwm_handler(nrfx_pwm_evt_type_t event_type)
{
  if (event_type == NRFX_PWM_EVT_FINISHED)
  {
    /* LED is always on */
    if (step_list[app_data.current_led_mode] >= PWM_INDICATOR_TOP_VALUE)
    {
      pwm_indicator_config.sequence_values.channel_0 = PWM_INDICATOR_TOP_VALUE;
    }
    else
    {
      /* handle overflow */
      if (pwm_indicator_period >= 2U * PWM_INDICATOR_TOP_VALUE)
      {
        pwm_indicator_period = 0;
      }

      pwm_indicator_config.sequence_values.channel_0 = pwm_indicator_period > PWM_INDICATOR_TOP_VALUE
                                                    ? 2U * PWM_INDICATOR_TOP_VALUE - pwm_indicator_period
                                                    : pwm_indicator_period;

      pwm_indicator_period += step_list[app_data.current_led_mode];
    }
  }
}

void reset_indicator_led(void)
{
  pwm_indicator_period = 0;
}

void init_pwm(void)
{
  /* Indicator pwm init start */
  uint8_t i = 0;
  rgb_params_t rgb;

  pwm_indicator_config.config = (nrfx_pwm_config_t)NRFX_PWM_DEFAULT_CONFIG;
  pwm_indicator_config.instance = (nrfx_pwm_t)NRFX_PWM_INSTANCE(1);
  pwm_indicator_config.sequence = (nrf_pwm_sequence_t)PWM_INDIVIDUAL_SEQ_DEFAULT_CONFIG(
                                  pwm_indicator_config.sequence_values);

  pwm_indicator_config.config.output_pins[i++] = LED_1;
  pwm_indicator_config.config.output_pins[i++] = NRFX_PWM_PIN_NOT_USED;
  pwm_indicator_config.config.output_pins[i++] = NRFX_PWM_PIN_NOT_USED;
  pwm_indicator_config.config.output_pins[i++] = NRFX_PWM_PIN_NOT_USED;
  pwm_indicator_config.config.top_value = PWM_INDICATOR_TOP_VALUE;

  pwm_indicator_config.sequence_values.channel_0 = 0;
  pwm_indicator_config.sequence_values.channel_1 = 0;
  pwm_indicator_config.sequence_values.channel_2 = 0;
  pwm_indicator_config.sequence_values.channel_3 = 0;

  APP_ERROR_CHECK(nrfx_pwm_init(&pwm_indicator_config.instance,
          &pwm_indicator_config.config, indicator_pwm_handler));
  NRF_LOG_INFO("Indicator PWM Initiated");
  /* Indicator pwm init end */

  /* RGB pwm init start */
  pwm_rgb_config.config = (nrfx_pwm_config_t)NRFX_PWM_DEFAULT_CONFIG;
  pwm_rgb_config.instance = (nrfx_pwm_t)NRFX_PWM_INSTANCE(0);
  pwm_rgb_config.sequence = (nrf_pwm_sequence_t)PWM_INDIVIDUAL_SEQ_DEFAULT_CONFIG(
        pwm_rgb_config.sequence_values);

  pwm_rgb_config.config.output_pins[0] = NRFX_PWM_PIN_NOT_USED;

  memcpy(&pwm_rgb_config.sequence_values,
  &pwm_indicator_config.sequence_values,
                sizeof(nrf_pwm_values_individual_t));

  rgb = color_changing_machine(&app_data.current_hsv, 0, 0);

  pwm_rgb_config.sequence_values.channel_1 = rgb.red;
  pwm_rgb_config.sequence_values.channel_2 = rgb.green;
  pwm_rgb_config.sequence_values.channel_3 = rgb.blue;


  APP_ERROR_CHECK(nrfx_pwm_init(&pwm_rgb_config.instance,
                &pwm_rgb_config.config, rgb_pwm_handler));


  nrfx_pwm_simple_playback(&pwm_rgb_config.instance,
                            &pwm_rgb_config.sequence,
                            PWM_RGB_CYCLES_FOR_ONE_STEP, NRFX_PWM_FLAG_LOOP);

  nrfx_pwm_simple_playback(&pwm_indicator_config.instance,
                            &pwm_indicator_config.sequence,
                            PWM_INDICATOR_CYCLES_FOR_ONE_STEP, NRFX_PWM_FLAG_LOOP);

  NRF_LOG_INFO("LED PWM Initiated");
  /* RGB pwm init end */

  NRF_LOG_FLUSH();
}