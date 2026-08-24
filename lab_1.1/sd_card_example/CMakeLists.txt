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
cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w CACHE STRING "Board type")
set(PROGRAM_NAME sd_card_example)

include(pico_sdk_import.cmake)

project(${PROGRAM_NAME} C CXX ASM)


# Initialise the Pico SDK
pico_sdk_init()

add_subdirectory(sd_card_driver build)

# Add executable. Default name is the project name, version 0.1
add_executable(${PROGRAM_NAME} 
    config/hw_config.c
    main.cpp 
)

# https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf
target_compile_definitions(${PROGRAM_NAME} PRIVATE
  PICO_STACK_SIZE=0x1000
  PICO_CORE1_STACK_SIZE=0x800
)

target_compile_options(${PROGRAM_NAME} PUBLIC 
  -Wall 
  -Wextra 
  -Wshadow 
  -Wstack-usage=2048 
  -fanalyzer 
)

add_compile_definitions(
    PARAM_ASSERTIONS_ENABLE_ALL=1 
    PICO_MALLOC_PANIC=1
    PICO_USE_STACK_GUARDS=1
)


set_property(TARGET ${PROGRAM_NAME} APPEND_STRING PROPERTY LINK_FLAGS 
  "-Wl,--print-memory-usage"
)

pico_set_program_name(${PROGRAM_NAME} "${PROGRAM_NAME}")
pico_set_program_version(${PROGRAM_NAME} "0.1")

pico_enable_stdio_uart(${PROGRAM_NAME} 0)
pico_enable_stdio_usb(${PROGRAM_NAME} 1)

target_include_directories(${PROGRAM_NAME} PUBLIC 
        include/ 
)
target_link_libraries(${PROGRAM_NAME}
    pico_stdlib
    hardware_i2c
    pico_multicore
    pico_util
    sd_custom_driver
    hardware_clocks
    hardware_adc
    hardware_pio
)

# Add the standard include files to the build
target_include_directories(${PROGRAM_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(${PROGRAM_NAME})


