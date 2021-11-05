#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"

#define MAX_BLINKS              7

__STATIC_INLINE void init_leds(void);

__STATIC_INLINE void init_leds(void)
{
    const uint32_t leds_array[] = LEDS_LIST;
    uint8_t idx = 0;

    for(idx = 0; idx < LEDS_NUMBER; idx++)
    {
        nrf_gpio_cfg_output(leds_array[idx]);
        nrf_gpio_pin_write(leds_array[idx], 1);
    }
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    const uint8_t inv_num[] = {6, 5, 7, 7};
    const uint32_t leds_array[] = LEDS_LIST;

    init_leds();

    /* Toggle LEDs. */
    while (true)
    {
        for (uint8_t i = 0; i < LEDS_NUMBER; i++)
        {
            for (uint8_t blink_num = 0; blink_num < MAX_BLINKS; blink_num++)
            {
                if (blink_num < inv_num[i]){
                    nrf_gpio_pin_toggle(leds_array[i]);
                    nrf_delay_ms(100);

                    nrf_gpio_pin_toggle(leds_array[i]);
                    nrf_delay_ms(100);
                }
            }
        }

        nrf_delay_ms(1000);
    }
}