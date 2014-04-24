TARGET_CHIP     := NRF51822_QFAA_CA
BOARD           := BOARD_PCA10001
DEVICE_VARIANT  := xxaa
USE_SOFTDEVICE  := S110
OUTPUT_FILENAME := ble_hrm

# Additional Source Paths
C_SOURCE_PATHS += boards/

# Project Source Files
C_SOURCE_FILES += main.c
C_SOURCE_FILES += heart_rate.c
C_SOURCE_FILES += btle.c
C_SOURCE_FILES += btle_gap.c
C_SOURCE_FILES += btle_advertising.c
C_SOURCE_FILES += printf_retarget.c
C_SOURCE_FILES += stdio.c
C_SOURCE_FILES += board_pca10001.c

# [SDK]/app_common
C_SOURCE_FILES += app_fifo.c
C_SOURCE_FILES += app_gpiote.c
C_SOURCE_FILES += app_timer.c
C_SOURCE_FILES += app_button.c
C_SOURCE_FILES += app_uart_fifo.c

# [SDK]/ble and [SDK]/ble/ble_services
C_SOURCE_FILES += softdevice_handler.c
C_SOURCE_FILES += ble_advdata.c
C_SOURCE_FILES += ble_bondmngr.c
C_SOURCE_FILES += ble_conn_params.c
C_SOURCE_FILES += ble_flash.c
C_SOURCE_FILES += pstorage.c
C_SOURCE_FILES += crc16.c
C_SOURCE_FILES += ble_srv_common.c
C_SOURCE_FILES += ble_hrs.c

# [SDK]/simple_uart
C_SOURCE_FILES += simple_uart.c

# [SDK]/nrf_delay
C_SOURCE_FILES += nrf_delay.c

# Make sure we're pointing to the right SDK version
ROOT_PATH = ../../
SDK_PATH = $(ROOT_PATH)lib/sdk/nRF51_SDK_v5.1.0.36092/Nordic/nrf51822/

# Include the correct SoftDevice header files
INCLUDEPATHS += -I"$(ROOT_PATH)lib/softdevice/s110_nrf51822_6.0.0/s110_nrf51822_6.0.0_API/include"

# Local and SDK include paths
INCLUDEPATHS += -I"."
INCLUDEPATHS += -I"../common"
INCLUDEPATHS += -I"boards"
INCLUDEPATHS += -I"$(SDK_PATH)Include/app_common"
INCLUDEPATHS += -I"$(SDK_PATH)Include/ble"
INCLUDEPATHS += -I"$(SDK_PATH)Include/ble/ble_services"
INCLUDEPATHS += -I"$(SDK_PATH)Include/simple_uart"
INCLUDEPATHS += -I"$(SDK_PATH)Include/sd_common"

# Additional Flags
CFLAGS += -DBLE_STACK_SUPPORT_REQD

# Add in the global Makefile
include $(SDK_PATH)Source/templates/gcc/Makefile.common
