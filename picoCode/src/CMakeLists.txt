add_executable(mqttmain 
	mqttmain.cpp 
	lcd_printer.cpp
       	sensor_functions.cpp 
	mqtt_functions.cpp
	melody.cpp	
)
#add_executable(mqttmain test_ntp.cpp)

set_target_properties(mqttmain PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(mqttmain
        pico_stdlib
        pico_cyw43_arch_lwip_threadsafe_background
	hardware_gpio
	pico_lwip_mqtt
	hardware_i2c
	hardware_pwm
)

target_include_directories(mqttmain PUBLIC ${CMAKE_SOURCE_DIR}/include)

pico_enable_stdio_uart(mqttmain 1)
pico_enable_stdio_usb(mqttmain 1)

pico_add_extra_outputs(mqttmain)

add_custom_command(
    TARGET mqttmain
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/src/mqttmain.uf2" 
    	    "${CMAKE_BINARY_DIR}/bin/mqttmain.uf2"
    )

