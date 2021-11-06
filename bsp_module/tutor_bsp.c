#include "tutor_bsp.h"
#include "nrf_gpio.h"

static const uint32_t leds_array[] = LEDS_LIST;
static const uint32_t btns_array[] = BUTTONS_LIST;

void init_leds(void)
{
    uint8_t idx;

    for(idx = 0; idx < LEDS_NUMBER; idx++)
    {
        init_led(idx);
    }
}

void init_led(uint8_t idx)
{
    ASSERT(idx < LEDS_NUMBER);
    nrf_gpio_cfg_output(leds_array[idx]);
    nrf_gpio_pin_write(leds_array[idx], 1);
}

void init_btns(void)
{
    uint8_t idx;

    for(idx = 0; idx < BUTTONS_NUMBER; idx++)
    {
        init_btn(idx);
    }
}

void init_btn(uint8_t idx)
{
    ASSERT(idx < BUTTONS_NUMBER);
    nrf_gpio_cfg_input(btns_array[idx], GPIO_PIN_CNF_PULL_Pullup);
}

void led_on(uint8_t idx)
{
    ASSERT(idx < LEDS_NUMBER);
    nrf_gpio_pin_write(leds_array[idx], 0);
}

void led_off(uint8_t idx)
{
    ASSERT(idx < LEDS_NUMBER);
    nrf_gpio_pin_write(leds_array[idx], 1);
}

void led_toggle(uint8_t idx)
{
    ASSERT(idx < LEDS_NUMBER);
    nrf_gpio_pin_toggle(leds_array[idx]);
}


#if (BUTTONS_NUMBER == 0)
uint32_t read_btn_state(void)
{
    return 0;
}
#elif (BUTTONS_NUMBER == 1)
uint32_t read_btn_state()
{
    return nrf_gpio_pin_read(btns_array[0]);
}
#else
void read_btn_state(uint8_t idx)
{
    ASSERT(idx < BUTTONS_NUMBER);
    return nrf_gpio_pin_read(btns_array[idx]);
}
#endif /* (BUTTONS_NUMBER == 0) */

