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
set(PICO_BOARD pico_w CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
project(pico-10dof-imu_project)
pico_sdk_init()
add_subdirectory(icm20948)
include_directories(./icm20948)
add_executable(imu
main.c
)
pico_enable_stdio_usb(imu 1)
pico_enable_stdio_uart(imu 1)

pico_add_extra_outputs(imu)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(imu pico_stdlib icm20948 hardware_i2c pico_multicore)
