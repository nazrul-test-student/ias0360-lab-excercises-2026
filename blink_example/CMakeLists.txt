cmake_minimum_required(VERSION 3.13)

# Pico W (RP2040 + CYW43439 wireless chip). Must be set before
# include(pico_sdk_import.cmake) — the SDK defaults/caches the board as
# soon as that file is included, so setting it later has no effect.
set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(blink_example C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(blink
    blink.c
)

target_link_libraries(blink pico_stdlib pico_cyw43_arch_none)

# USB stdio also registers a reset interface, letting `picotool load -f`
# reboot the board into BOOTSEL over USB instead of the physical button.
pico_enable_stdio_usb(blink 1)
pico_enable_stdio_uart(blink 0)

pico_add_extra_outputs(blink)
