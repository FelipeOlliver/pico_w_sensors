# Define o nome da nossa biblioteca
add_library(gfx
    gfx.c
)

# Garante que os includes funcionem corretamente
target_include_directories(gfx PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(gfx PUBLIC pico_stdlib hardware_dma ili9341)