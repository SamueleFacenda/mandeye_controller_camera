cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_CXX_STANDARD 20)
if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
endif()

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

# Tbb
find_package(PkgConfig REQUIRED)
pkg_check_modules(TBB REQUIRED tbb)

find_package(OpenCV REQUIRED)

# pigpio
find_package(pigpio QUIET)
if(NOT pigpio_FOUND)
    include(3rd/Findpigpio.cmake)
endif()

include_directories(${pigpio_INCLUDE_DIR})

#executable
add_executable(control_program
        src/main.cpp
        src/state_management.cpp
        src/utils/utils.cpp
        src/utils/save_laz.cpp
        src/clients/TimeStampReceiver.cpp
        src/clients/concrete/GnssClient.cpp
        src/clients/concrete/LivoxClient.cpp
        src/clients/concrete/GpioClient.cpp
        src/clients/concrete/FileSystemClient.cpp
        src/clients/concrete/SystemTimeStampProvider.cpp
        src/clients/concrete/CamerasClient.cpp
)

target_include_directories(control_program
        PUBLIC include
        SYSTEM ${OpenCV_INCLUDE_DIRS})

set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -latomic")
message("${TBB_LIBRARIES}")
target_link_libraries(control_program livox_lidar_sdk_static pistache atomic laszip ${LIBSERIAL_LIBRARY} minea ${OpenCV_LIBS} ${TBB_LIBRARIES} ${pigpio_LIBRARY})

add_executable(led_demo src/demos/led_demo.cpp src/clients/concrete/GpioClient.cpp)
target_include_directories(led_demo PRIVATE include)
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS}")
target_link_libraries(led_demo ${pigpio_LIBRARY})

add_executable(button_demo src/demos/button_demo.cpp src/clients/concrete/GpioClient.cpp)
target_include_directories(button_demo PRIVATE include)
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS}")
target_link_libraries(button_demo ${pigpio_LIBRARY})
