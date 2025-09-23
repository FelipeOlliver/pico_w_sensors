# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(gy_neo6mv2_gps C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(gy_neo6mv2_gps gy_neo6mv2_gps.c )

pico_set_program_name(gy_neo6mv2_gps "gy_neo6mv2_gps")
pico_set_program_version(gy_neo6mv2_gps "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(gy_neo6mv2_gps 0)
pico_enable_stdio_usb(gy_neo6mv2_gps 1)

# Add the standard library to the build
target_link_libraries(gy_neo6mv2_gps
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(gy_neo6mv2_gps PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(gy_neo6mv2_gps 
        pico_cyw43_arch_none
        hardware_uart
)

pico_add_extra_outputs(gy_neo6mv2_gps)

