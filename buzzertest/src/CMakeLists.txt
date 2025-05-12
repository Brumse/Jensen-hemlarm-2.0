add_executable(buzzer main.cpp)

set_target_properties(buzzer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(buzzer
        pico_stdlib
        hardware_gpio
        hardware_pwm
)

target_include_directories(buzzer PUBLIC ${CMAKE_SOURCE_DIR}/include)

pico_enable_stdio_uart(buzzer 1)
pico_enable_stdio_usb(buzzer 1)

pico_add_extra_outputs(buzzer)

add_custom_command(
    TARGET buzzer 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/src/buzzer.uf2" 
            "${CMAKE_BINARY_DIR}/bin/buzzer.uf2"
    )

