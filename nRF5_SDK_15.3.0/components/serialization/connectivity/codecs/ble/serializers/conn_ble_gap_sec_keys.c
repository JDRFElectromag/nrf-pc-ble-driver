/**
 * Copyright (c) 2014 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#include "conn_ble_gap_sec_keys.h"
#include "ser_config.h"
#include "nrf_error.h"
#include "nordic_common.h"
#include <stddef.h>

typedef enum {
    BLE_DATA_BUF_FREE,
    BLE_DATA_BUF_IN_USE,
    BLE_DATA_BUF_LAST_DIRTY,
} ble_data_buf_state_t;

ser_ble_gap_conn_keyset_t m_conn_keys_table[SER_MAX_CONNECTIONS];
#if NRF_SD_BLE_API_VERSION >= 6

typedef struct
{
    ble_data_buf_state_t state;
    uint32_t id;
    uint8_t  ble_data[SER_MAX_ADV_DATA];
} ble_data_item_t;

typedef struct
{
    uint8_t * buf1;
    uint8_t * buf2;
} adv_set_data_t;

static adv_set_data_t adv_set_data[BLE_GAP_ADV_SET_COUNT_MAX];

ble_data_item_t m_ble_data_pool[8];
static uint8_t * mp_scan_data;

#endif

void conn_ble_gap_sec_keys_init(void)
{
    uint32_t i;
    for (i = 0; i < SER_MAX_CONNECTIONS; i++)
    {
        m_conn_keys_table[i].conn_active = 0;
    }
#if NRF_SD_BLE_API_VERSION >= 6
    for (i = 0; i < 8; i++)
    {
        m_ble_data_pool[i].state = BLE_DATA_BUF_FREE;
        m_ble_data_pool[i].id = 0;
    }
#endif
}

uint32_t conn_ble_gap_sec_context_create(uint32_t *p_index)
{
    uint32_t err_code = NRF_ERROR_NO_MEM;
    uint32_t i;

    for (i=0; i<SER_MAX_CONNECTIONS; i++)
    {
        if (!m_conn_keys_table[i].conn_active)
        {
            m_conn_keys_table[i].conn_active = 1;
            *p_index = i;
            err_code = NRF_SUCCESS;
            break;
        }
    }

    return err_code;
}

uint32_t conn_ble_gap_sec_context_destroy(uint16_t conn_handle)
{
  uint32_t err_code = NRF_ERROR_NOT_FOUND;
  uint32_t i;

  for (i=0; i<SER_MAX_CONNECTIONS; i++ )
  {
    if (  m_conn_keys_table[i].conn_handle == conn_handle )
    {
        m_conn_keys_table[i].conn_active = 0;
        err_code = NRF_SUCCESS;
        break;
    }
  }

  return err_code;
}

uint32_t conn_ble_gap_sec_context_find(uint16_t conn_handle, uint32_t *p_index)
{
  uint32_t err_code = NRF_ERROR_NOT_FOUND;
  uint32_t i;

  for (i=0; i<SER_MAX_CONNECTIONS; i++ )
  {
    if ((m_conn_keys_table[i].conn_handle == conn_handle) && (m_conn_keys_table[i].conn_active == 1))
    {
        *p_index = i;
        err_code = NRF_SUCCESS;
        break;
    }
  }

  return err_code;
}

#if NRF_SD_BLE_API_VERSION >= 6
uint8_t * conn_ble_gap_ble_data_buf_alloc(uint32_t id)
{
    uint32_t i;

    if (id == 0)
    {
        return NULL;
    }

    /* First find if given id already allocated the buffer. */
    for (i = 0; i < ARRAY_SIZE(m_ble_data_pool); i++)
    {
        if (m_ble_data_pool[i].id == id)
        {
            return m_ble_data_pool[i].ble_data;
        }
    }

    for (i = 0; i < ARRAY_SIZE(m_ble_data_pool); i++)
    {
        if (m_ble_data_pool[i].state == BLE_DATA_BUF_FREE)
        {
            m_ble_data_pool[i].state = BLE_DATA_BUF_IN_USE;
            m_ble_data_pool[i].id = id;
            return m_ble_data_pool[i].ble_data;
        }
    }

    return NULL;
}


uint32_t conn_ble_gap_ble_data_buf_free(uint8_t * p_data)
{
    uint32_t i;

    if (p_data == NULL)
    {
        return 0;
    }
    /* First find if given id already allocated the buffer. */
    for (i = 0; i < ARRAY_SIZE(m_ble_data_pool); i++)
    {
        if (m_ble_data_pool[i].ble_data == p_data)
        {
            uint32_t id = m_ble_data_pool[i].id;
            m_ble_data_pool[i].id = 0;
            m_ble_data_pool[i].state = BLE_DATA_BUF_FREE;
            return id;
        }
    }

    return 0;
}

void conn_ble_gap_ble_data_mark_dirty(uint8_t * p_data)
{
    uint32_t i;

    if (p_data)
    {
        for (i = 0; i < ARRAY_SIZE(m_ble_data_pool); i++)
        {
            if (m_ble_data_pool[i].ble_data == p_data)
            {
                m_ble_data_pool[i].state = BLE_DATA_BUF_LAST_DIRTY;
            }
        }
    }
}

void conn_ble_gap_ble_adv_data_mark_dirty(uint8_t * p_data1, uint8_t *p_data2)
{
    uint32_t i;

    /* First find if given id already allocated the buffer. */
    for (i = 0; i < ARRAY_SIZE(m_ble_data_pool); i++)
    {
        if (m_ble_data_pool[i].state == BLE_DATA_BUF_LAST_DIRTY)
        {
            conn_ble_gap_ble_data_buf_free(m_ble_data_pool[i].ble_data);
        }
    }

    conn_ble_gap_ble_data_mark_dirty(p_data1);
    conn_ble_gap_ble_data_mark_dirty(p_data2);
}

void conn_ble_gap_scan_data_set(uint8_t * p_scan_data)
{
	mp_scan_data = p_scan_data;
}

void conn_ble_gap_scan_data_unset(bool free)
{
	if (mp_scan_data)
	{
		if (free)
		{
			conn_ble_gap_ble_data_buf_free(mp_scan_data);
		}
		mp_scan_data = NULL;
	}
}

void conn_ble_gap_set_adv_data_set(uint8_t adv_handle, uint8_t * buf1, uint8_t * buf2)
{
    if (adv_handle == BLE_GAP_ADV_SET_HANDLE_NOT_SET)
    {
        adv_handle = BLE_GAP_ADV_SET_COUNT_MAX - 1;
    }
    conn_ble_gap_ble_adv_data_mark_dirty(adv_set_data[adv_handle].buf1, adv_set_data[adv_handle].buf2);

    adv_set_data[adv_handle].buf1 = buf1;
    adv_set_data[adv_handle].buf2 = buf2;
}
#endif

