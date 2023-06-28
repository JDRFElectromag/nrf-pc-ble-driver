#include <stdint.h>

#include "boards.h"
#include "ble.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_log.h"

#include "nrf21540Configs.h"
#include "nrf21540.h"

/* ****************************************************************************
 * --- Private routines ---
 ******************************************************************************/
static inline void initGpios(void)
{
    /*Provided by pca10112.h*/
    nrf_gpio_cfg_output(NRF21540_PDN_PIN);
    nrf_gpio_cfg_output(NRF21540_RXEN_PIN);
    nrf_gpio_cfg_output(NRF21540_TXEN_PIN);
    nrf_gpio_cfg_output(NRF21540_MODE_PIN);
    nrf_gpio_cfg_output(NRF21540_ANTSEL_PIN);
}

static const ble_common_opt_pa_lna_t *getPaLnaConfigs(void)
{
    static const ble_common_opt_pa_lna_t params = {
        .pa_cfg =
        {
            .enable = 1,
            .active_high = 1,
            .gpio_pin = NRF21540_TXEN_PIN
        },
        .lna_cfg =
        {
            .enable = 1,
            .active_high = 1,
            .gpio_pin = NRF21540_RXEN_PIN
        },
        .ppi_ch_id_set = NRF21540_PIN_SET_PPI_CHANNEL,
        .ppi_ch_id_clr = NRF21540_PIN_CLEAR_PPI_CHANNEL,
        .gpiote_ch_id = NRF21540_GPIOTE_CHANNEL
    };

    return &params;
}

/* ****************************************************************************
 * --- Public API ---
 ******************************************************************************/
void nrf21540_setPowerMode(nrf21540_powerMode_t mode)
{
    /* Currently only supports default power levels.
     * POUTA_PROD=20dB --> Set logic level low.
     * POUTB_PROD=10dB --> Set logic level high.*/
    nrf_gpio_pin_write(NRF21540_MODE_PIN, NRF21540_POWER_10dBm == mode);
}

void nrf21540_setAntenna(nrf21540_antenna_t antenna)
{
    nrf_gpio_pin_write(NRF21540_ANTSEL_PIN, NRF21540_ANTENNA_2 == antenna);
}

void nrf21540_enablePower(bool isEnabled)
{
    nrf_gpio_pin_write(NRF21540_PDN_PIN, isEnabled);
    /*Wait for it to complete*/
    const uint32_t completeDelay = isEnabled ?
                                   NRF21540_PD_TO_PG_MAX_SETUP_TIME_US :
                                   NRF21540_ANY_TO_PD_MAX_SETUP_TIME_US;
    nrf_delay_us(completeDelay);
}

void nrf21540_bleAttach(void)
{
    const ble_opt_t opt = {
        .common_opt.pa_lna = *getPaLnaConfigs(),
    };
    const uint32_t rc = sd_ble_opt_set(BLE_COMMON_OPT_PA_LNA, &opt);
    NRF_LOG_INFO("%s[%u]: rc:0x%x",  __FUNCTION__, __LINE__, rc);
    ASSERT(NRF_SUCCESS == rc);
}

void nrf21540_init(void)
{
    /* On POR the power pin is floating
     * on the PCA10112 (tristate + no pulldown).
     * To produce known behavior always
     * explicitly power it down/up.*/
    initGpios();
    nrf21540_enablePower(false);
    nrf21540_setPowerMode(NRF21540_POWER_20dBm);
    nrf21540_setAntenna(NRF21540_ANTENNA_1);
}
