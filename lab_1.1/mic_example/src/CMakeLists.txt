# No VSCode boilerplate, no pico_sdk_import, no project(), no pico_sdk_init() here

add_library(pico_pdm_microphone STATIC
    pdm_microphone.c
    OpenPDM2PCM/OpenPDMFilter.c
)

target_include_directories(pico_pdm_microphone PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# Generate PIO header for the library target
pico_generate_pio_header(pico_pdm_microphone
    ${CMAKE_CURRENT_LIST_DIR}/pdm_microphone.pio
)

# Link the hardware libs needed by the driver
target_link_libraries(pico_pdm_microphone PUBLIC
    pico_stdlib
    hardware_dma
    hardware_pio
)

# Remove this unless you actually have src/mic_example/
# add_subdirectory(mic_example)
