cmake_minimum_required(VERSION 3.10)

project(Converter)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(TMIConverterLib)
add_subdirectory(TMIGenerator)

add_executable(TMIConverter main.cpp)
target_include_directories(TMIConverter PUBLIC "TMIConverterLib")

target_link_libraries(TMIConverter TMIConverterLib)
