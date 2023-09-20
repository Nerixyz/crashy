cmake_minimum_required(VERSION 3.20)
project(c2-crash CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED On)

add_library(crashy-payload SHARED payload.cpp)
add_executable(crashy crashy.cpp)
