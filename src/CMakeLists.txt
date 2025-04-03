add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} pico_stdlib pico_cyw43_arch_lwip_threadsafe_background)

pico_add_extra_outputs(${PROJECT_NAME})

#för att få uf2 i bin
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/lib/${PROJECT_NAME}.uf2
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.uf2
)

message(STATUS "Adding extra outputs for ${PROJECT_NAME}")


pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 1)


target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/include)
