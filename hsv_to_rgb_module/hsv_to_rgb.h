#ifndef _RGB_HSV_UTILS
#define _RGB_HSV_UTILS

#include <stdint.h>

#define COLOR_POWER                     10      //must be greater than 0 less than 10 cus using 32 bit for calculations
#define COLOR_POW(color)                ((uint32_t)(color) << COLOR_POWER)
#define COLOR_REDUCE_POW(color)         ((uint32_t)(color) >> COLOR_POWER)

#define HUE_MAX_VALUE                   360
#define SAT_MAX_VALUE                   100
#define BRIGHT_MAX_VALUE                100

#define DEFAULT_COLOR_PARAMS              \
{                                         \
  .hue = 0,                               \
  .saturation = 0,                        \
  .brightness = 0,                        \
  .hue_count_down_flag = 0,               \
  .sat_count_down_flag = 0,               \
  .brightness_count_down_flag = 0,        \
  .red = 0,                               \
  .green = 0,                             \
  .blue = 0                               \
}

typedef struct color_params_s
{
  uint16_t hue:9;
  uint16_t saturation:7;
  uint8_t  brightness:7;
  uint8_t  hue_count_down_flag:1;
  uint8_t  sat_count_down_flag:1;
  uint8_t  brightness_count_down_flag:1;
  uint8_t  reserved_flags:6; /* I think we can use it for additional args */
  uint8_t  red;
  uint8_t  green;
  uint8_t  blue;
} color_params_t;

typedef enum color_changing_mode_e
{
  NO_CHANGE         = 0,
  HUE_CHANGE        = 1,
  SATURATION_CHANGE = 2,
  BRIGHTNESS_CHANGE = 3,
  MODES_COUNT       = 4
} color_changing_mode_t;

void color_changing_machine(color_params_t *const params, uint16_t step, color_changing_mode_t mode);

#endif /* _RGB_HSV_UTILS */