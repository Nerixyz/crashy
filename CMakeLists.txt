cmake_minimum_required(VERSION 3.20)
project(c2-crash LANGUAGES CXX VERSION 0.1.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED On)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

add_library(crashy-payload SHARED payload.cpp)
target_link_libraries(crashy-payload PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui)
add_executable(crashy crashy.cpp)
