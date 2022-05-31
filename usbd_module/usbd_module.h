#ifndef _USBD_MODULE_H
#define _USBD_MODULE_H

#include "nrfx.h"
#include "hsv_to_rgb.h"

#define CDC_ACM_COMM_INTERFACE  2
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN3

#define CDC_ACM_DATA_INTERFACE  3
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN4
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT4
#define READ_SIZE               1

#define MAX_INPUT_STR_SIZE    100
#define MAX_OUTPUT_STR_SIZE   80


typedef enum reading_status_s
{
  READING_OK = 0,
  READING_CONTINUE = 1,
  READING_LINE_END = 2,
  READING_ERR_INCORRECT_SYMBOL = 3,
  READING_ERR_STR_TOO_LONG = 4,
  READING_ERR_INCORRECT_CMD_NAME = 5,
  READING_ERR_INCORRECT_ARG = 6,
} reading_status_t;

void init_usbd(void);

#endif /* _USBD_MODULE_H */