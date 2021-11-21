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

#include "nrfx_gpiote.h"
#include "app_timer.h"

#include "pwm_config.h"
#include "nrfx_pwm.h"

/* Timer timeouts */
#define BTN_DISABLE_ACTIVITY_TIMEOUT          (APP_TIMER_CLOCK_FREQ >> 4) /* part of sec */
#define BTN_DOUBLE_CLICK_TIMEOUT              APP_TIMER_CLOCK_FREQ        /* 1 sec timeout */

/* Application flags */
#define APP_FLAG_IS_RUNNING_MASK                 0x01U
#define APP_FLAG_FST_CLICK_OCCURRED_MASK         0x02U

/* static vars declaration */
static nrf_pwm_values_individual_t sequence_values;

static uint32_t timer_start_timestamp;
static uint8_t app_flags = 0;
APP_TIMER_DEF(timeout_timer_id);

/* static function declaration  */
static void init_all(const nrfx_gpiote_in_config_t *btn_gpiote_cfg);
static void init_pwm(nrfx_pwm_t const * const pwm_instance, const nrfx_pwm_config_t *pwm_config);

static void timer_timeout_handler(void *p_context)
{
    /* Clear fst click if it isn't occured */
    app_flags &= ~APP_FLAG_FST_CLICK_OCCURRED_MASK;
}

static void btn_pressed_evt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    switch (action)
    {
    case NRF_GPIOTE_POLARITY_HITOLO:
        if (!(app_flags & APP_FLAG_FST_CLICK_OCCURRED_MASK))
        {
            app_flags |= APP_FLAG_FST_CLICK_OCCURRED_MASK;
            timer_start_timestamp = app_timer_cnt_get();

            app_timer_start(timeout_timer_id, BTN_DOUBLE_CLICK_TIMEOUT, NULL);
        }
        /* timeout used to disable undefined button behavior after press */
        else if ((app_timer_cnt_get() - timer_start_timestamp) > BTN_DISABLE_ACTIVITY_TIMEOUT)
        {
            app_flags ^= APP_FLAG_IS_RUNNING_MASK;
            app_flags &= ~APP_FLAG_FST_CLICK_OCCURRED_MASK;
        }
        break;

    default:
        break;
    }
}

static void pwm_handler(nrfx_pwm_evt_type_t event_type)
{
    if (event_type == NRFX_PWM_EVT_FINISHED)
    {
        static uint16_t current_period = 0;
        static uint8_t blink_num = 0;
        static uint8_t led_idx = 0;

        /* I think that we don't need these definitions anywhere else */
        const uint8_t inv_num[] = {6, 5, 7, 7};
        const char led_color[] = {'G', 'R', 'G', 'B'};

        if (app_flags & APP_FLAG_IS_RUNNING_MASK)
        {
            if (current_period == PWM_TOP_VALUE)
            {
                NRF_LOG_INFO("100%% duty cycle on %c LED, curr LED iter is %d/%d", led_color[led_idx], blink_num + 1, inv_num[led_idx]);
            }
            else if (current_period == PWM_TOP_VALUE * 2)
            {
                NRF_LOG_INFO("Btn cycle ended on %c LED, curr LED iter is %d/%d", led_color[led_idx], blink_num + 1, inv_num[led_idx]);
                current_period = 0;

                blink_num++;

                /* blink num has changed */
                if (blink_num == inv_num[led_idx])
                {
                    blink_num = 0;
                    ((uint16_t *)(&sequence_values))[led_idx] = 0;
                    led_idx = (led_idx + 1) % LEDS_NUMBER;
                }
            }

            current_period++;
            ((uint16_t *)(&sequence_values))[led_idx] = current_period > PWM_TOP_VALUE
                            ? 2U * PWM_TOP_VALUE - current_period  : current_period;
        }
    }
}

static void logs_init()
{
    ret_code_t ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    const nrfx_pwm_config_t pwm_config = NRFX_PWM_DEFAULT_CONFIG;
    const nrfx_pwm_t pwm_instance = NRFX_PWM_INSTANCE(0);
    const nrf_pwm_sequence_t pwm_sequence = PWM_INDIVIDUAL_SEQ_DEFAULT_CONFIG(sequence_values);


    const nrfx_gpiote_in_config_t btn_gpiote_cfg = {
        .sense = NRF_GPIOTE_POLARITY_HITOLO,
        .pull = NRF_GPIO_PIN_PULLUP,
        .is_watcher = false,
        .hi_accuracy = false,
        .skip_gpio_setup = true
    };

    init_all(&btn_gpiote_cfg);
    init_pwm(&pwm_instance, &pwm_config);

    nrfx_pwm_simple_playback(&pwm_instance, &pwm_sequence, 1, NRFX_PWM_FLAG_LOOP);

    /* Toggle LEDs. */
    while (true)
    {
        __WFE();

        __SEV();
        __WFE();

        NRF_LOG_FLUSH();
        LOG_BACKEND_USB_PROCESS();  /* Process here to maintain connect */
        /* Don't spam PC with logs when btn isn't pressed */
    }
}

static void init_all(const nrfx_gpiote_in_config_t *btn_gpiote_cfg)
{
    /* Init systick */
    nrfx_systick_init();

    /* Init logs */
    logs_init();
    NRF_LOG_INFO("Starting up the test project with USB logging");
    NRF_LOG_FLUSH();

    /* Init leds and btns */
    init_leds();
    init_btns();

    /* Init gpiote */
    APP_ERROR_CHECK(nrfx_gpiote_init());
    APP_ERROR_CHECK(nrfx_gpiote_in_init(BUTTON_1, btn_gpiote_cfg, &btn_pressed_evt_handler));
    NRF_LOG_INFO("GPIOTE initiated");
    NRF_LOG_FLUSH();

    nrfx_gpiote_in_event_enable(BUTTON_1, true);

    APP_ERROR_CHECK(app_timer_init());
    APP_ERROR_CHECK(app_timer_create(&timeout_timer_id, APP_TIMER_MODE_SINGLE_SHOT, &timer_timeout_handler));
    NRF_LOG_INFO("App timer initiated");

    NRF_LOG_FLUSH();
}

static void init_pwm(nrfx_pwm_t const * const pwm_instance, const nrfx_pwm_config_t *pwm_config)
{
    APP_ERROR_CHECK(nrfx_pwm_init(pwm_instance, pwm_config, pwm_handler));
    NRF_LOG_INFO("PWM Initiated");

    sequence_values.channel_0 = 0;
    sequence_values.channel_1 = 0;
    sequence_values.channel_2 = 0;
    sequence_values.channel_3 = 0;

    NRF_LOG_FLUSH();
}
