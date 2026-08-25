# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

set(PROGRAM_NAME lcd_draw)

cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
project(Pico-ResTouch-X_X_code)
pico_sdk_init()

add_subdirectory(lib/config)
add_subdirectory(lib/lcd)
add_subdirectory(lib/font)

include_directories(./lib/config)
include_directories(./lib/lcd)
include_directories(./lib/font)

add_executable(${PROGRAM_NAME} 
main.c
)

# enable usb output, disable uart output
pico_enable_stdio_usb(${PROGRAM_NAME} 1)
pico_enable_stdio_uart(${PROGRAM_NAME} 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(${PROGRAM_NAME})

target_link_libraries(${PROGRAM_NAME} 
                        lcd 
                        font 
                        config 
                        pico_stdlib 
                        hardware_spi 
                        hardware_pwm
                        )
