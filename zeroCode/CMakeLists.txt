cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(pico_to_api LANGUAGES C CXX ASM)

include_directories(include)

find_package(PkgConfig REQUIRED)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)
  if(MOSQUITTO_FOUND)
    include_directories(${MOSQUITTO_INCLUDE_DIRS})
    link_directories(${MOSQUITTO_LIBRARY_DIRS})
    set(MOSQUITTO_LIBRARIES ${MOSQUITTO_LIBRARIES})
  else()
    message(FATAL_ERROR "libmosquitto not found with pkg-config.")
  endif()

  pkg_check_modules(JANSSON REQUIRED jansson)
  if(JANSSON_FOUND)
    include_directories(${JANSSON_INCLUDE_DIRS})
    link_directories(${JANSSON_LIBRARY_DIRS})
    set(JANSSON_LIBRARIES ${JANSSON_LIBRARIES})
  else()
    message(FATAL_ERROR "jansson not found with pkg-config.")
  endif()

  pkg_check_modules(CURL REQUIRED libcurl)
  if(CURL_FOUND)
    include_directories(${CURL_INCLUDE_DIRS})
    link_directories(${CURL_LIBRARY_DIRS})
    set(CURL_LIBRARIES ${CURL_LIBRARIES})
  else()
    message(FATAL_ERROR "libcurl not found with pkg-config.")
  endif()
else()
  message(FATAL_ERROR "pkg-config not found.")
endif()

add_subdirectory(src)

