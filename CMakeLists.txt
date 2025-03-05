set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PICO_BOARD pico_w) # Set the pico board type to pico_w

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

cmake_minimum_required(VERSION 3.13)

SET(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS-Kernel)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(HandWatch C CXX ASM)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

set(PICO_CXX_ENABLE_RTTI 1)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
include_directories(
        lib/no-OS-FatFS-SD-SPI-RPi-Pico/include
        include
        lib/
        lib/FreeRTOS-Kernel/include
        lib/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/include
        lib/tiny-json/
        lib/json-maker/src/include
        lib/Pico-I2C-LCD/
        lib/picomp3lib/src
        lib/picomp3lib/interface
        lib/Pico-libs/Battery Check/
        lib/Pico-libs/Button/
        lib/Pico-libs/PWM DMA Audio
        lib/Pico-libs/Keypad Matrix
        lib/libwave/
        lib/pico-ds1302/
        lib/ADXL345/
)

file(GLOB SOURCES_C "./source/*.c")
file(GLOB SOURCES_CXX "./source/*.cpp")
file (GLOB FreeRTOS_source "./lib/FreeRTOS-Kernel/*.c")
add_executable(HandWatch 
  ${SOURCES_CXX}
  ${SOURCES_C}
  ${FreeRTOS_source}
  lib/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/port.c
  lib/json-maker/src/json-maker.c
  lib/tiny-json/tiny-json.c
  lib/picomp3lib/interface/music_file.c
  lib/Pico-I2C-LCD/LCD_I2C.cpp
  lib/ADXL345/ADXL345.cpp
  lib/Pico-I2C-LCD/LCD_I2C.cpp
)

add_subdirectory(lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)

add_subdirectory(lib/json-maker/src)

add_subdirectory(lib/picomp3lib/src)

add_subdirectory(lib/libwave/)

add_subdirectory(lib/pico-ds1302/)

add_subdirectory("lib/Pico-libs/Battery Check")
add_subdirectory("lib/Pico-libs/Button/")
add_subdirectory("lib/Pico-libs/PWM DMA Audio")
add_subdirectory("lib/Pico-libs/Keypad Matrix")

# Set program info
pico_set_program_name(HandWatch "HandWatch")
pico_set_program_version(HandWatch "0.1")
pico_set_program_url(HandWatch "https://github.com/BambukoDev/HandWatch")
pico_set_program_description(HandWatch "Copyright (c) 2025 Adam Jamrozowicz")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(HandWatch 0)
pico_enable_stdio_usb(HandWatch 1)

target_compile_definitions(HandWatch
  PUBLIC PICO_MAX_SHARED_IRQ_HANDLERS=20
)

# Add any user requested libraries
target_link_libraries(HandWatch 
        pico_stdlib
        pico_multicore
        pico_btstack_ble
        pico_btstack_cyw43
        pico_cyw43_arch_none

        hardware_exception
        hardware_spi
        hardware_i2c
        hardware_adc
        hardware_rtc
        hardware_pio
        hardware_timer
        hardware_clocks
        hardware_pwm
        
        FatFs_SPI
        picomp3lib
        FreeRTOS-Kernel-Heap4
        wave
        ds1302pico
)

pico_btstack_make_gatt_header(HandWatch PRIVATE "${CMAKE_CURRENT_LIST_DIR}/ble.gatt")

pico_add_extra_outputs(HandWatch)