#include "g_context.h"

#define BTN_DISABLE_ACTIVITY_TIMEOUT_TICKS      (APP_TIMER_CLOCK_FREQ / 14)             /* RTC timer ticks */
#define BTN_DOUBLE_CLICK_TIMEOUT_TICKS          (APP_TIMER_CLOCK_FREQ)                  /* 1 sec timeout */
#define BTN_LONG_CLICK_TIMEOUT_TICKS            (APP_TIMER_CLOCK_FREQ >> 1)             /* MUST be less than BTN_DOUBLE_CLICK_TIMEOUT_TICKS */
STATIC_ASSERT(BTN_LONG_CLICK_TIMEOUT_TICKS < BTN_DOUBLE_CLICK_TIMEOUT_TICKS);

APP_TIMER_DEF(timer_id_double_click_timeout);                                           /**< Double click timer */
APP_TIMER_DEF(timer_id_en_btn_timeout);                                                 /**< Button pressed timer (need it to remove fluctuations) */


static void btn_pressed_evt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action);


static void timer_double_click_timeout_handler(void *p_context)
{
  g_app_data.flags.fst_click_occurred = false;
}

static void timer_en_btn_timeout_handler(void *p_context)
{
  /* btn value at the end of disable timeout */
  if (g_app_data.flags.btn_pressed)
  {
    g_app_data.flags.app_is_running = true;
  }
  else
  {
    g_app_data.flags.app_is_running = false;
  }

  g_app_data.flags.btn_is_disabled = false;
}


static void btn_pressed_evt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  static uint32_t timer_start_timestamp = 0;

  /* Track btn state: false is released, true is pressed now */
  g_app_data.flags.btn_pressed ^= true;

  if (!(g_app_data.flags.btn_is_disabled))
  {
    if (g_app_data.flags.btn_pressed)
    {
      NRF_LOG_INFO("Btn pressed");

      if (!(g_app_data.flags.fst_click_occurred))
      {
        timer_start_timestamp = app_timer_cnt_get();
        g_app_data.flags.fst_click_occurred = true;
        app_timer_start(timer_id_double_click_timeout, BTN_DOUBLE_CLICK_TIMEOUT_TICKS, NULL);
      }
      else if (app_timer_cnt_diff_compute(app_timer_cnt_get(), timer_start_timestamp) < BTN_DOUBLE_CLICK_TIMEOUT_TICKS)
      {
        g_app_data.flags.fst_click_occurred = false;
        g_app_data.current_led_mode = (g_app_data.current_led_mode + 1) % MODES_COUNT;
        reset_indicator_led();

        if (!g_app_data.current_led_mode)
        {
          nvmc_write_new_record(g_app_data.current_hsv);
        }
      }
    }
    else
    {
      NRF_LOG_INFO("Btn released");

      /* It isn't double click */
      if (app_timer_cnt_diff_compute(app_timer_cnt_get(), timer_start_timestamp) < BTN_DOUBLE_CLICK_TIMEOUT_TICKS)
      {
        g_app_data.flags.fst_click_occurred = false;
      }
    }

    /* Always disable any actions with btn if it was enabled */
    g_app_data.flags.btn_is_disabled = true;
    app_timer_start(timer_id_en_btn_timeout, BTN_DISABLE_ACTIVITY_TIMEOUT_TICKS, NULL);
  }
}


void button_timers_init(void)
{
  APP_ERROR_CHECK(app_timer_create(&timer_id_double_click_timeout, APP_TIMER_MODE_SINGLE_SHOT, &timer_double_click_timeout_handler));
  APP_ERROR_CHECK(app_timer_create(&timer_id_en_btn_timeout, APP_TIMER_MODE_SINGLE_SHOT, &timer_en_btn_timeout_handler));
}