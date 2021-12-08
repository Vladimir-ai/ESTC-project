#ifndef _CLI_USB_H
#define _CLI_USB_H

#include "hsv_to_rgb.h"

#define MAX_NUM_LENGTH        3
#define MAX_CMD_NAME_LEN      6

typedef enum cmd_s
{
  RGB_CMD,
  HSV_CMD,
  SAVE_CMD,
  HELP_CMD,
  NO_CMD
} cmd_t;

static const char cmd_list[][MAX_CMD_NAME_LEN] =
{
  {"rgb"},
  {"hsv"},
  {"save"},
  {"help"},
};
static const uint8_t cmd_arg_size[] = {3, 3, 0};

typedef union console_output_s
{
  hsv_params_t hsv;
  rgb_params_t rgb;
} console_output_t;

typedef void (*update_pwm_handler_t)(void);
typedef void (*msg_hadler_t)(char* msg,...);
typedef void (*nvmc_handler_t)(hsv_params_t hsv_params);

void process_input_string(char *input_str, uint8_t input_str_size, msg_hadler_t msg_handler);
void init_cli(update_pwm_handler_t pwm_force_update, nvmc_handler_t nvmc_handler);

#endif /* _CLI_USB_H */