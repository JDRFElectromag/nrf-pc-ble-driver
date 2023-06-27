# nrf-pc-ble-driver

This is an experimental repo. It provides modified build system for [nrf-pc-ble-driver][nrf-pc-ble-driver].

# How to Use
1. Clone [nrf-pc-ble-driver][nrf-pc-ble-driver]
    - Lets call the root directory `nrf-pc-ble-driver-root`
2. Installing dependencies as per [Installing on Ubuntu Linux](nRF-README.md)
3. Additional installation dependencies
    ```
    export GCCARMEMB_TOOLCHAIN_PATH=${GNUARMEMB_TOOLCHAIN_PATH}
    ```
4. Connectivity Firmware Project Generation
    - Replace `<nrf-pc-ble-driver-root>` with path from step 1.
    ```
    mkdir build
    cd build
    cmake -G Ninja -DCOMPILE_CONNECTIVITY=1 -DCONNECTIVITY_VERSION=1.2.3 -DDISABLE_TESTS=1 -DARCH=arm -DSDK_V15_PATH=<nrf-pc-ble-driver-root> ..
    ```
5. Connectivity Firmware Compilation
    ```
    cmake --build . --target compile_connectivity
    ```
6. Use your favorite processes to flash the hex file below. RTT may be observed
    ```
    <nrf-pc-ble-driver-root>/hex/sd_api_v6/connectivity_1.2.3_usb_with_s140_6.1.1.hex
    ```

[nrf-pc-ble-driver]: https://github.com/JDRFElectromag/nrf-pc-ble-driver
