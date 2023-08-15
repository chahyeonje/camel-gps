cmake_minimum_required(VERSION 3.26)
project(camel_gps)

set(CMAKE_CXX_STANDARD 17)

add_executable(camel_gps main.cpp
        gps.cpp
        gpsInterface.cpp)
