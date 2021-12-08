#include "usbd_module.h"
#include "app_usbd_cdc_acm.h"
#include "nrf_log.h"
#include "g_context.h"
#include "cli_usb.h"
#include <string.h>
#include <stdarg.h>

static void usb_ev_handler(app_usbd_class_inst_t const * p_inst,
                           app_usbd_cdc_acm_user_event_t event);

APP_USBD_CDC_ACM_GLOBAL_DEF(usb_cdc_acm,
                            usb_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_AT_V250);

static uint8_t m_rx_buffer[READ_SIZE];

static char input_str[MAX_INPUT_STR_SIZE + 1];
static uint8_t input_str_size = 0;
static char output_str[MAX_OUTPUT_STR_SIZE + 2];

static void print_msg(char* msg,...);


static void print_msg(char* msg,...)
{
  uint8_t msg_length;
  va_list args;

  va_start(args, msg);

  vsprintf(output_str, msg, args);

  va_end(args);

  for(msg_length = 0; msg_length < MAX_OUTPUT_STR_SIZE && output_str[msg_length] != 0; msg_length++);

  if (output_str[msg_length] != '\n')
  {
    output_str[msg_length++] = '\r';
    output_str[msg_length++] = '\n';
  }

  app_usbd_cdc_acm_write(&usb_cdc_acm, output_str, msg_length);

  memset(output_str, 0, msg_length);
  NRF_LOG_INFO("Message sent");
}

static bool is_symbol_supported(char symbol)
{
  return (symbol >= 'a' && symbol <= 'z') ||
         (symbol >= 'A' && symbol <= 'Z') ||
         (symbol >= '0' && symbol <= '9') ||
         (symbol == ' ' || symbol == '_');
}

static void clear_input(void)
{
  memset(input_str, 0, sizeof(input_str));
}

static reading_status_t add_symbol_to_string(char symbol)
{
  static uint8_t symbol_idx = 0;
  reading_status_t ret;

  if (!symbol_idx)
  {
    memset(input_str, 0, sizeof(input_str));
  }

  /* Skip all not supported symbols at the start of string*/
  if (!is_symbol_supported(symbol) && !symbol_idx)
  {
    return READING_OK;
  }

  if(symbol_idx > MAX_INPUT_STR_SIZE)
  {
    ret = READING_ERR_STR_TOO_LONG;
  }
  else if(is_symbol_supported(symbol))
  {
    input_str[symbol_idx++] = symbol;
    ret = READING_OK;
  }
  else
  {
    ret = READING_LINE_END;
  }

  if (ret != READING_OK)
  {
    /* To clear buffer on the next reading attempt */
    input_str_size = symbol_idx;
    symbol_idx = 0;
  }

  return ret;
}

void process_symbol(char symbol)
{
  reading_status_t symbol_reading_status;

  symbol_reading_status = add_symbol_to_string(symbol);

  if (symbol_reading_status == READING_ERR_STR_TOO_LONG)
  {
    print_msg("Error: string too long");
    clear_input();
  }

  if (symbol_reading_status == READING_LINE_END)
  {
    process_input_string(input_str, input_str_size, &print_msg);
    clear_input();
  }
}

void init_usbd(void)
{
  app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&usb_cdc_acm);
  APP_ERROR_CHECK(app_usbd_class_append(class_cdc_acm));
}

static void usb_ev_handler(app_usbd_class_inst_t const * p_inst,
                           app_usbd_cdc_acm_user_event_t event)
{
  switch (event)
  {
  case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
  {
      ret_code_t ret;
      ret = app_usbd_cdc_acm_read(&usb_cdc_acm, m_rx_buffer, READ_SIZE);
      UNUSED_VARIABLE(ret);
      break;
  }
  case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
  {
      break;
  }
  case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
  {
      NRF_LOG_INFO("tx done");
      break;
  }
  case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
  {
      ret_code_t ret;
      do
      {
        /*Get amount of data transfered*/
        size_t size = app_usbd_cdc_acm_rx_size(&usb_cdc_acm);
        NRF_LOG_INFO("rx size: %d", size);

        /* It's the simple version of an echo. Note that writing doesn't
          * block execution, and if we have a lot of characters to read and
          * write, some characters can be missed.
          */
        if (m_rx_buffer[0] == '\r' || m_rx_buffer[0] == '\n')
        {
            ret = app_usbd_cdc_acm_write(&usb_cdc_acm, "\r\n", 2);
        }
        else
        {
            ret = app_usbd_cdc_acm_write(&usb_cdc_acm,
                                          m_rx_buffer,
                                          READ_SIZE);
        }

        process_symbol(*m_rx_buffer);

        /* Fetch data until internal buffer is empty */
        ret = app_usbd_cdc_acm_read(&usb_cdc_acm,
                                    m_rx_buffer,
                                    READ_SIZE);

      } while (ret == NRF_SUCCESS);

      break;
  }
  default:
      break;
  }
}