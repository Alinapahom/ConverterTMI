cmake_minimum_required(VERSION 3.0)

project(TMIGenerator CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(TMIGenerator main.cpp tmiGenerator.cpp)

if (WIN32)
    target_link_libraries(TMIGenerator wsock32 ws2_32)
endif()
