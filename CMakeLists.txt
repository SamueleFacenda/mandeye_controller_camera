cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 17)

project (mandeye_multisensor)

message("pistache")
#pistache
add_subdirectory(3rd/pistache)

message("LASzip library")
# LASzip library
add_subdirectory(3rd/LASzip)
include_directories(3rd/LASzip/include)

message("INCBIN")
# INCBIN
include_directories(3rd/incbin)

message("JSON")
#JSON
include_directories(3rd/json)

message("Livox SDK2")
# Livox SDK2
add_subdirectory(3rd/Livox-SDK2)

# GPIOs
add_subdirectory(3rd/CppGPIO)
include_directories(3rd/CppGPIO/include/)

# libserial
find_path(LIBSERIAL_INCLUDE_DIR SerialPort.h
  PATH_SUFFIXES libserial include
  PATHS ${CMAKE_INCLUDE_PATH} /usr /usr/local /usr/include/libserial)
message(${LIBSERIAL_INCLUDE_DIR})
find_library(LIBSERIAL_LIBRARY
  NAMES libserial.so
  PATH_SUFFIXES lib
  PATHS ${CMAKE_LIBRARY_PATH} /usr/lib)

# NMEA
include_directories(3rd/minmea ${LIBSERIAL_INCLUDE_DIR})
add_library(minea STATIC 3rd/minmea/minmea.c)


# Project headers
include_directories(code/)

#executable
add_executable(control_program
        ${PISTACHE_SRC}
        code/main.cpp
        code/utils/save_laz.cpp
        code/state_management.cpp
        code/clients/GnssClient.cpp
        code/clients/LivoxClient.cpp
        code/clients/GpioClient.cpp
        code/clients/FileSystemClient.cpp
        code/clients/TimeStampReceiver.cpp
        code/utils/utils.cpp
        code/file_savers.cpp)

set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -latomic")
target_link_libraries(control_program pthread livox_lidar_sdk_static cppgpio-static pistache atomic laszip ${LIBSERIAL_LIBRARY} minea)
#target_link_libraries(control_program pthread livox_lidar_sdk_static cppgpio-static atomic laszip)

add_executable(led_demo code/demos/led_demo.cpp code/clients/GpioClient.cpp)
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS}")
target_link_libraries(led_demo cppgpio-static)

add_executable(button_demo code/demos/button_demo.cpp code/clients/GpioClient.cpp
        code/web/ServerHandler.h)
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS}")
target_link_libraries(button_demo cppgpio-static)
