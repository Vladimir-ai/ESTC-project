#ifndef _FSTORAGE_MODULE
#define _FSTORAGE_MODULE

void save_state(void);
void init_flash(void);
void wait_for_flash_ready(nrf_fstorage_t const * p_fstorage);

#endif /* _FSTORAGE_MODULE */