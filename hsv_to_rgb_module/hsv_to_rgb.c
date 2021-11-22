#include "hsv_to_rgb.h"
#include "nrf_assert.h"

/**
 * @brief Updates red, green and blue value using
 *  hue, saturation and brightness.
 *  Link to algorithm: https://stackoverflow.com/questions/24152553/hsv-to-rgb-and-back-without-floating-point-math-in-python
 *
 * @param param pointer to color params struct
 */
static void hsv_to_rgb(color_params_t *const param)
{

  if (param->saturation == 0)
  {
    uint32_t brightness = COLOR_REDUCE_POW(COLOR_POW((uint32_t)param->brightness) * 255U / 100U);
    param->red = brightness;
    param->blue = brightness;
    param->green = brightness;
  }
  else
  {
    /* From 0-100, 0-360 to 0-255 */
    uint8_t hue = COLOR_REDUCE_POW(COLOR_POW((uint32_t)param->hue) * 255 / 360);
    uint8_t saturation = COLOR_REDUCE_POW(COLOR_POW((uint32_t)param->saturation) * 255 / 100);
    uint8_t brightness = COLOR_REDUCE_POW(COLOR_POW((uint32_t)param->brightness) * 255 / 100);

    uint8_t region = hue / 43;
    uint8_t reminder = (hue - region * 43) * 6;
    uint8_t p = (brightness * (255 - saturation)) >> 8;
    uint8_t q = (brightness * (255 - ((saturation * reminder) >> 8))) >> 8;
    uint8_t t = (brightness * (255 - ((saturation * (255 - reminder)) >> 8))) >> 8;

    switch (region)
    {
    case 0:
      param->red = brightness;
      param->green = t;
      param->blue = p;
      break;

    case 1:
      param->red = q;
      param->green = brightness;
      param->blue = p;
      break;

    case 2:
      param->red = p;
      param->green = brightness;
      param->blue = t;
      break;

    case 3:
      param->red = p;
      param->green = q;
      param->blue = brightness;
      break;

    case 4:
      param->red = t;
      param->green = p;
      param->blue = brightness;
      break;

    default:
      param->red = brightness;
      param->green = p;
      param->blue = q;
      break;
    }
  }

}

/**
 * @brief Function for updating counter that contains unsigned value
 *  that counts from 0 to max_value if count_down == 0 then
 *  counts from max_value to 0 if count_down == 1.
 *
 * @param ctr Pointer to the counter that we need to change
 * @param step Value changing step
 * @param max_value Max counter value
 * @param count_down 1 if couning down, 0 otherwise
 * @return uint8_t new count down value
 */
__STATIC_INLINE uint8_t update_ctr(uint16_t *const ctr, uint16_t step, uint16_t max_value, uint8_t count_down)
{
  if (count_down)
  {
    if (*ctr < step)
    {
      (*ctr) += step;
      return !count_down;
    }

    (*ctr) -= step;
  }
  else
  {
    if (*ctr >= max_value - step)
    {
      *ctr -= step;
      return !count_down;
    }

    (*ctr) += step;
  }

  return count_down;
}

void color_changing_machine(color_params_t *const params, uint16_t step, color_changing_mode_t mode)
{
  /* Use ctr, because can't send bitfield pointer to func */
  static uint16_t ctr;

  ASSERT(mode < MODES_COUNT);

  /* Update rgb_params first */
  hsv_to_rgb(params);

  switch (mode)
  {
    case NO_CHANGE:
      /* Nothing to do */
      break;

    case HUE_CHANGE:
      ctr = params->hue;
      params->hue_count_down_flag = update_ctr(&ctr, step, HUE_MAX_VALUE, params->hue_count_down_flag);
      params->hue = ctr;
      break;

    case SATURATION_CHANGE:
      ctr = params->saturation;
      params->sat_count_down_flag = update_ctr(&ctr, step, SAT_MAX_VALUE, params->sat_count_down_flag);
      params->saturation = ctr;
      break;

    case BRIGHTNESS_CHANGE:
      ctr = params->brightness;
      params->brightness_count_down_flag = update_ctr(&ctr, step, BRIGHT_MAX_VALUE, params->brightness_count_down_flag);
      params->brightness = ctr;
      break;

    default:
      break;
  }
}
