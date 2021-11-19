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

#include "pwm_module.h"

#define DUTY_CICLE_UNIT_TIME_US                     10U
#define DUTY_CICLE_TIME_US                          (100U * DUTY_CICLE_UNIT_TIME_US)
#define FULL_PERIOD_IN_DUTY_PERCENT                 200U    /* 256 - MAX */
#define HALF_PERIOD_IN_DUTY_PERCENT                 (FULL_PERIOD_IN_DUTY_PERCENT / 2)

#define GET_PERCENT(num, max_value)                 ((num) * 100 / (max_value))

/* static function declaration  */
static void init_all(void);

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
    const uint8_t inv_num[] = {6, 5, 7, 7};
    const char led_color[] = {'G', 'R', 'G', 'B'};

    uint8_t led_idx = 0;
    uint8_t blink_num = 0;
    uint8_t current_absolute_period = 0;      /* Absolute cycle */
    uint8_t current_cycle_percent = 0;
    nrfx_systick_state_t systick_state;

    nrfx_systick_init();
    init_all();

    nrfx_systick_get(&systick_state);

    /* Toggle LEDs. */
    while (true)
    {
        if (!read_btn_state())
        {
            if (blink_num == inv_num[led_idx])
            {
                blink_num = 0;
                led_idx = (led_idx + 1) % LEDS_NUMBER;

                continue;
            }

            current_cycle_percent = current_absolute_period > HALF_PERIOD_IN_DUTY_PERCENT
                    ? GET_PERCENT(FULL_PERIOD_IN_DUTY_PERCENT - current_absolute_period, HALF_PERIOD_IN_DUTY_PERCENT)
                    : GET_PERCENT(current_absolute_period, HALF_PERIOD_IN_DUTY_PERCENT);

            /* Check active phase */
            if (!nrfx_systick_test(&systick_state, current_cycle_percent * DUTY_CICLE_UNIT_TIME_US))
            {
                led_on(led_idx);
            }
            /* Check not active phase */
            else if (!nrfx_systick_test(&systick_state, 100U * DUTY_CICLE_UNIT_TIME_US))
            {
                led_off(led_idx);
            }
            /* Period finished */
            else
            {
                if (current_absolute_period == HALF_PERIOD_IN_DUTY_PERCENT + 1)
                {
                    NRF_LOG_INFO("100%% duty cycle on %c LED, curr LED iter is %d/%d", led_color[led_idx], blink_num + 1, inv_num[led_idx]);
                }
                else if (current_absolute_period == FULL_PERIOD_IN_DUTY_PERCENT + 1)
                {
                    NRF_LOG_INFO("Btn cycle ended on %c LED, curr LED iter is %d/%d", led_color[led_idx], blink_num + 1, inv_num[led_idx]);
                    current_absolute_period = 0;
                    blink_num++;
                }

                current_absolute_period++;
                led_off(led_idx);
                nrfx_systick_get(&systick_state);
            }
        }

        NRF_LOG_FLUSH();
        LOG_BACKEND_USB_PROCESS();  /* Process here to maintain connect */
        /* Don't spam PC with logs when btn isn't pressed */
    }
}

static void init_all(void)
{
    /* Init systick */
    nrfx_systick_init();

    /* Init logs */
    logs_init();
    NRF_LOG_INFO("Starting up the test project with USB logging");
    NRF_LOG_PROCESS();

    /* Init leds and btns */
    init_leds();
    init_btns();
}