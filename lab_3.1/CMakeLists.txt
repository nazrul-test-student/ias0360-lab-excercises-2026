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
set(PICO_BOARD pico_w)

set(PROGRAM_NAME main)

cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
project(${PROGRAM_NAME} C CXX ASM)
set(PICO_CYW43_ARCH lwip_sys_freertos CACHE STRING "" FORCE)

pico_sdk_init()

# --- Libraries ---
add_subdirectory(lib/http_client)
add_subdirectory(lib/config)
add_subdirectory(lib/lcd)
add_subdirectory(lib/font)
add_subdirectory(lib/context)

include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/lib/config
    ${CMAKE_CURRENT_LIST_DIR}/lib/lcd
    ${CMAKE_CURRENT_LIST_DIR}/lib/font
    ${CMAKE_CURRENT_LIST_DIR}/lib/context
)

# --- Target: networking_main ---
add_executable(networking_main networking_main.cpp)

target_include_directories(networking_main
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

set_target_properties(networking_main PROPERTIES
    COMPILE_FLAGS "-fno-rtti -fno-exceptions -fno-threadsafe-statics -nostdlib"
)

target_link_libraries(networking_main
    http_client
    pico_stdlib
    hardware_spi
    hardware_pwm
    hardware_adc
    pico_lwip_http
    pico_lwip_mbedtls
    pico_mbedtls
    pico_lwip_sntp
)

set_property(TARGET networking_main APPEND_STRING PROPERTY LINK_FLAGS
    "-Wl,--print-memory-usage"
)

pico_enable_stdio_usb(networking_main 1)
pico_enable_stdio_uart(networking_main 0)
pico_add_extra_outputs(networking_main)

# --- Target: application1 (digit recognition, no display) ---
add_executable(application1 application1.cpp)

target_include_directories(application1
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

set_target_properties(application1 PROPERTIES
    COMPILE_FLAGS "-fno-rtti -fno-exceptions -fno-threadsafe-statics -nostdlib"
)

pico_set_linker_script(application1 ${CMAKE_CURRENT_SOURCE_DIR}/ldscripts/application1_memmap.ld)

target_link_libraries(application1
    lcd
    font
    config
    context
    pico_stdlib
    hardware_spi 
    hardware_pwm 
    pico_multicore 
    hardware_adc
)

set_property(TARGET application1 APPEND_STRING PROPERTY LINK_FLAGS
    "-Wl,--print-memory-usage"
)

pico_enable_stdio_usb(application1 1)
pico_enable_stdio_uart(application1 0)
pico_add_extra_outputs(application1)

# --- Target: application2 (letter recognition, LCD + networking) ---
add_executable(application2 application2.cpp)

target_include_directories(application2
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
)

set_target_properties(application2 PROPERTIES
    COMPILE_FLAGS "-fno-rtti -fno-exceptions -fno-threadsafe-statics -nostdlib"
)

pico_set_linker_script(application2 ${CMAKE_CURRENT_SOURCE_DIR}/ldscripts/application2_memmap.ld)

target_link_libraries(application2
    lcd
    font
    config
    context
    pico_stdlib
    hardware_spi 
    hardware_pwm 
    pico_multicore 
    hardware_adc
)

set_property(TARGET application2 APPEND_STRING PROPERTY LINK_FLAGS
    "-Wl,--print-memory-usage"
)

pico_enable_stdio_usb(application2 1)
pico_enable_stdio_uart(application2 0)
pico_add_extra_outputs(application2)
