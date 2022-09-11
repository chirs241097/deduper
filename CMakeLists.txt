cmake_minimum_required(VERSION 3.14.0)
project(deduper CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui)
find_package(Threads REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

include_directories(.)

add_library(xsig STATIC imageutil.cpp signature.cpp subslice_signature.cpp base64.cpp)

add_subdirectory(tests)
