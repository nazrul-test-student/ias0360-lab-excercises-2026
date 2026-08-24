cmake_minimum_required(VERSION 3.12)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(pico_sdk_import.cmake)
# project(pico-tflite-inference-test)
project(pico_tflite_inference_test C CXX ASM)
# initialize the Pico SDK
pico_sdk_init()


add_executable(main main.cpp) # main function to run.
add_executable(main_arena_size_test arena_size_test.cpp) # to check arena size for a given model.
add_executable(main_inference_test inference_test.cpp) # to test hand written digit recognition with static test data.

target_include_directories(main
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/.
)

target_include_directories(main_arena_size_test
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/.
)

target_include_directories(main_inference_test
  PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/.
)


set_target_properties(
  main
  PROPERTIES
  COMPILE_FLAGS -fno-rtti
  COMPILE_FLAGS -fno-exceptions
  COMPILE_FLAGS -fno-threadsafe-statics
  COMPILE_FLAGS -nostdlib
)

set_target_properties(
  main_arena_size_test
  PROPERTIES
  COMPILE_FLAGS -fno-rtti
  COMPILE_FLAGS -fno-exceptions
  COMPILE_FLAGS -fno-threadsafe-statics
  COMPILE_FLAGS -nostdlib
)

set_target_properties(
  main_inference_test
  PROPERTIES
  COMPILE_FLAGS -fno-rtti
  COMPILE_FLAGS -fno-exceptions
  COMPILE_FLAGS -fno-threadsafe-statics
  COMPILE_FLAGS -nostdlib
)


add_subdirectory(src)
add_subdirectory(models)
add_subdirectory(lib/pico-tflmicro)
add_subdirectory(lib/config)
add_subdirectory(lib/lcd)
add_subdirectory(lib/font)

include_directories(./src)
include_directories(./models)
include_directories(./lib/pico-tflmicro)
include_directories(./lib/config)
include_directories(./lib/lcd)
include_directories(./lib/font)

target_link_libraries(
  main 
  src
  lcd 
  font 
  config
  models
  pico_stdlib
  hardware_spi 
  hardware_pwm 
  pico_multicore 
  hardware_adc
  pico-tflmicro
)

target_link_libraries(
  main_arena_size_test 
  src
  config
  models
  pico_stdlib
  hardware_spi 
  hardware_pwm 
  pico_multicore 
  hardware_adc
  pico-tflmicro
)

target_link_libraries(
  main_inference_test 
  src
  config
  models
  pico_stdlib
  hardware_spi 
  hardware_pwm 
  pico_multicore 
  hardware_adc
  pico-tflmicro
)

pico_set_program_flash(program1 0x140000)
pico_set_program_flash(program2 0x180000)

# enable usb and uart output
pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 1)

pico_enable_stdio_usb(main_arena_size_test 1)
pico_enable_stdio_uart(main_arena_size_test 1)


pico_enable_stdio_usb(main_inference_test 1)
pico_enable_stdio_uart(main_inference_test 1)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(main)

pico_add_extra_outputs(main_arena_size_test)

pico_add_extra_outputs(main_inference_test)
