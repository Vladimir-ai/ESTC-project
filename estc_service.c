/**
 * Copyright 2022 Evgeniy Morozov
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE
*/

#include "estc_service.h"

#include "app_error.h"
#include "nrf_log.h"

#include "ble.h"
#include "ble_gatts.h"
#include "ble_srv_common.h"

#include "hsv_to_rgb.h"

#include "g_context.h"

static ret_code_t estc_ble_add_rgb_char(ble_estc_service_t *service);
static ret_code_t estc_ble_add_hsv_char(ble_estc_service_t *service);
static ret_code_t estc_ble_add_onoff_characteristic(ble_estc_service_t *service);
static ret_code_t estc_ble_add_change_speed_characteristic(ble_estc_service_t *service);


ret_code_t estc_ble_service_init(ble_estc_service_t *service)
{
  ret_code_t error_code = NRF_SUCCESS;
  ble_uuid128_t base_uuid = {ESTC_BASE_UUID};
  ble_uuid_t ble_uuid = {.uuid = ESTC_SERVICE_UUID };

  error_code = sd_ble_uuid_vs_add(&base_uuid, &ble_uuid.type);
  VERIFY_SUCCESS(error_code);

  error_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &service->service_handle);
  VERIFY_SUCCESS(error_code);

  error_code = estc_ble_add_rgb_char(service);
  VERIFY_SUCCESS(error_code);

  error_code = estc_ble_add_hsv_char(service);
  VERIFY_SUCCESS(error_code);

  error_code = estc_ble_add_onoff_characteristic(service);
  VERIFY_SUCCESS(error_code);

  error_code = estc_ble_add_change_speed_characteristic(service);

  return error_code;
}

static ret_code_t estc_ble_add_rgb_char(ble_estc_service_t *service)
{
  const uint8_t user_descr[] = ESTC_USER_CHAR_LED_RGB_DESCR;

  ret_code_t error_code = NRF_SUCCESS;
  ble_uuid_t ble_uuid = { .uuid = ESTC_GATT_CHAR_LED_RGB_UUID, .type = BLE_UUID_TYPE_BLE};
  ble_gatts_char_md_t char_md = { 0 };
  ble_gatts_attr_md_t attr_md = { 0 };
  ble_gatts_attr_t attr_char_value = { 0 };

  char_md.char_props.read = true;
  char_md.char_props.write = true;
  char_md.char_props.notify = true;
  char_md.char_user_desc_max_size = sizeof(user_descr);
  char_md.char_user_desc_size = sizeof(user_descr);
  char_md.p_char_user_desc = user_descr;

  // Configures attribute metadata. For now we only specify that the attribute will be stored in the softdevice
  attr_md.vloc = BLE_GATTS_VLOC_STACK;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

  attr_char_value.p_uuid = &ble_uuid;
  attr_char_value.p_attr_md = &attr_md;
  attr_char_value.max_len = sizeof(rgb_params_t);
  attr_char_value.init_len = sizeof(rgb_params_t);
  attr_char_value.p_value = (uint8_t *) &g_app_data.rgb_value;

  error_code = sd_ble_gatts_characteristic_add(service->service_handle, &char_md, &attr_char_value, &service->rgb_characteristic_handle);

  return error_code;
}


static ret_code_t estc_ble_add_hsv_char(ble_estc_service_t *service)
{
  const uint8_t notifying_user_descr[] = ESTC_HSV_CHAR_DESCR;

  ret_code_t error_code = NRF_SUCCESS;
  ble_uuid_t ble_uuid = { .uuid = ESTC_GATT_CHAR_LED_HSV_UUID, .type = BLE_UUID_TYPE_BLE };
  ble_gatts_char_md_t char_md = { 0 };
  ble_gatts_attr_md_t attr_md = { 0 };
  ble_gatts_attr_t attr_char_value = { 0 };

  /* Let's support only rgb bluetooth write */
  char_md.char_props.read = 1;
  char_md.char_props.notify = 1;
  char_md.char_user_desc_max_size = sizeof(notifying_user_descr);
  char_md.char_user_desc_size = sizeof(notifying_user_descr);
  char_md.p_char_user_desc = notifying_user_descr;

  // Configures attribute metadata. For now we only specify that the attribute will be stored in the softdevice
  attr_md.vloc = BLE_GATTS_VLOC_STACK;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);

  attr_char_value.p_uuid = &ble_uuid;
  attr_char_value.p_attr_md = &attr_md;
  attr_char_value.max_len = sizeof(g_app_data.hsv_value);
  attr_char_value.init_len = sizeof(g_app_data.hsv_value);
  attr_char_value.p_value = (uint8_t *) &g_app_data.hsv_value;

  error_code = sd_ble_gatts_characteristic_add(service->service_handle, &char_md, &attr_char_value, &service->hsv_characteristic_handle);

  return error_code;
}


static ret_code_t estc_ble_add_onoff_characteristic(ble_estc_service_t *service)
{
  const uint8_t user_descr[] = ESTC_ONOFF_CHAR_DESCR;

  ret_code_t error_code = NRF_SUCCESS;
  ble_uuid_t ble_uuid = { .uuid = ESTC_GATT_CHAR_LED_TOGGLE_UUID, .type = BLE_UUID_TYPE_BLE };
  ble_gatts_char_md_t char_md = { 0 };
  ble_gatts_attr_md_t attr_md = { 0 };
  ble_gatts_attr_t attr_char_value = { 0 };

  char_md.char_props.read = 1;
  char_md.char_props.write = 1;
  char_md.char_props.notify = 1;
  char_md.char_user_desc_max_size = sizeof(user_descr);
  char_md.char_user_desc_size = sizeof(user_descr);
  char_md.p_char_user_desc = user_descr;

  // Configures attribute metadata. For now we only specify that the attribute will be stored in the softdevice
  attr_md.vloc = BLE_GATTS_VLOC_STACK;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

  attr_char_value.p_uuid = &ble_uuid;
  attr_char_value.p_attr_md = &attr_md;
  attr_char_value.max_len = sizeof(g_app_data.flags.app_is_running);
  attr_char_value.p_value = (uint8_t *) &g_app_data.flags.app_is_running;
  attr_char_value.init_len = sizeof(g_app_data.flags.app_is_running);

  error_code = sd_ble_gatts_characteristic_add(service->service_handle, &char_md, &attr_char_value, &service->onoff_characteristic_handle);

  return error_code;
}


static ret_code_t estc_ble_add_change_speed_characteristic(ble_estc_service_t *service)
{
  const uint8_t user_descr[] = ESTC_CHANGE_SPEED_CHAR_DESCR;

  ret_code_t error_code = NRF_SUCCESS;
  ble_uuid_t ble_uuid = { .uuid = ESTC_GATT_CHAR_LED_CHANGE_SPEED_UUID, .type = BLE_UUID_TYPE_BLE };
  ble_gatts_char_md_t char_md = { 0 };
  ble_gatts_attr_md_t attr_md = { 0 };
  ble_gatts_attr_t attr_char_value = { 0 };

  char_md.char_props.read = 1;
  char_md.char_props.write = 1;
  char_md.char_props.notify = 1;
  char_md.char_user_desc_max_size = sizeof(user_descr);
  char_md.char_user_desc_size = sizeof(user_descr);
  char_md.p_char_user_desc = user_descr;

  // Configures attribute metadata. For now we only specify that the attribute will be stored in the softdevice
  attr_md.vloc = BLE_GATTS_VLOC_STACK;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

  attr_char_value.p_uuid = &ble_uuid;
  attr_char_value.p_attr_md = &attr_md;
  attr_char_value.max_len = sizeof(g_app_data.current_led_mode);
  attr_char_value.p_value = (uint8_t *) &g_app_data.current_led_mode;
  attr_char_value.init_len = sizeof(g_app_data.current_led_mode);

  error_code = sd_ble_gatts_characteristic_add(service->service_handle, &char_md, &attr_char_value,
                                               &service->change_speed_characteristic_handle);

  return error_code;
}


void construct_ble_notify(uint16_t conn_handle, uint16_t value_handle,
                          uint8_t *new_value, uint16_t len)
{
  if (conn_handle != BLE_CONN_HANDLE_INVALID)
  {
    ble_gatts_hvx_params_t hvx_params = {0};
    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = value_handle;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.offset = 0;
    hvx_params.p_len  = &len;
    hvx_params.p_data = (uint8_t*)new_value;

    sd_ble_gatts_hvx(conn_handle, &hvx_params);
  }
}
