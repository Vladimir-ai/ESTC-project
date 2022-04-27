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

#ifndef ESTC_SERVICE_H__
#define ESTC_SERVICE_H__

#include <stdint.h>

#include "ble.h"
#include "sdk_errors.h"

/* 7c8b0000-be9e-4ae9-a6b5-afff63f4f815 */
#define ESTC_BASE_UUID                { 0x15, 0xF8, 0xF4, 0x63, 0xFF, 0xAF, /* - */  \
                                        0xB5, 0xA6, /* - */                          \
                                        0xE9, 0x4A, /* -  */                         \
                                        0x9E, 0xBE, /* - */                          \
                                        0x00, 0x00, 0x8B, 0x7C }
/* Use 7394 as UUID */
#define ESTC_SERVICE_UUID             0x7394

#define ESTC_GATT_CHAR_1_UUID         0x1234
#define ESTC_NOTIFY_CHAR_UUID         0x1122
#define ESTC_INDICATION_CHAR_UUID     0x3344

#define ESTC_NOTIFY_CHAR_DEF_VAL      0xABCD
#define ESTC_INDICATION_CHAR_DEF_VAL  0x0123

#define ESTC_USER_CHAR_1_DESCR        "Random user description"
#define ESTC_NOTIFY_CHAR_DESCR        "Characteristic with nofitications"
#define ESTC_INDICATION_CHAR_DESCR    "Characteristic with indications"


typedef struct
{
    uint16_t service_handle;
    uint16_t connection_handle;
    ble_gatts_char_handles_t first_characteristic_handle;
    ble_gatts_char_handles_t notifying_characteristic_handle;
    ble_gatts_char_handles_t indicating_characteristic_handle;
} ble_estc_service_t;

ret_code_t estc_ble_service_init(ble_estc_service_t *service);

void estc_ble_service_on_ble_event(const ble_evt_t *ble_evt, void *ctx);

void estc_update_characteristic_1_value(ble_estc_service_t *service, int32_t *value);

void notifying_char_update(uint16_t conn_handle, uint16_t value_handle,
                           uint8_t type, uint8_t *new_value, uint16_t len);

#endif /* ESTC_SERVICE_H__ */