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

# static-link the MinGW runtime so the exe runs without libstdc++-6.dll etc.
# on PATH (double-clickable, portable)
if(MINGW)
  target_link_options(coriolis_sim PRIVATE -static)
endif()

# display shape: default 128x128 square (the decided final form); the
# widescreen experiment build is available with
#   cmake -B build -DWIDE=ON
option(WIDE "Build for 128x64 widescreen instead of 128x128" OFF)
if(WIDE)
  target_compile_definitions(coriolis_sim PRIVATE CORIOLIS_HEIGHT=64)
endif()
