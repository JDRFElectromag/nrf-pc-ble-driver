#pragma once

/**
 * State transition timing over GPIO.
 * See `nRF21540_PS_v1.2`::`5.3 State transition timing`.*/
#define NRF21540_TRX_TO_PG_MAX_SETUP_TIME_US                     (3)  /*TX/RX to PG*/
#define NRF21540_PG_TO_TRX_MAX_SETUP_TIME_US                     (11) /*PG to TX/RX. CEILING(10.5)*/
#define NRF21540_PD_TO_PG_MAX_SETUP_TIME_US                      (18) /*CEILING(17.5)*/
#define NRF21540_ANY_TO_PD_MAX_SETUP_TIME_US                     (10)

/* Uses by `MESH_LNA_SETUP_TIME_US`/`MESH_PA_SETUP_TIME_US`.
 * `MESH_XXX_SETUP_TIME_US` cannot be provided via `app_config.h`.
 * `MESH_XXX_SETUP_TIME_US` is provided by `CMAKE::add_nrf21540()`.*/
#define NRF21540_TRANSCEIVER_DIRECTION_UPDATE_MAX_SETUP_TIME_US  (NRF21540_TRX_TO_PG_MAX_SETUP_TIME_US + NRF21540_PG_TO_TRX_MAX_SETUP_TIME_US)

/**
 * The GPIOTE and PPI channels.*/
#ifndef NRF21540_GPIOTE_CHANNEL
    #define NRF21540_GPIOTE_CHANNEL         (0)
#endif
#ifndef NRF21540_PIN_SET_PPI_CHANNEL
    #define NRF21540_PIN_SET_PPI_CHANNEL    (0)
#endif
#ifndef NRF21540_PIN_CLEAR_PPI_CHANNEL
    #define NRF21540_PIN_CLEAR_PPI_CHANNEL  (1)
#endif
