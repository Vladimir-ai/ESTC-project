#include "g_context.h"
#include "nrf_fstorage_sd.h"
#include "nrf_log.h"
#include "nrf_fstorage.h"


/* Don't need to define it in the handler */
typedef struct nvram_struct_s
{
  hsv_params_t hsv_values;
  uint8_t led_mode;
  uint8_t aligned[3];
} nvram_struct_t;


static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt);


NRF_FSTORAGE_DEF(nrf_fstorage_t fstorage) =
{
    .evt_handler = fstorage_evt_handler,
    .start_addr = 0xde000,
    .end_addr   = 0xdefff
};


void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage)
{
    /* While fstorage is busy, sleep and wait for an event. */
    while (nrf_fstorage_is_busy(p_fstorage))
    {
        (void) sd_app_evt_wait();
    }
}


static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
  if (p_evt->result != NRF_SUCCESS)
  {
    NRF_LOG_INFO("--> Event received: ERROR while executing an fstorage operation.");
    return;
  }

  switch (p_evt->id)
  {
    case NRF_FSTORAGE_EVT_WRITE_RESULT:
    {
      NRF_LOG_INFO("--> Event received: wrote %d bytes at address 0x%x.",
                    p_evt->len, p_evt->addr);
    } break;

    case NRF_FSTORAGE_EVT_ERASE_RESULT:
    {
      NRF_LOG_INFO("--> Event received: erased %d page from address 0x%x.",
                    p_evt->len, p_evt->addr);
    } break;

    default:
        break;
  }
}


void init_flash(void)
{
  int ret = nrf_fstorage_init(&fstorage, &nrf_fstorage_sd, NULL);
  nvram_struct_t nvram_struct = { 0 };
  APP_ERROR_CHECK(ret);

  ret = nrf_fstorage_read(&fstorage, fstorage.start_addr, &nvram_struct, sizeof(nvram_struct));
  APP_ERROR_CHECK(ret);

  if (!validate_hsv_by_ptr(&nvram_struct.hsv_values, sizeof(nvram_struct.hsv_values)))
  {
    NRF_LOG_INFO("LED state is not set in the flash, defaulting to OFF");
  }
  else
  {
    g_app_data.current_led_mode = nvram_struct.led_mode;
    g_app_data.hsv_value = nvram_struct.hsv_values;
    hsv_to_rgb(&nvram_struct.hsv_values, &g_app_data.rgb_value);
  }
}


void save_state(void)
{
  static nvram_struct_t current_state;
  current_state.led_mode = g_app_data.current_led_mode;
  current_state.hsv_values = g_app_data.hsv_value;

  nrf_fstorage_erase(&fstorage, fstorage.start_addr, 1, NULL);
  nrf_fstorage_write(&fstorage,
                     fstorage.start_addr,
                     &current_state,
                     sizeof(current_state), NULL);
}

