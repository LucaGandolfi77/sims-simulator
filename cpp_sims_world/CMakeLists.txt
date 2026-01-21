cmake_minimum_required(VERSION 3.11)

project(NationSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------
# Raylib FetchContent
# Scarica e compila Raylib automaticamente se non presente nel sistema
# -----------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)
FetchContent_MakeAvailable(raylib)

# -----------------------------------------------------------------------------
# Setup Sources
# -----------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/Sim.cpp
    # Add other cpp files here if created
)

set(HEADERS
    src/Sim.hpp
    src/World.hpp
)

# -----------------------------------------------------------------------------
# Executable
# -----------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link Raylib
target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# macOS Specific Frameworks (handled by raylib via cmake usually, but ensuring)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()

# Copy resources if needed (not needed for this procedural project, but good practice)
# file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})
