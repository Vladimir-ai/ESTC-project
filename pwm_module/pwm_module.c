#include "pwm_module.h"
#include "tutor_bsp.h"
#include "nrf_assert.h"
#include "nrf_drv_systick.h"

void pwm_process_one_period(uint8_t led_idx, uint8_t duty_cycle)
{
  ASSERT(led_idx < LEDS_NUMBER);
  ASSERT(duty_cycle <= 100U);

  led_toggle(led_idx);
  nrfx_systick_delay_us(PWM_PERCENT_TIME_US * duty_cycle);

  led_toggle(led_idx);
  nrfx_systick_delay_us(PWM_PERCENT_TIME_US * (100 - duty_cycle));
}
