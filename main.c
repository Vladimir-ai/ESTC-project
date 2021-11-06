#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "tutor_bsp.h"

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    const uint8_t inv_num[] = {6, 5, 7, 7};
    uint8_t blink_num = 0;

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
                }
            }
        }

        nrf_delay_ms(1000);
    }
}