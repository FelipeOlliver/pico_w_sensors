# Define nossas bibliotecas customizadas que não têm seu próprio CMake
add_library(mfrc522_lib STATIC ../lib/mfrc522/mfrc522.c)
target_include_directories(mfrc522_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../lib/mfrc522)
target_link_libraries(mfrc522_lib pico_stdlib hardware_spi)

add_library(dht22_lib STATIC ../lib/dht22/dht.c)
target_include_directories(dht22_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../lib/dht22)
target_link_libraries(dht22_lib pico_stdlib hardware_gpio hardware_timer)

add_library(tcs34725_lib STATIC ../lib/tcs34725/tcs34725.c)
target_include_directories(tcs34725_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../lib/tcs34725)
target_link_libraries(tcs34725_lib pico_stdlib hardware_gpio hardware_i2c)

add_library(max30102_lib STATIC ../lib/max30102/max30102.c)
target_include_directories(max30102_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../lib/max30102)
target_link_libraries(max30102_lib pico_stdlib hardware_gpio hardware_i2c)


# Cria o executável final com nossos arquivos .c
add_executable(${PROJECT_NAME}
    multifunctional_datalogger.c
    hw_config.c
)

# Linka todas as bibliotecas que nosso programa precisa
target_link_libraries(${PROJECT_NAME}
    # SDK
    pico_stdlib
    hardware_spi
    hardware_pwm
    pico_sync
    hardware_i2c 
    hardware_timer
    pico_cyw43_arch_none
    
    # FreeRTOS
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
    
    # Nossas Bibliotecas
    FatFs_SPI              
    mfrc522_lib            
    dht22_lib
    tcs34725_lib
    max30102_lib              
)

# Adiciona o diretório de include do FreeRTOSConfig.h (que está na pasta pai "..")
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/.. 
)

# Gera os arquivos de saída (.uf2, etc.)
pico_add_extra_outputs(${PROJECT_NAME})