cmake_minimum_required(VERSION 3.0)

project(TMIConverterLib CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(TMIConverterLib STATIC tmiConverter.cpp)
