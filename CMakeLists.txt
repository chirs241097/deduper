cmake_minimum_required(VERSION 3.11.0)
project(deduper CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

include_directories(.)

add_library(xsig STATIC imageutil.cpp signature.cpp)

add_subdirectory(tests)
