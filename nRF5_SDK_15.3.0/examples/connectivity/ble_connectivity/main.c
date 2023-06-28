/**
 * Copyright (c) 2013 - 2019, Nordic Semiconductor ASA
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

/**@file
 *
 * @defgroup ble_sdk_app_connectivity_main main.c
 * @{
 * @ingroup ble_sdk_app_connectivity
 *
 * @brief BLE Connectivity application.
 */

#include <stdbool.h>
#include "nrf_sdm.h"
#include "nrf_soc.h"
#include "app_error.h"
#include "app_scheduler.h"
#include "nrf_sdh.h"
#include "ser_hal_transport.h"
#include "ser_conn_handlers.h"
#include "boards.h"
#include "nrf_drv_clock.h"
#include "app_timer.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_timer.h"
#include "ser_phy_debug_comm.h"
#include "ser_config.h"
#include "app_timer.h"

#if defined(NRF_DFU_TRIGGER_USB_INTERFACE_NUM) && defined(QSPI_ENABLED)
#define BOARD_DETECTION 1
#else
#define BOARD_DETECTION 0
#endif

#if BOARD_DETECTION
#include "nrf_dfu_trigger_usb.h"
#include "nrf_drv_qspi.h"
#define PCA10056_PIN_RESET 24
#define PCA10059_PIN_RESET 19
#endif

/*Macro to log and flush. Use this before entering the super loop*/
#define NRF_LOG_INFO_FLUSH(arg) \
do { \
    NRF_LOG_INFO(arg); \
    NRF_LOG_INTERNAL_FLUSH(); \
} while (0)

/* ****************************************************************************
 * --- nrf21540 routines ---
 ******************************************************************************/
#if HAS_NRF21540
#include "nrf21540.h"
#endif
static void nrf21540_initialize(void)
{
#if HAS_NRF21540
    nrf21540_init();
    nrf21540_enablePower(true);
    NRF_LOG_INFO("%s", __FUNCTION__);
#endif
}

static void nrf21540_onBleStackEnabled(void)
{
#if HAS_NRF21540
    nrf21540_bleAttach();
    NRF_LOG_INFO("%s", __FUNCTION__);
#endif
}

/* ****************************************************************************
 * --- LED routines ---
 ******************************************************************************/
static inline void led_Gpioinit(void)
{
    const uint32_t pins[] = {
        LED_1,
    };

    const uint32_t *pin = pins;
    size_t i = ARRAY_SIZE(pins);
    while (i--) {
        nrf_gpio_cfg_output(*pin);
        nrf_gpio_pin_write(*pin, LEDS_ACTIVE_STATE);
        pin++;
    }
}

static inline void led_heartBeatStep(void *arg)
{
    nrf_gpio_pin_toggle(LED_1);
}

static inline void led_timerInit(void)
{
    APP_TIMER_DEF(timer);
    app_timer_create(&timer, APP_TIMER_MODE_REPEATED, led_heartBeatStep);
    app_timer_start(timer, APP_TIMER_TICKS(1000), NULL);
}

/* ****************************************************************************
 * --- Timer Routines ---
 ******************************************************************************/
static inline void timerInit(void)
{
    const ret_code_t rc = app_timer_init();
    ASSERT(NRFX_SUCCESS == rc);
    app_timer_resume();
}

/* ****************************************************************************
 * --- ---
 ******************************************************************************/
#if defined(APP_USBD_ENABLED) && APP_USBD_ENABLED
#include "app_usbd_serial_num.h"
#include "app_usbd.h"

static volatile bool m_usb_started;

static void usbd_user_evt_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_DRV_SUSPEND:
            break;
        case APP_USBD_EVT_DRV_RESUME:
            break;
        case APP_USBD_EVT_STARTED:
            m_usb_started = true;
            break;
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            break;
        case APP_USBD_EVT_POWER_DETECTED:
            NRF_LOG_INFO("USB power detected");

            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            NRF_LOG_INFO("USB power removed");
            app_usbd_stop();
            break;
        case APP_USBD_EVT_POWER_READY:
            NRF_LOG_INFO("USB ready");
            app_usbd_start();
            break;
        default:
            break;
    }
}

static void usbd_init(void)
{
    app_usbd_serial_num_generate();
    static const app_usbd_config_t usbd_config = {
        .ev_state_proc = usbd_user_evt_handler
    };
    APP_ERROR_CHECK(app_usbd_init(&usbd_config));
}

static void usbd_enable(void)
{
    APP_ERROR_CHECK(app_usbd_power_events_enable());

    /* Process USB events until USB is started. This is related to the fact that
     * current version of softdevice does not handle USB POWER events. */
    while (m_usb_started == false)
    {
        while (app_usbd_event_queue_process())
        {
            /* Nothing to do */
        }
    }

}
#endif //APP_USBD_ENABLED


typedef struct __attribute__((packed))
{
        uint32_t    magic_number;               /* Magic number to verify the presence of this structure in memory */
        uint32_t    struct_version     : 8;     /* Version of this struct format */
        uint32_t    rfu0               : 24;    /* Reserved for future use, shall be 0xFFFFFF */
        uint32_t    revision_hash;              /* Unique revision identifier */
        uint32_t    version_major      : 8;     /* Major version number */
        uint32_t    version_minor      : 8;     /* Minor version number */
        uint32_t    version_patch      : 8;     /* Patch version number */
        uint32_t    rfu1               : 8;     /* Reserved for future use, shall be 0xFF */
        uint32_t    sd_ble_api_version : 8;     /* SoftDevice BLE API version number */
        uint32_t    transport_type     : 8;     /* Connectivity transport type, 1 = UART HCI */
        uint32_t    rfu2               : 16;    /* Reserved for future use, shall be 0xFFFF */
        uint32_t    baud_rate;                  /* UART transport baud rate */
} version_info_t;
#if defined ( __CC_ARM )
static const version_info_t version_info __attribute__((at(0x50000))) =
#elif defined ( __GNUC__ ) || defined ( __SES_ARM )
volatile static const version_info_t version_info  __attribute__ ((section(".connectivity_version_info"))) =
#elif defined ( __ICCARM__ )
__root    const version_info_t version_info @ 0x50000 =
#endif
{
    .magic_number       = 0x46D8A517,
    .struct_version     = 2,
    .rfu0               = 0xFFFFFF,
    .revision_hash      = 0,
    .version_major      = APP_VERSION_MAJOR,
    .version_minor      = APP_VERSION_MINOR,
    .version_patch      = APP_VERSION_PATCH,
    .rfu1               = 0xFF,
    .sd_ble_api_version = NRF_SD_BLE_API_VERSION,
    .transport_type     = 1,
    .rfu2               = 0xFFFF,
    .baud_rate          = SER_PHY_UART_BAUDRATE_VAL,
};

static void on_idle(void)
{

    if (!NRF_LOG_PROCESS())
    {
      // Wait for an event.
      if (nrf_sdh_is_enabled())
      {
          ret_code_t ret_code = sd_app_evt_wait();
          ASSERT((ret_code == NRF_SUCCESS) || (ret_code == NRF_ERROR_SOFTDEVICE_NOT_ENABLED));
          UNUSED_VARIABLE(ret_code);
      }
      else
      {
          // Wait for an event.
          __WFE();
          // Clear the internal event register.
          __SEV();
          __WFE();
      }
    }

#if defined(APP_USBD_ENABLED) && APP_USBD_ENABLED

    while (app_usbd_event_queue_process())
    {
        /* Nothing to do */
    }
#endif
}

uint32_t timestamp(void)
{
  return DWT->CYCCNT;
}

#if BOARD_DETECTION
void pin_to_default(uint8_t pin)
{
    if (pin != NRF_QSPI_PIN_NOT_CONNECTED)
    {
        nrf_gpio_cfg_default(pin);
    }
}
#endif

/**@brief Main function of the connectivity application. */
int main(void)
{


    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    uint32_t err_code = NRF_SUCCESS;

    APP_ERROR_CHECK(NRF_LOG_INIT(timestamp, 64000000));


    NRF_LOG_DEFAULT_BACKENDS_INIT();

    NRF_LOG_INFO_FLUSH("BLE connectivity started");

    timerInit();
    led_Gpioinit();
    led_timerInit();
    nrf21540_initialize();
    nrfSdhBle_registerEnableObserver(nrf21540_onBleStackEnabled);

#if BOARD_DETECTION
    uint32_t pin_reset = PCA10059_PIN_RESET;

    nrf_drv_qspi_config_t config = NRF_DRV_QSPI_DEFAULT_CONFIG;

    //Using qspi memory to de
    err_code = nrf_drv_qspi_init(&config, NULL, NULL);
    if (err_code == NRF_SUCCESS)
    {
        //qspi memory is present on pca10056
        pin_reset = PCA10056_PIN_RESET;
        nrf_drv_qspi_uninit();
    }
    else if (err_code == NRF_ERROR_TIMEOUT)
    {
        //pca10059 assumed when no qspi memory
        nrf_drv_qspi_uninit();
        pin_reset = PCA10059_PIN_RESET;
        pin_to_default(config.pins.csn_pin);
        pin_to_default(config.pins.io0_pin);
        pin_to_default(config.pins.io1_pin);
        pin_to_default(config.pins.io2_pin);
        pin_to_default(config.pins.io3_pin);
        pin_to_default(config.pins.sck_pin);
    }
#endif //BOARD_DETECTION

#if (defined(SER_PHY_HCI_DEBUG_ENABLE) || defined(SER_PHY_DEBUG_APP_ENABLE))
    debug_init(NULL);
#endif

    /* Force constant latency mode to control SPI slave timing */
    NRF_POWER->TASKS_CONSTLAT = 1;

    /* Initialize scheduler queue. */
    //lint -save -e666 -e587
    APP_SCHED_INIT(SER_CONN_SCHED_MAX_EVENT_DATA_SIZE, SER_CONN_SCHED_QUEUE_SIZE);
    //lint -restore

    /* Initialize SoftDevice.
     * SoftDevice Event IRQ is not scheduled but immediately copies BLE events to the application
     * scheduler queue */

    err_code = nrf_drv_clock_init();
    if ((err_code != NRF_SUCCESS) &&
        (err_code != NRF_ERROR_MODULE_ALREADY_INITIALIZED))
    {
        APP_ERROR_CHECK(err_code);
    }

    nrf_drv_clock_hfclk_request(NULL);
    nrf_drv_clock_lfclk_request(NULL);

    while (!nrf_drv_clock_hfclk_is_running())
    {}

#if defined(APP_USBD_ENABLED) && APP_USBD_ENABLED
    usbd_init();
#endif

#if BOARD_DETECTION
        APP_ERROR_CHECK(nrf_dfu_trigger_usb_init(pin_reset));
#endif

    /* Open serialization HAL Transport layer and subscribe for HAL Transport events. */
    err_code = ser_hal_transport_open(ser_conn_hal_transport_event_handle);
    APP_ERROR_CHECK(err_code);

#if defined(APP_USBD_ENABLED) && APP_USBD_ENABLED
    NRF_LOG_INFO_FLUSH("Awaiting USB connection...");
    usbd_enable(); /*Spin loop until a USB is plugged into nRF-USB connector*/
    NRF_LOG_INFO_FLUSH("USB connected");
#endif

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    ser_conn_on_no_mem_handler_set(on_idle);

    /* Enter main loop. */
    for (;;)
    {
        /* Process SoftDevice events. */
        app_sched_execute();

        CRITICAL_REGION_ENTER();
        if (nrf_sdh_is_suspended())
        {
            // Resume pulling new events if queue utilization drops below 50%.
            if (app_sched_queue_space_get() > (SER_CONN_SCHED_QUEUE_SIZE >> 1))
            {
                nrf_sdh_resume();
            }
        }
        CRITICAL_REGION_EXIT();

        /* Process received packets.
         * We can NOT add received packets as events to the application scheduler queue because
         * received packets have to be processed before SoftDevice events but the scheduler queue
         * does not have priorities. */
        err_code = ser_conn_rx_process();
        APP_ERROR_CHECK(err_code);

        on_idle();

    }
}
/** @} */
