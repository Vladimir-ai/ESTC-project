#include <stdbool.h>
#include "nrf_drv_systick.h"
#include "nrf_delay.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_log_backend_usb.h"
#include "nrfx_nvmc.h"
#include "app_usbd.h"
#include "app_usbd_serial_num.h"

#include "g_context.h"
#include "tutor_bsp.h"
#include "pwm_module.h"
#include "hsv_to_rgb.h"
#include "nvmc_module.h"
#include "usbd_module.h"
#include "cli_usb.h"


/* Timer timeouts ==============================================*/
#define BTN_DISABLE_ACTIVITY_TIMEOUT_TICKS          (APP_TIMER_CLOCK_FREQ / 14)     /* RTC timer ticks */
#define BTN_DOUBLE_CLICK_TIMEOUT_TICKS              APP_TIMER_CLOCK_FREQ            /* 1 sec timeout */
#define BTN_LONG_CLICK_TIMEOUT_TICKS                (APP_TIMER_CLOCK_FREQ >> 1)     /* MUST be less than BTN_DOUBLE_CLICK_TIMEOUT_TICKS */
STATIC_ASSERT(BTN_LONG_CLICK_TIMEOUT_TICKS < BTN_DOUBLE_CLICK_TIMEOUT_TICKS);

/* static vars declaration ======================================= */
/* timer config */
APP_TIMER_DEF(timer_id_double_click_timeout);
APP_TIMER_DEF(timer_id_en_btn_timeout);
/* btn config */
static nrfx_gpiote_in_config_t gpiote_btn_config;
/* app data */
g_app_data_t g_app_data;

/* static function declaration  ====================================*/
static void logs_init(void);
static void init_all();

/* interrupt handlers ============================================== */
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

/**
 * @brief Function for application main entry.
 *
 * Note: I really don't want to store all configurations in global scope.
 *  They are stored globally only if needed.
 */
int main(void)
{
  g_app_data.current_hsv = nvmc_find_last_record();

  init_pwm();
  init_all();
  init_cli(&update_leds, &nvmc_write_new_record);

  while (true)
  {
    nvmc_erase_last_written_page();

    __WFE();

    NRF_LOG_FLUSH();
    LOG_BACKEND_USB_PROCESS(); /* Process here to maintain connect */
                               /* Don't spam PC with logs when btn isn't pressed */
  }
}

/* Init functions =============================================== */
static void logs_init()
{
  ret_code_t ret = NRF_LOG_INIT(NULL);
  APP_ERROR_CHECK(ret);

  NRF_LOG_DEFAULT_BACKENDS_INIT();
}

static void init_all()
{
  gpiote_btn_config = (nrfx_gpiote_in_config_t)
  {
      .sense = NRF_GPIOTE_POLARITY_TOGGLE,
      .pull = NRF_GPIO_PIN_PULLUP,
      .is_watcher = false,
      .hi_accuracy = false,
      .skip_gpio_setup = true
  };

  /* Init systick */
  nrfx_systick_init();

  /* Init logs */
  logs_init();
  NRF_LOG_INFO("Starting up the test project with USB logging");

  /* Init leds and btns */
  init_leds();
  init_btns();

  /* Init gpiote */
  APP_ERROR_CHECK(nrfx_gpiote_init());
  APP_ERROR_CHECK(nrfx_gpiote_in_init(BUTTON_1, &gpiote_btn_config,
         &btn_pressed_evt_handler));
  NRF_LOG_INFO("GPIOTE initiated");

  nrfx_gpiote_in_event_enable(BUTTON_1, true);

  APP_ERROR_CHECK(app_timer_init());
  APP_ERROR_CHECK(app_timer_create(&timer_id_double_click_timeout, APP_TIMER_MODE_SINGLE_SHOT, &timer_double_click_timeout_handler));
  APP_ERROR_CHECK(app_timer_create(&timer_id_en_btn_timeout, APP_TIMER_MODE_SINGLE_SHOT, &timer_en_btn_timeout_handler));
  NRF_LOG_INFO("App timer initiated");

  init_usbd();
  NRF_LOG_INFO("USBD initiated");

  NRF_LOG_FLUSH();
}
