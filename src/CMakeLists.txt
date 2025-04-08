add_executable(mqttmain mqttmain.cpp)

target_link_libraries(mqttmain
        pico_stdlib
        pico_cyw43_arch_lwip_threadsafe_background
	#add after mqtt tests are done!
	#hardware_gpio
	pico_lwip_mqtt
)

target_include_directories(mqttmain PUBLIC ${CMAKE_SOURCE_DIR}/include)

pico_enable_stdio_uart(mqttmain 1)
pico_enable_stdio_usb(mqttmain 1)

pico_add_extra_outputs(mqttmain)
