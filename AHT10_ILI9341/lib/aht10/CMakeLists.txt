add_library(aht10
    aht10.c
)

target_include_directories(aht10
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}
)

# A biblioteca aht10 precisa das funções de I2C do SDK
target_link_libraries(aht10
    pico_stdlib
    hardware_i2c
)