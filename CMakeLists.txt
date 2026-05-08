cmake_minimum_required(VERSION 3.31)
project(Jadeight)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
add_executable(Jadeight main.cpp)
