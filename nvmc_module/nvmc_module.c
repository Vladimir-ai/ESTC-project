#include "nvmc_module.h"
#include "nrfx_nvmc.h"

static bool last_write_is_ok = true;
static bool last_pg_is_erased = true;

static uint32_t nvmc_find_last_used_addr(void);
static uint32_t nvmc_find_last_used_addr_on_page(uint8_t pg_idx);

static uint32_t nvmc_find_last_used_addr_on_page(uint8_t pg_idx)
{
  ASSERT(pg_idx < 3);
  uint32_t ret_addr = NVMC_START_APP_DATA_ADDR + pg_idx * CODE_PAGE_SIZE;
  uint32_t curr_addr;

  for(curr_addr = ret_addr; curr_addr + NVMC_STUCT_SIZE < NVMC_END_APP_DATA_ADDR; curr_addr += NVMC_STUCT_SIZE)
  {
    if (validate_hsv_by_ptr((void*) curr_addr, NVMC_STUCT_SIZE))
    {
      ret_addr = curr_addr;
    }
    else
    {
      break;
    }
  }

  return ret_addr;
}

static uint32_t nvmc_find_last_used_addr(void)
{
  uint8_t page_idx;
  uint32_t curr_addr;
  uint32_t ret_addr = NVMC_START_APP_DATA_ADDR;

  for (page_idx = 0; page_idx < NVMC_PAGES_CNT; page_idx++)
  {
    curr_addr = nvmc_find_last_used_addr_on_page(page_idx);

    if (curr_addr % CODE_PAGE_SIZE != 0)
    {
      ret_addr = curr_addr;
    }
  }

  return ret_addr;
}

hsv_params_t nvmc_find_last_record(void)
{
  hsv_params_t hsv = HSV_STRUCT_DEFAULT_VALUE;
  void* addr;
  addr = (void*) nvmc_find_last_used_addr();

  if (validate_hsv_by_ptr(addr, NVMC_STUCT_SIZE))
  {
    hsv = *((hsv_params_t*)addr);
  }

  return hsv;
}

void nvmc_write_new_record(hsv_params_t curr_params)
{
  uint32_t addr;
  addr = (uint32_t)nvmc_find_last_used_addr();

  if (((addr + NVMC_STUCT_SIZE) / CODE_PAGE_SIZE) > (addr / CODE_PAGE_SIZE))
  {
    /* If page was switched, then initiate partial erase */
    last_pg_is_erased = false;
    nrfx_nvmc_page_partial_erase_init((addr / CODE_PAGE_SIZE) * CODE_PAGE_SIZE,
                                      NVMC_ERASE_DURATION_MS);
  }

  if(addr + NVMC_STUCT_SIZE > NVMC_END_APP_DATA_ADDR)
  {
    addr = NVMC_START_APP_DATA_ADDR;
  }

  nrfx_nvmc_bytes_write(addr + NVMC_STUCT_SIZE, &curr_params, NVMC_STUCT_SIZE);
  last_write_is_ok = false; // let's check it on next Interrupt
}

void nvmc_erase_last_written_page(void)
{
  if(!last_write_is_ok)
  {
    last_write_is_ok = nrfx_nvmc_write_done_check();
  }

  if (last_write_is_ok &&
      !last_pg_is_erased &&
      nrfx_nvmc_page_partial_erase_continue())
  {
    last_pg_is_erased = true;
  }
}