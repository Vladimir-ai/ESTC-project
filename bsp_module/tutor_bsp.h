#ifndef _BSP_H
#define _BSP_H

#ifdef BOARD_PCA10059
#include "pca10059.h"
#endif /* BOARD_PCA10059 */

#ifndef LEDS_NUMBER
#define LEDS_NUMBER     0
#endif /* LEDS_NUMBER */

#ifndef LEDS_LIST
#define LEDS_LIST       {}
#endif /* LEDS_LIST */

#ifndef BUTTONS_NUMBER
#define BUTTONS_NUMBER  0
#endif /* BUTTONS_NUMBER */

#ifndef BUTTONS_LIST
#define BUTTONS_LIST    {}
#endif /* BUTTONS_LIST */

/* init functions */
void init_leds(void);
void init_led(uint8_t idx);
void init_btns(void);
void init_btn(uint8_t idx);

/* led operation functions */
void led_on(uint8_t idx);
void led_off(uint8_t idx);
void led_toggle(uint8_t idx);

/* btn operation functions */
#if (BUTTONS_NUMBER <= 1)
uint32_t read_btn_state(void);
#else
uint32_t read_btn_state(uint8_t idx);
#endif /* (BUTTONS_NUMBER <= 1) */

#endif /* _BSP_H */