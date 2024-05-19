cmake_minimum_required(VERSION 3.20)

project(snes)

set(CMAKE_BUILD_TYPE DEBUG)

set(SOURCES src/mem/Bus.cpp
            src/cpu/cpu.cpp
            src/main.cpp
            src/ppu/ppu.cpp
            src/mem/hdma.cpp
            src/sound/spc700.cpp
            src/sound/dsp.cpp)

include_directories(${CMAKE_SOURCE}/src)
include_directories(${CMAKE_SOURCE})

add_executable(snes ${SOURCES})

find_package(SDL2 REQUIRED)
include_directories(snes ${SDL2_INCLUDE_DIRS})

target_link_libraries(snes ${SDL2_LIBRARIES})