cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 20)

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

find_package(OpenCV REQUIRED)

#executable
add_executable(control_program
        src/main.cpp
        src/state_management.cpp
        src/utils/utils.cpp
        src/utils/save_laz.cpp
        src/clients/TimeStampReceiver.cpp
        src/clients/SaveChunkInFileClient.cpp
        src/clients/concrete/GnssClient.cpp
        src/clients/concrete/LivoxClient.cpp
        src/clients/concrete/GpioClient.cpp
        src/clients/concrete/FileSystemClient.cpp
        src/clients/concrete/CamerasClient.cpp
        src/clients/concrete/ImuFileSaver.cpp
        src/clients/concrete/LidarFileSaver.cpp)

target_include_directories(control_program
        PUBLIC include
        SYSTEM ${OpenCV_INCLUDE_DIRS})

set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -latomic")
target_link_libraries(control_program livox_lidar_sdk_static cppgpio-static pistache atomic laszip ${LIBSERIAL_LIBRARY} minea ${OpenCV_LIBS})

add_executable(led_demo src/demos/led_demo.cpp src/clients/concrete/GpioClient.cpp)
target_include_directories(led_demo PRIVATE include)
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS}")
target_link_libraries(led_demo cppgpio-static)

add_executable(button_demo src/demos/button_demo.cpp src/clients/concrete/GpioClient.cpp)
target_include_directories(button_demo PRIVATE include)
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS}")
target_link_libraries(button_demo cppgpio-static)
