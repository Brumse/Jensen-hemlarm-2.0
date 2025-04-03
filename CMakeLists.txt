cmake_minimum_required(VERSION 3.17)
set(PROJECT_NAME my_pico_w_project)

#set(PICO_SDK_PATH "${CMAKE_SOURCE_DIR}/../pico-sdk")
set(ENV{PICO_BOARD} pico_w)

set(PICO_SDK_FETCH_FROM_GIT on)
include(pico_sdk_import.cmake)

project(${PROJECT_NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

include_directories(include)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

add_subdirectory(src)

# add_subdirectory(test) # Uncomment when adding tests

