#include "nrf_log.h"
#include "cli_usb.h"
#include "g_context.h"
#include <ctype.h>

static console_output_t result_buf;
static update_pwm_handler_t pwm_force_update_handler;
static nvmc_handler_t nvmc_write_handler;

static cmd_t get_cmd_from_args(const char *args, uint8_t args_size)
{
  cmd_t ret = NO_CMD;
  uint8_t cmd_size, cmd_idx;

  for (cmd_idx = 0; cmd_idx < NO_CMD; cmd_idx++)
  {
    cmd_size = strlen(cmd_list[cmd_idx]);

    if (args_size >= cmd_size &&
        !strncmp(args, cmd_list[cmd_idx], cmd_size) &&
        (isspace((uint8_t)args[cmd_size]) || args[cmd_size] == 0))
    {
      ret = cmd_idx;
      break;
    }
  }

  return ret;
}

static const char* find_next_arg(const char* args, uint8_t input_str_len)
{
  const char* ret = NULL;
  uint8_t offset = 0;
  ASSERT(args != NULL);


  if (!isspace((uint8_t)args[offset]))
  {

    for (offset = 0; offset < input_str_len; offset++)
    {

      if (isspace((uint8_t)args[offset]))
      {
        ret = &args[offset + 1];
        break;
      }
    }
  }

  return ret;
}

/**
 * @brief Given args and result arrays returns args_count numbers from args str.
 */
static bool read_numeric_args(const char *args, uint8_t input_str_len, uint16_t *result, uint8_t args_count)
{
  uint16_t value;
  const char *current_args = args;
  const size_t args_end = (size_t)args + input_str_len;

  for(uint8_t arg_idx = 0; arg_idx < args_count; arg_idx++)
  {
    if (current_args == NULL)
    {
      return false;
    }

    if(sscanf(current_args, "%hu", &value) == EOF)
    {
      return false;
    }

    current_args = find_next_arg(current_args, args_end - (size_t)current_args);
    result[arg_idx] = value;
  }

  return true;
}

void process_input_string(const char *input_str, uint8_t input_str_len, msg_hadler_t msg_handler)
{
  cmd_t cmd = get_cmd_from_args(input_str, input_str_len);
  const char *args_pointer = find_next_arg(input_str, input_str_len - cmd_arg_size[cmd]);
  uint8_t args_len = 0;

  if (args_pointer != NULL)
  {
    args_len = input_str_len - (args_pointer - args_pointer);
  }

  if (cmd == RGB_CMD)
  {
    uint16_t numeric_args[cmd_arg_size[cmd]];
    memset(numeric_args, 0, sizeof(numeric_args));

    if (read_numeric_args(args_pointer, args_len, numeric_args, 3))
    {
      if (numeric_args[0] <= RGB_MAX_VALUE &&
          numeric_args[1] <= RGB_MAX_VALUE &&
          numeric_args[2] <= RGB_MAX_VALUE)
      {
        result_buf.rgb.red = (uint8_t) numeric_args[0];
        result_buf.rgb.green = (uint8_t) numeric_args[1];
        result_buf.rgb.blue = (uint8_t) numeric_args[2];

        NRF_LOG_INFO("RGB Cmd: %d, %d, %d", numeric_args[0], numeric_args[1], numeric_args[2]);
        msg_handler("Color changed to rgb: red %hu, green %hu, blue %hu",
                   result_buf.rgb.red, result_buf.rgb.green, result_buf.rgb.blue);

        g_app_data.current_hsv = hsv_by_rgb(result_buf.rgb, 1);
        pwm_force_update_handler();
      }
      else
      {
        msg_handler("Error: incorrect argument value");
      }
    }
    else
    {
      msg_handler("Error: args: <r> <g> <b>");
    }
  }
  else if (cmd == HSV_CMD)
  {
    uint16_t numeric_args[cmd_arg_size[cmd]];
    memset(numeric_args, 0, sizeof(numeric_args));

    if (read_numeric_args(args_pointer, args_len, numeric_args, 3))
    {
      if (numeric_args[0] <= HUE_MAX_VALUE &&
          numeric_args[1] <= SAT_MAX_VALUE &&
          numeric_args[2] <= BRIGHT_MAX_VALUE)
      {
        result_buf.hsv.hue = numeric_args[0];
        result_buf.hsv.saturation = (uint8_t) numeric_args[1];
        result_buf.hsv.brightness = (uint8_t) numeric_args[2];

        msg_handler("Color changed to hsv: hue %hu, sat %hu, bright %hu",
                   numeric_args[0], numeric_args[1], numeric_args[2]);
        NRF_LOG_INFO("HSV Cmd: %d, %d, %d", numeric_args[0], numeric_args[1], numeric_args[2]);


        g_app_data.current_hsv = result_buf.hsv;
        pwm_force_update_handler();
      }
      else
      {
        msg_handler("Error: Error: incorrect argument value");
      }
    }
    else
    {
      msg_handler("Error: args: <h> <s> <v>");
    }
  }
  else if (cmd == SAVE_CMD)
  {
    nvmc_write_handler(g_app_data.current_hsv);
    msg_handler("Current state saved");
  }
  else if (cmd == HELP_CMD)
  {
    msg_handler("Usage: rgb <r> <g> <b> or hsv <h> <s <v> or save");
  }
  else if (cmd == NO_CMD)
  {
    msg_handler("Error: incorrect cmd name");
  }
}

void init_cli(update_pwm_handler_t pwm_force_update, nvmc_handler_t nvmc_handler)
{
  pwm_force_update_handler = pwm_force_update;
  nvmc_write_handler = nvmc_handler;
}