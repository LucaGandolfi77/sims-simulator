cmake_minimum_required(VERSION 3.11)

project(NationSimulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------
# Raylib FetchContent
# -----------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz
)
FetchContent_MakeAvailable(raylib)

# -----------------------------------------------------------------------------
# EnTT FetchContent (ECS Library)
# -----------------------------------------------------------------------------
FetchContent_Declare(
    entt
    URL https://github.com/skypjack/entt/archive/refs/tags/v3.13.0.tar.gz
)
FetchContent_MakeAvailable(entt)


# -----------------------------------------------------------------------------
# Setup Sources
# -----------------------------------------------------------------------------
set(SOURCES
    src/main.cpp
    src/Components.hpp
    src/Systems.hpp
)

set(HEADERS
    src/World.hpp
)

# -----------------------------------------------------------------------------
# Executable
# -----------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link Libraries
target_link_libraries(${PROJECT_NAME} PRIVATE raylib EnTT::EnTT)

# macOS Specific
if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()
