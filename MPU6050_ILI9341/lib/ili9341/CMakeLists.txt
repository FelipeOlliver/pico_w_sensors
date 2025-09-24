# Define o nome da nossa biblioteca
add_library(ili9341
    ili9341.c
)

# Garante que os includes funcionem corretamente
target_include_directories(ili9341 PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(ili9341 PUBLIC pico_stdlib hardware_spi hardware_dma)