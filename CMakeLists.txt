cmake_minimum_required(VERSION 3.17)
set(PROJECT_NAME my_pico_w_project)




set(PICO_SDK_PATH "${CMAKE_SOURCE_DIR}/../pico-sdk")
set(ENV{PICO_BOARD} pico_w)
include(pico_sdk_import.cmake)

project(${PROJECT_NAME} C CXX ASM)

pico_sdk_init()


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

message(STATUS "CMAKE_C_COMPILER: ${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_ASM_COMPILER: ${CMAKE_ASM_COMPILER}")

add_subdirectory(src)
# add_subdirectory(test) # Uncomment when adding tests
