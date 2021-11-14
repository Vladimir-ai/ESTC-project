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

#define DUTY_CICLE_UNIT_TIME_US         10U
#define FULL_PERIOD                     200U

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
    uint8_t current_cycle = 0;      /* Uses 7th bit to store state (increasing or decreasing), 0-6 bits to store percentage */

    init_all();

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

            pwm_process_one_period(led_idx, current_cycle > 100 ? 200U - current_cycle : current_cycle);
            current_cycle++;

            switch (current_cycle)
            {
            case 101:
                NRF_LOG_INFO("100%% duty cycle on %c LED, curr LED iter is %d/%d", led_color[led_idx], blink_num + 1, inv_num[led_idx]);
                NRF_LOG_PROCESS();
                break;

            case 201:
                NRF_LOG_INFO("Btn cycle ended on %c LED, curr LED iter is %d/%d", led_color[led_idx], blink_num + 1, inv_num[led_idx]);
                NRF_LOG_PROCESS();
                current_cycle = 0;
                blink_num++;
                break;

            default:
                break;
            }
        }

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