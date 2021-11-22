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

#include "hsv_to_rgb.h"

/* Timer timeouts ==============================================*/
#define BTN_DISABLE_ACTIVITY_TIMEOUT (APP_TIMER_CLOCK_FREQ >> 4) /* part of sec */
#define BTN_DOUBLE_CLICK_TIMEOUT APP_TIMER_CLOCK_FREQ            /* 1 sec timeout */
#define BTN_LONG_CLICK_TIMEOUT (APP_TIMER_CLOCK_FREQ >> 2)       /* MUST be less than BTN_DOUBLE_CLICK_TIMEOUT */

/* Application flags ============================================*/
#define APP_FLAG_IS_RUNNING_MASK 0x01U
#define APP_FLAG_FST_CLICK_OCCURRED_MASK 0x02U
#define APP_FLAG_BTN_STATE_MASK 0x04U
#define APP_FLAG_BTN_DISABLED_MASK 0x08U

#define COLOR_CHANGE_STEP 1

/* static vars declaration ======================================= */
APP_TIMER_DEF(timer_id_double_click_timeout);
APP_TIMER_DEF(timer_id_en_btn_timeout);
static uint32_t timer_start_timestamp;

static nrf_pwm_values_individual_t rgb_sequence_values;
static nrf_pwm_values_individual_t pwm_indicator_sequence_values;
static uint16_t pwm_indicator_period = 0;

static uint8_t app_flags = 0;
static uint8_t current_mode = 0;
static color_params_t current_params = DEFAULT_COLOR_PARAMS;

static const uint16_t step_list[] =
    {
        0,
        16,
        64,
        PWM_INDICATOR_TOP_VALUE};

/* static function declaration  ====================================*/
static void logs_init(void);
static void init_all(const nrfx_gpiote_in_config_t *btn_gpiote_cfg);
static void init_rgb_pwm(nrfx_pwm_t const *const pwm_instance, nrfx_pwm_config_t *pwm_config);
static void init_indicator_pwm(nrfx_pwm_t const *const pwm_instance, nrfx_pwm_config_t *pwm_config);

/* interrupt handlers ============================================== */
static void timer_double_click_timeout_handler(void *p_context)
{
    app_flags &= ~APP_FLAG_FST_CLICK_OCCURRED_MASK;
}

static void timer_en_btn_timeout_handler(void *p_context)
{
    if (app_flags & APP_FLAG_BTN_STATE_MASK)
    {
        app_flags |= APP_FLAG_IS_RUNNING_MASK;
    }
    else
    {
        app_flags &= ~APP_FLAG_IS_RUNNING_MASK;
    }

    app_flags &= ~APP_FLAG_BTN_DISABLED_MASK;
}

static void btn_pressed_evt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    /* Track btn state: 0 is released, 1 is pressed now */
    app_flags ^= APP_FLAG_BTN_STATE_MASK;

    if (!(app_flags & APP_FLAG_BTN_DISABLED_MASK))
    {
        if (app_flags & APP_FLAG_BTN_STATE_MASK)
        {
            NRF_LOG_INFO("Btn pressed");

            if (!(app_flags & APP_FLAG_FST_CLICK_OCCURRED_MASK))
            {
                timer_start_timestamp = app_timer_cnt_get();
                app_flags |= APP_FLAG_FST_CLICK_OCCURRED_MASK;
                app_timer_start(timer_id_double_click_timeout, BTN_DOUBLE_CLICK_TIMEOUT, NULL);
            }
            else if (app_timer_cnt_diff_compute(app_timer_cnt_get(), timer_start_timestamp) < BTN_DOUBLE_CLICK_TIMEOUT)
            {
                app_flags &= ~APP_FLAG_FST_CLICK_OCCURRED_MASK;
                current_mode = (current_mode + 1) % MODES_COUNT;
                pwm_indicator_period = 0;
            }
        }
        else if (app_timer_cnt_diff_compute(app_timer_cnt_get(), timer_start_timestamp) > BTN_LONG_CLICK_TIMEOUT)
        {
            NRF_LOG_INFO("Btn released");
            app_flags &= ~APP_FLAG_FST_CLICK_OCCURRED_MASK;
        }

        /* Always disable any actions with btn if it was enabled */
        app_flags |= APP_FLAG_BTN_DISABLED_MASK;
        app_timer_start(timer_id_en_btn_timeout, BTN_DISABLE_ACTIVITY_TIMEOUT, NULL);
    }
}

static void rgb_pwm_handler(nrfx_pwm_evt_type_t event_type)
{
    if (event_type == NRFX_PWM_EVT_FINISHED)
    {
        if (app_flags & APP_FLAG_IS_RUNNING_MASK)
        {
            color_changing_machine(&current_params, COLOR_CHANGE_STEP, current_mode);

            rgb_sequence_values.channel_1 = current_params.red;
            rgb_sequence_values.channel_2 = current_params.green;
            rgb_sequence_values.channel_3 = current_params.blue;

            NRF_LOG_INFO("\nCurrent values:");
            NRF_LOG_INFO("r: %d, g: %d, b: %d", current_params.red, current_params.green, current_params.blue);
            NRF_LOG_INFO("h: %d, s: %d, v: %d", current_params.hue, current_params.saturation, current_params.brightness);
        }
    }
}

static void indicator_pwm_handler(nrfx_pwm_evt_type_t event_type)
{
    if (event_type == NRFX_PWM_EVT_FINISHED)
    {
        /* LED is always on */
        if (step_list[current_mode] >= PWM_INDICATOR_TOP_VALUE)
        {
            pwm_indicator_sequence_values.channel_0 = PWM_INDICATOR_TOP_VALUE;
        }
        else
        {
            /* handle overflow */
            if (pwm_indicator_period >= 2U * PWM_INDICATOR_TOP_VALUE)
            {
                pwm_indicator_period = 0;
            }

            pwm_indicator_sequence_values.channel_0 = pwm_indicator_period > PWM_INDICATOR_TOP_VALUE
                                                          ? 2U * PWM_INDICATOR_TOP_VALUE - pwm_indicator_period
                                                          : pwm_indicator_period;

            pwm_indicator_period += step_list[current_mode];
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
    /* conig structures ====================================== */
    nrfx_pwm_config_t pwm_rgb_config = NRFX_PWM_DEFAULT_CONFIG;
    nrfx_pwm_config_t pwm_indicator_config = NRFX_PWM_DEFAULT_CONFIG;

    const nrfx_pwm_t pwm_rgb_instance = NRFX_PWM_INSTANCE(0);
    const nrfx_pwm_t pwm_indicator_instance = NRFX_PWM_INSTANCE(1);

    const nrf_pwm_sequence_t pwm_rgb_sequence = PWM_INDIVIDUAL_SEQ_DEFAULT_CONFIG(rgb_sequence_values);
    const nrf_pwm_sequence_t pwm_indicator_sequence = PWM_INDIVIDUAL_SEQ_DEFAULT_CONFIG(pwm_indicator_sequence_values);

    const nrfx_gpiote_in_config_t btn_gpiote_cfg = {
        .sense = NRF_GPIOTE_POLARITY_TOGGLE,
        .pull = NRF_GPIO_PIN_PULLUP,
        .is_watcher = false,
        .hi_accuracy = false,
        .skip_gpio_setup = true};

    /* RGB pwm */
    init_rgb_pwm(&pwm_rgb_instance, &pwm_rgb_config);
    nrfx_pwm_simple_playback(&pwm_rgb_instance, &pwm_rgb_sequence,
                             PWM_RGB_CYCLES_FOR_ONE_STEP, NRFX_PWM_FLAG_LOOP);

    /* LEG pwm */
    init_indicator_pwm(&pwm_indicator_instance, &pwm_indicator_config);
    nrfx_pwm_simple_playback(&pwm_indicator_instance, &pwm_indicator_sequence,
                             PWM_INDICATOR_CYCLES_FOR_ONE_STEP, NRFX_PWM_FLAG_LOOP);

    init_all(&btn_gpiote_cfg);

    /* Toggle LEDs. */
    while (true)
    {
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

static void init_rgb_pwm(nrfx_pwm_t const *const pwm_instance, nrfx_pwm_config_t *pwm_config)
{
    pwm_config->output_pins[0] = NRFX_PWM_PIN_NOT_USED;
    APP_ERROR_CHECK(nrfx_pwm_init(pwm_instance, pwm_config, rgb_pwm_handler));
    NRF_LOG_INFO("PWM Initiated");

    rgb_sequence_values.channel_0 = 0;
    rgb_sequence_values.channel_1 = 0;
    rgb_sequence_values.channel_2 = 0;
    rgb_sequence_values.channel_3 = 0;

    NRF_LOG_FLUSH();
}

static void init_indicator_pwm(nrfx_pwm_t const *const pwm_instance, nrfx_pwm_config_t *pwm_config)
{
    uint8_t i = 0;
    pwm_config->output_pins[i++] = LED_1;
    pwm_config->output_pins[i++] = NRFX_PWM_PIN_NOT_USED;
    pwm_config->output_pins[i++] = NRFX_PWM_PIN_NOT_USED;
    pwm_config->output_pins[i++] = NRFX_PWM_PIN_NOT_USED;
    pwm_config->top_value = PWM_INDICATOR_TOP_VALUE;

    APP_ERROR_CHECK(nrfx_pwm_init(pwm_instance, pwm_config, indicator_pwm_handler));
    NRF_LOG_INFO("PWM Initiated");

    pwm_indicator_sequence_values.channel_0 = 0;
    pwm_indicator_sequence_values.channel_1 = 0;
    pwm_indicator_sequence_values.channel_2 = 0;
    pwm_indicator_sequence_values.channel_3 = 0;

    NRF_LOG_FLUSH();
}

static void init_all(const nrfx_gpiote_in_config_t *btn_gpiote_cfg)
{
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
    APP_ERROR_CHECK(nrfx_gpiote_in_init(BUTTON_1, btn_gpiote_cfg, &btn_pressed_evt_handler));
    NRF_LOG_INFO("GPIOTE initiated");

    nrfx_gpiote_in_event_enable(BUTTON_1, true);

    APP_ERROR_CHECK(app_timer_init());
    APP_ERROR_CHECK(app_timer_create(&timer_id_double_click_timeout, APP_TIMER_MODE_SINGLE_SHOT, &timer_double_click_timeout_handler));
    APP_ERROR_CHECK(app_timer_create(&timer_id_en_btn_timeout, APP_TIMER_MODE_SINGLE_SHOT, &timer_en_btn_timeout_handler));
    NRF_LOG_INFO("App timer initiated");

    NRF_LOG_FLUSH();
}
