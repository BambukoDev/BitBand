set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

cmake_minimum_required(VERSION 3.13)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(HandWatch C CXX)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

SET(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeRTOS-Kernel)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# FILE(GLOB FreeRTOS_src submodules/FreeRTOS-Kernel/*.c)

# add_library( FreeRTOS STATIC
#   ${FreeRTOS_src}
#   submodules/FreeRTOS-Kernel/portable/GCC/ARM_CM0/port.c
#   submodules/FreeRTOS-Kernel/portable/MemMang/heap_4.c
# )

# target_include_directories( FreeRTOS PUBLIC
#   submodules/FreeRTOS-Kernel/include
#   submodules/FreeRTOS-Kernel/portable/GCC/ARM_CM0
# )

# Add executable. Default name is the project name, version 0.1
include_directories(
        ./lib/no-OS-FatFS-SD-SPI-RPi-Pico/include
        ./include
        ./lib/FreeRTOS-Kernel/include
        ./lib/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/include
)
file(GLOB SOURCES_C "./source/*.c")
file(GLOB SOURCES_CXX "./source/*.cpp")
add_executable(HandWatch 
  source/main.cpp
  source/LiquidCrystal_I2C.cpp
  source/hw_config.cpp
  source/Log.cpp
  source/ButtonInput.cpp
  lib/FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/port.c
)

add_subdirectory(lib/no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)

pico_set_program_name(HandWatch "HandWatch")
pico_set_program_version(HandWatch "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(HandWatch 0)
pico_enable_stdio_usb(HandWatch 1)

# Add any user requested libraries
target_link_libraries(HandWatch 
        pico_stdlib
        pico_multicore

        hardware_spi
        hardware_i2c
        FatFs_SPI
        FreeRTOS
)

pico_add_extra_outputs(HandWatch)