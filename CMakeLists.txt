cmake_minimum_required(VERSION 3.31)
project(BreadedGBA)

set(CMAKE_CXX_STANDARD 20)

add_executable(BreadedGBA src/main.cpp src/system.cpp src/cpu/arm7_cpu.cpp src/memory/memory.cpp src/ppu/ppu.cpp)
