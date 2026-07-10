cmake_minimum_required(VERSION 3.16)
project(coriolis CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.5
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

add_executable(coriolis_sim sim/main.cpp)
target_link_libraries(coriolis_sim PRIVATE raylib)

# display shape: default 128x64 widescreen; switch to the square build with
#   cmake -B build -DSQUARE=ON
option(SQUARE "Build for 128x128 instead of 128x64" OFF)
if(SQUARE)
  target_compile_definitions(coriolis_sim PRIVATE CORIOLIS_HEIGHT=128)
endif()
