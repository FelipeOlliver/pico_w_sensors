# CMakeLists.txt (RAIZ DO PROJETO)
cmake_minimum_required(VERSION 3.13)

# Define o caminho para o Kernel do FreeRTOS que está DENTRO do SDK
set(FREERTOS_KERNEL_PATH ${PICO_SDK_PATH}/lib/FreeRTOS-Kernel)

set(PICO_BOARD pico_w)
include(pico_sdk_import.cmake)

# --- CORREÇÃO: Usa o caminho completo para o script de importação oficial ---
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)
# --- FIM DA CORREÇÃO ---

project(multifunctional_datalogger C CXX ASM)
pico_sdk_init()

# Força todos os executáveis a serem gerados na pasta raiz do build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Informa ao CMake para procurar e executar os CMakeLists.txt das nossas bibliotecas e do nosso código-fonte
add_subdirectory(lib/FatFs_SPI)
add_subdirectory(src)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)