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

#define DUTY_CICLE_UNIT_TIME_US         0x10U
#define GET_PERCENTAGE(num)             ((num) & 0x7FU)
#define GET_STATE(num)                  ((num) >> 7)

void logs_init()
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
    uint8_t blink_num = 0;
    uint8_t current_cycle = 0;      /* Uses 7th bit to store state (increasing or decreasing), 0-6 bits to store percentage */

    /* Init systick */
    nrfx_systick_init();

    /* Init logs */
    logs_init();
    NRF_LOG_INFO("Starting up the test project with USB logging");
    NRF_LOG_PROCESS();

    /* Init leds and btns */
    init_leds();
    init_btns();

    /* Toggle LEDs. */
    while (true)
    {
        for (uint8_t i = 0; i < LEDS_NUMBER; i++)
        {
            blink_num = 0;

            while (blink_num < inv_num[i])
            {
                current_cycle = 0;
                while (current_cycle != 0x7FU)   /* 0x7F, 101 are reserved value */
                {
                    if (!read_btn_state())
                    {
                        led_toggle(i);
                        nrfx_systick_delay_us(DUTY_CICLE_UNIT_TIME_US * GET_PERCENTAGE(current_cycle));

                        led_toggle(i);
                        nrfx_systick_delay_us(DUTY_CICLE_UNIT_TIME_US * GET_PERCENTAGE(0x100U - current_cycle));

                        /* Increase or decrease depending on state */
                        current_cycle = GET_STATE(current_cycle) ? (current_cycle - 1) : (current_cycle + 1);

                        /* Start decreasing if overflow */
                        if (current_cycle == 101)
                        {
                            current_cycle = 0xE4U;
                            NRF_LOG_INFO("100%% duty cycle on %c LED, curr LED iter is %d/%d", led_color[i], blink_num + 1, inv_num[i]);
                            NRF_LOG_PROCESS();
                        }
                    }

                    LOG_BACKEND_USB_PROCESS();  /* Process here to maintain connect */
                    /* Don't spam PC with logs when btn isn't pressed */
                }

                blink_num++;

                /* Process current LED and it's iter */
                NRF_LOG_INFO("Btn pressed on %c LED, curr LED iter is %d/%d", led_color[i], blink_num, inv_num[i]);
                NRF_LOG_PROCESS();  /* Process logs only when we have logs */
            }

        }
    }
}