/**
 * A light weight driver for the nRF21540 (external PA/LNA).
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enable/Disable nRF21540.
 *        Blocks until complete.
 *        This may be called at anytime, worse case some packets are dropped.
 *
 * @param[in] isEnabled True enable power, false disable.
 */
void nrf21540_enablePower(bool isEnabled);

/**
 * @brief Set the power mode.
 *        nRF21540 can switch between two preprogrammed TX gains.
 *        The transceiver direction must change for this to take effect.
 *
 * @param[in] mode One of nrf21540_powerMode_t
 */
typedef enum {
    NRF21540_POWER_20dBm,
    NRF21540_POWER_10dBm
} nrf21540_powerMode_t;
void nrf21540_setPowerMode(nrf21540_powerMode_t mode);

/**
 * @brief Set the antenna
 *
 * @param[in] antenna One of nrf21540_antenna_t
 */
typedef enum {
    NRF21540_ANTENNA_1,
    NRF21540_ANTENNA_2
} nrf21540_antenna_t;
void nrf21540_setAntenna(nrf21540_antenna_t antenna);

/**
 * @brief Attach to the BLE (softdevice).
 *        Call after `nrf_sdh_ble_enable()`.
 *        Assert on all failure.
 */
void nrf21540_bleAttach(void);

/**
 * @brief Initialize the nrf21540.
 *        Assert on all failure.
 *        Call before `meshBleInit()`.
 *        Defaults
 *          - Powered up.
 *          - NRF21540_POWER_20dBm
 *          - NRF21540_ANTENNA_1.
 *          - NRF21540_TRNSCVR_DIR_DISABLE.
 */
void nrf21540_init(void);

#ifdef __cplusplus
}
#endif
