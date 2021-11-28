#ifndef _NVMC_MODULE_H
#define _NVMC_MODULE_H

#include "nrf_dfu_types.h"
#include "nrf_bootloader_info.h"
#include "hsv_to_rgb.h"

#ifdef BOARD_PCA10059

#define NVMC_START_APP_DATA_ADDR       (0x000E0000U - NRF_DFU_APP_DATA_AREA_SIZE)
#define NVMC_END_APP_DATA_ADDR         0x000E0000U
#define NVMC_PAGES_CNT                 2

#define NVMC_STUCT_SIZE                 (sizeof(hsv_params_t))

#define NVMC_ERASE_DURATION_MS         1

hsv_params_t nvmc_find_last_record(void);
void nvmc_write_new_record(hsv_params_t curr_params);
void nvmc_erase_last_written_page(void);


#endif /* BOARD_PCA10059 */

#endif /* _NVMC_MODULE_H */