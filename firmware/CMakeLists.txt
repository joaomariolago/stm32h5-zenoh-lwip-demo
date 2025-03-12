cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME firmware)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc"
)
target_compile_options(freertos_config INTERFACE -Wno-error)

# CubeMX is the one providing the FreeRTOS kernel
add_library(freertos_kernel ALIAS stm32cubemx)

# Get include directories provided by STM32CubeMX
get_target_property(CUBE_MX_PROVIDED_INCLUDE_DIRS stm32cubemx INTERFACE_INCLUDE_DIRECTORIES)

# Third Party libraries

# - Lightweight IP (lwIP)

set(LWIP_SPECIFIC_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/Lib/stm32-mw-lwip/src/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/Lib/stm32-mw-lwip/system"
    "${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc/LWIP/Target"
)

set(LWIP_INCLUDE_DIRS
    ${LWIP_SPECIFIC_INCLUDE_DIRS}
    ${CUBE_MX_PROVIDED_INCLUDE_DIRS}
)

add_library(lwip-system INTERFACE)

target_include_directories(lwip-system INTERFACE
  ${LWIP_SPECIFIC_INCLUDE_DIRS}
  ${CUBE_MX_PROVIDED_INCLUDE_DIRS}
)

target_sources(lwip-system  INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/Lib/stm32-mw-lwip/system/OS/sys_arch.c"
)

add_subdirectory(Lib/stm32-mw-lwip)

# - BSP LAN8742A

add_library(lan8742 INTERFACE)

set(LAN8742_SPECIFIC_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/Lib/stm32-lan8742"
)

target_include_directories(lan8742 INTERFACE
    ${LAN8742_SPECIFIC_INCLUDE_DIRS}
    ${CUBE_MX_PROVIDED_INCLUDE_DIRS}
)

target_sources(lan8742 INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/Lib/stm32-lan8742/lan8742.c"
)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# Create binary and hex files
add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating binary and hex file"
)
