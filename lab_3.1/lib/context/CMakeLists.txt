aux_source_directory(. DIR_context)

add_library(context context.c)
target_include_directories(context PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(context pico_stdlib cmsis_core hardware_flash)