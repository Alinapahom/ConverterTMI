cmake_minimum_required(VERSION 3.0)

project(TMIConverter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(TMIConverterProgram main.cpp)
include_directories("../TMIConverterLib")

# link_directories("../build/TMIConverterLib")
target_link_libraries(TMIConverterProgram TMIConverter)

if (WIN32)
    target_link_libraries(TMIConverterProgram wsock32 ws2_32)
endif()
