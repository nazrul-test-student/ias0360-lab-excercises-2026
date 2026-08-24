cmake_minimum_required(VERSION 3.12)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(pic_debugging C CXX ASM)

pico_sdk_init()

add_executable(main main.c tight_loop_override.c)

target_include_directories(main PRIVATE ${CMAKE_CURRENT_LIST_DIR}/config)

# Debug-friendly build flags
target_compile_options(main PRIVATE -g -O0 -fno-rtti -fno-exceptions -fno-threadsafe-statics)

target_link_libraries(main pico_stdlib pico_cyw43_arch_lwip_threadsafe_background)

# Enable USB and UART output if needed
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 1)

# Create .uf2/.bin/.elf
pico_add_extra_outputs(main)