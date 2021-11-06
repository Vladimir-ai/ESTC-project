#include <stdbool.h>
#include "nrf_delay.h"
#include "tutor_bsp.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "nrf_log_backend_usb.h"

#include "app_usbd.h"
#include "app_usbd_serial_num.h"


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

    /* Init logs */
    logs_init();
    NRF_LOG_INFO("Starting up the test project with USB logging");

    /* Init leds and btns */
    init_leds();
    init_btns();

    /* Toggle LEDs. */
    while (true)
    {
        for (uint8_t i = 0; i < LEDS_NUMBER; i++)
        {
            blink_num = 0;

            while(blink_num < inv_num[i])
            {
                if (!read_btn_state()){
                    led_toggle(i);
                    nrf_delay_ms(100);

                    led_toggle(i);
                    nrf_delay_ms(100);

                    blink_num++;

                    /* Process current LED and it's iter */
                    NRF_LOG_INFO("Btn pressed on %c LED, curr LED iter is %d/%d", led_color[i], blink_num, inv_num[i]);
                    NRF_LOG_PROCESS();  /* Process logs only when we have logs */
                }

                LOG_BACKEND_USB_PROCESS();  /* Process here to maintain connect */
                /* Don't spam PC with logs when btn isn't pressed */
            }
        }

        nrf_delay_ms(1000);
    }
}