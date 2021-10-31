#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"

#define MAX_BLINKS              7

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    const uint8_t inv_num[] = {6, 5, 7, 7};

    /* Configure board. */
    bsp_board_init(BSP_INIT_LEDS);

    /* Toggle LEDs. */
    while (true)
    {
        for (uint8_t i = 0; i < LEDS_NUMBER; i++)
        {
            for (uint8_t blink_num = 0; blink_num < MAX_BLINKS; blink_num++)
            {
                if (blink_num < inv_num[i]){
                    bsp_board_led_on(i);
                    nrf_delay_ms(100);

                    bsp_board_led_off(i);
                    nrf_delay_ms(100);
                }
            }
        }

        nrf_delay_ms(1000);
    }
}