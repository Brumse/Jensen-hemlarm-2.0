add_executable(mqtt_to_api main.cpp)

set_target_properties(mqtt_to_api PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(mqtt_to_api
  ${MOSQUITTO_LIBRARIES}
  ${JANSSON_LIBRARIES}
  ${CURL_LIBRARIES}
)



install(DIRECTORY include DESTINATION include)
