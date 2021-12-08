#ifndef __PWM_MODULE_H
#define __PWM_MODULE_H

#ifndef PWM_PERCENT_TIME_US
#define PWM_PERCENT_TIME_US       10U
#endif

#ifdef BOARD_PCA10059
#include "pca10059.h"
#endif /* BOARD_PCA10059 */

#define COLOR_CHANGE_STEP 1

void pwm_process_one_period(uint8_t led_idx, uint8_t duty_cycle);
void init_pwm(void);
void reset_indicator_led(void);

#endif /* __PWM_MODULE_H */