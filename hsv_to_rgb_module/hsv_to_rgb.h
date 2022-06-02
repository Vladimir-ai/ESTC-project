#ifndef _RGB_HSV_UTILS
#define _RGB_HSV_UTILS

#include <nrfx.h>

#define COLOR_POWER                     10      //must be greater than 0 less than 10 cus using 32 bit for calculations
#define COLOR_POW(color)                ((uint32_t)(color) << COLOR_POWER)
#define COLOR_REDUCE_POW(color)         ((uint32_t)(color) >> COLOR_POWER)

#define HUE_MAX_VALUE                   360
#define SAT_MAX_VALUE                   100
#define BRIGHT_MAX_VALUE                100

#define RGB_MAX_VALUE                   255

#define HSV_STRUCT_DEFAULT_VALUE        \
{                                       \
  .hue = 0,                             \
  .saturation = 100,                    \
  .brightness = 100                          \
}                                       \

typedef struct rgb_params_s
{
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} rgb_params_t;

typedef struct hsv_params_s
{
  uint16_t hue;
  uint8_t saturation;
  uint8_t brightness;
} hsv_params_t;

typedef enum color_changing_mode_e
{
  NO_CHANGE         = 0,
  HUE_CHANGE        = 1,
  SATURATION_CHANGE = 2,
  BRIGHTNESS_CHANGE = 3,
  MODES_COUNT       = 4
} color_changing_mode_t;

rgb_params_t color_changing_machine(hsv_params_t *const hsv, uint16_t step, color_changing_mode_t mode);

bool validate_hsv_by_ptr(void* ptr, uint16_t size);
void hsv_to_rgb(const hsv_params_t *const hsv, rgb_params_t *const rgb);
hsv_params_t hsv_by_rgb(const rgb_params_t rgb);

#endif /* _RGB_HSV_UTILS */