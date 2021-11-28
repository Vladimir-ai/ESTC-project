#include <stdbool.h>
#include "nrf_drv_systick.h"
#include "nrf_delay.h"
#include "tutor_bsp.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_log_backend_usb.h"

#include "app_usbd.h"
#include "app_usbd_serial_num.h"

#include "pwm_config.h"

#include "g_context.h"
#include "hsv_to_rgb.h"
#include "nvmc_module.h"
#include "nrfx_nvmc.h"

/* Timer timeouts ==============================================*/
#define BTN_DISABLE_ACTIVITY_TIMEOUT_TICKS          (APP_TIMER_CLOCK_FREQ / 12)     /* RTC timer ticks */
#define BTN_DOUBLE_CLICK_TIMEOUT_TICKS              APP_TIMER_CLOCK_FREQ            /* 1 sec timeout */
#define BTN_LONG_CLICK_TIMEOUT_TICKS                (APP_TIMER_CLOCK_FREQ >> 1)     /* MUST be less than BTN_DOUBLE_CLICK_TIMEOUT_TICKS */
STATIC_ASSERT(BTN_LONG_CLICK_TIMEOUT_TICKS < BTN_DOUBLE_CLICK_TIMEOUT_TICKS);

/* Application flags ============================================*/
#define COLOR_CHANGE_STEP 1

/* static vars declaration ======================================= */
/* timer config */
APP_TIMER_DEF(timer_id_double_click_timeout);
APP_TIMER_DEF(timer_id_en_btn_timeout);
/*pwm config */
static g_pwm_config_t pwm_rgb_config;
static g_pwm_config_t pwm_indicator_config;
/* btn config */
static nrfx_gpiote_in_config_t gpiote_btn_config;
/* app data */
static g_app_data_t app_data;
static uint16_t pwm_indicator_period = 0;

static const uint16_t step_list[] =
{
    0,
    16,
    64,
    PWM_INDICATOR_TOP_VALUE
};

/* static function declaration  ====================================*/
static void logs_init(void);
static void init_all();
static void init_pwm();

/* interrupt handlers ============================================== */
static void timer_double_click_timeout_handler(void *p_context)
{
  app_data.flags.fst_click_occurred = 0;
}

static void timer_en_btn_timeout_handler(void *p_context)
{
  /* btn value at the end of disable timeout */
  if (app_data.flags.btn_state)
  {
    app_data.flags.app_is_running = 1;
  }
  else
  {
    app_data.flags.app_is_running = 0;
  }

  app_data.flags.btn_is_disabled = 0;
}

static void btn_pressed_evt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  static uint32_t timer_start_timestamp = 0;

  /* Track btn state: 0 is released, 1 is pressed now */
  app_data.flags.btn_state ^= 1;

  if (!(app_data.flags.btn_is_disabled))
  {
    if (app_data.flags.btn_state)
    {
      NRF_LOG_INFO("Btn pressed");

      if (!(app_data.flags.fst_click_occurred))
      {
        timer_start_timestamp = app_timer_cnt_get();
        app_data.flags.fst_click_occurred = 1;
        app_timer_start(timer_id_double_click_timeout, BTN_DOUBLE_CLICK_TIMEOUT_TICKS, NULL);
      }
      else if (app_timer_cnt_diff_compute(app_timer_cnt_get(), timer_start_timestamp) < BTN_DOUBLE_CLICK_TIMEOUT_TICKS)
      {
        app_data.flags.fst_click_occurred = 0;
        app_data.current_led_mode = (app_data.current_led_mode + 1) % MODES_COUNT;
        pwm_indicator_period = 0;

        if (!app_data.current_led_mode)
        {
          nvmc_write_new_record(app_data.current_hsv);
        }
      }
    }
    else
    {
      NRF_LOG_INFO("Btn released");

      /* It isn't double click */
      if (app_timer_cnt_diff_compute(app_timer_cnt_get(), timer_start_timestamp) < BTN_DOUBLE_CLICK_TIMEOUT_TICKS)
      {
        app_data.flags.fst_click_occurred = 0;
      }
    }

    /* Always disable any actions with btn if it was enabled */
    app_data.flags.btn_is_disabled = 1;
    app_timer_start(timer_id_en_btn_timeout, BTN_DISABLE_ACTIVITY_TIMEOUT_TICKS, NULL);
  }
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

/**
 * @brief Function for application main entry.
 *
 * Note: I really don't want to store all configurations in global scope.
 *  They are stored globally only if needed.
 */
int main(void)
{
  app_data.current_hsv = nvmc_find_last_record();

  init_pwm();
  init_all();

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

static void init_pwm()
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
  NRF_LOG_INFO("LED PWM Initiated");
  /* RGB pwm init end */

  NRF_LOG_FLUSH();
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

  NRF_LOG_FLUSH();

  nrfx_pwm_simple_playback(&pwm_rgb_config.instance,
                            &pwm_rgb_config.sequence,
                            PWM_RGB_CYCLES_FOR_ONE_STEP, NRFX_PWM_FLAG_LOOP);

  nrfx_pwm_simple_playback(&pwm_indicator_config.instance,
                            &pwm_indicator_config.sequence,
                            PWM_INDICATOR_CYCLES_FOR_ONE_STEP, NRFX_PWM_FLAG_LOOP);
}
