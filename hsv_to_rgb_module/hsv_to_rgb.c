#include "hsv_to_rgb.h"
#include "nrf_assert.h"
#include <string.h>

static bool count_down_flags[MODES_COUNT]; /* true means that we need to start counting down to prevent overflow */

static void hsv_to_rgb(const hsv_params_t *const hsv, rgb_params_t *const rgb);
static uint16_t update_ctr(uint16_t ctr, uint16_t step, uint16_t max_value, bool *const count_down);

/**
 * @brief Updates red, green and blue value using
 *  hue, saturation and brightness.
 *  Link to algorithm: https://stackoverflow.com/questions/24152553/hsv-to-rgb-and-back-without-floating-point-math-in-python
 *
 * @param[in] hsv pointer to hsv params struct
 * @param[out] rgb pointer to rgb params struct
 */
static void hsv_to_rgb(const hsv_params_t *const hsv, rgb_params_t *const rgb)
{
  ASSERT(hsv->hue <= 360);
  ASSERT(hsv->brightness <= 100);
  ASSERT(hsv->saturation <= 100);

  if (hsv->saturation == 0)
  {
    uint8_t brightness = COLOR_REDUCE_POW(COLOR_POW((uint32_t)hsv->brightness) * 255U / 100U);
    rgb->red = brightness;
    rgb->blue = brightness;
    rgb->green = brightness;
  }
  else
  {
    /* From 0-100, 0-360 to 0-255 */
    uint8_t hue = COLOR_REDUCE_POW(COLOR_POW((uint32_t)hsv->hue) * 255 / 360);
    uint8_t saturation = COLOR_REDUCE_POW(COLOR_POW((uint32_t)hsv->saturation) * 255 / 100);
    uint8_t brightness = COLOR_REDUCE_POW(COLOR_POW((uint32_t)hsv->brightness) * 255 / 100);

    uint8_t region = hue / 43;
    uint8_t reminder = (hue - region * 43) * 6;
    uint8_t p = (brightness * (255 - saturation)) >> 8;
    uint8_t q = (brightness * (255 - ((saturation * reminder) >> 8))) >> 8;
    uint8_t t = (brightness * (255 - ((saturation * (255 - reminder)) >> 8))) >> 8;

    switch (region)
    {
    case 0:
      rgb->red = brightness;
      rgb->green = t;
      rgb->blue = p;
      break;

    case 1:
      rgb->red = q;
      rgb->green = brightness;
      rgb->blue = p;
      break;

    case 2:
      rgb->red = p;
      rgb->green = brightness;
      rgb->blue = t;
      break;

    case 3:
      rgb->red = p;
      rgb->green = q;
      rgb->blue = brightness;
      break;

    case 4:
      rgb->red = t;
      rgb->green = p;
      rgb->blue = brightness;
      break;

    default:
      rgb->red = brightness;
      rgb->green = p;
      rgb->blue = q;
      break;
    }
  }
}

/**
 * @brief Function for updating counter that contains unsigned value
 *  that counts from 0 to max_value if count_down == 0 then
 *  counts from max_value to 0 if count_down == 1.
 * Also updates count_down value.
 *
 * @param ctr Counter that we need to change
 * @param step Value changing step
 * @param max_value Max counter value
 * @param count_down 1 if couning down, 0 otherwise. Updates on overflow and underflow
 * @return uint16_t new ctr value
 */
static uint16_t update_ctr(uint16_t ctr, uint16_t step, uint16_t max_value, bool *const count_down)
{
  if (*count_down)
  {
    if (ctr < step)
    {
      *count_down = 0;
      return ctr + step;
    }

    ctr -= step;
  }
  else
  {
    if (ctr > max_value - step)
    {
      *count_down = 1;
      return ctr - step;
    }

    ctr += step;
  }

  return ctr;
}

/**
 * @brief This function handles logic of changing hsv color depending on
 *  selected mode and step
 *
 * @param hsv HSV values
 * @param step step to increment
 * @param mode mode to handle
 * @return rgb_params_t
 */
rgb_params_t color_changing_machine(hsv_params_t *const hsv, uint16_t step, color_changing_mode_t mode)
{
  static rgb_params_t rgb_values =
  {
    .red = 0,
    .green = 0,
    .blue = 0
  };

  ASSERT(mode < MODES_COUNT);

  /* Update rgb_params first */
  hsv_to_rgb(hsv, &rgb_values);

  switch (mode)
  {
    case NO_CHANGE:
      /* Nothing to do */
      break;

    case HUE_CHANGE:
      hsv->hue = update_ctr(hsv->hue, step, HUE_MAX_VALUE, &count_down_flags[mode]);
      break;

    case SATURATION_CHANGE:
      hsv->saturation = update_ctr(hsv->saturation, step, SAT_MAX_VALUE, &count_down_flags[mode]);
      break;

    case BRIGHTNESS_CHANGE:
      hsv->brightness = update_ctr(hsv->brightness, step, BRIGHT_MAX_VALUE, &count_down_flags[mode]);
      break;

    default:
      break;
  }

  return rgb_values;
}

bool validate_hsv_by_ptr(void* ptr, uint16_t size)
{
  hsv_params_t *hsv = ptr;
  if (size != sizeof(hsv_params_t))
  {
    return false;
  }

  return (hsv->hue <= HUE_MAX_VALUE &&
    hsv->saturation <= SAT_MAX_VALUE &&
    hsv->brightness <= BRIGHT_MAX_VALUE);
}


/**
 * @brief Using pointer for rgb
 *  Link to algorithm: https://stackoverflow.com/questions/24152553/hsv-to-rgb-and-back-without-floating-point-math-in-python
 *
 * @param[in] rgb pointer to rgb params struct
 * @returns hsv params struct equal to rgb
 */
hsv_params_t hsv_by_rgb(const rgb_params_t rgb)
{
  hsv_params_t hsv;

  uint8_t rgb_max = MAX(rgb.red, MAX(rgb.green, rgb.blue));
  uint8_t rgb_min = MIN(rgb.red, MIN(rgb.green, rgb.blue));

  hsv.brightness = rgb_max;
  hsv.brightness = COLOR_REDUCE_POW(COLOR_POW(hsv.brightness) * BRIGHT_MAX_VALUE / 255); /* from [0; 255] to [0; 100] */

  if (rgb_max == 0)
  {
    return hsv;
  }

  hsv.saturation = 255 * (rgb_max - rgb_min) / rgb_max;
  hsv.saturation = COLOR_REDUCE_POW(COLOR_POW(hsv.saturation) * SAT_MAX_VALUE / 255);

  if (hsv.saturation == 0)
  {
    return hsv;
  }

  if (rgb_max == rgb.red)
  {
    hsv.hue = 43 * (rgb.green - rgb.blue) / (rgb_max - rgb_min);
  }
  else if (rgb_max == rgb.green)
  {
    hsv.hue = 85 + 43 * (rgb.blue - rgb.red) / (rgb_max - rgb_min);
  }
  else /* rgb_max == blue */
  {
    hsv.hue = 171 + 43 * (rgb.red - rgb.green) / (rgb_max - rgb_min);
  }

  hsv.hue = COLOR_REDUCE_POW(COLOR_POW(hsv.hue) * HUE_MAX_VALUE / 255);
  return hsv;
}