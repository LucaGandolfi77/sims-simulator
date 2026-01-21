cmake_minimum_required(VERSION 3.14)
project(SimsSimulator CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Includes FetchContent module to easily download dependencies
include(FetchContent)

# Download Raylib 5.0
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)

FetchContent_MakeAvailable(raylib)

# Create the executable
add_executable(SimsSimulator
    src/main.cpp
    src/Sim.cpp
)

# Link Raylib to our executable
target_link_libraries(SimsSimulator PRIVATE raylib)

# Specific setup for macOS if needed (usually handled by Raylib's cmake, but good to be safe)
if (APPLE)
    target_link_libraries(SimsSimulator PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()
