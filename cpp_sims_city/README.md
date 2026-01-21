# Sims Simulator C++

This is a C++ simulation of autonomous agents ("Sims") using the Raylib graphics engine. It is designed to run natively on macOS (Apple Silicon).

## Prerequisites

You need a C++ compiler and CMake.

1.  **Xcode Command Line Tools** (for the compiler):
    ```bash
    xcode-select --install
    ```
2.  **CMake** (to build the project):
    ```bash
    brew install cmake
    ```
    *(If you don't have Homebrew, visit [brew.sh](https://brew.sh) to install it, or download CMake from [cmake.org](https://cmake.org/download/))*

## How to Build

1.  Navigate to the `cpp_sims` folder in your terminal:
    ```bash
    cd cpp_sims
    ```

2.  Create a build directory:
    ```bash
    mkdir build
    cd build
    ```

3.  Generate the build files (this will download Raylib automatically):
    ```bash
    cmake ..
    ```

4.  Compile the project:
    ```bash
    make
    ```

## How to Run

After compiling, the executable will be in the `build` folder.

```bash
./SimsSimulator
```

## Controls

-   **Mouse Left Click + Drag**: Rotate the camera around the center.
-   **Mouse Wheel**: Zoom in/out.
-   **ESC**: Exit the simulator.

## Features

-   **Autonomous Sims**: Each Sim has stats (Hunger, Energy) independent of others.
-   **Decision Making**: Sims probabilistically decide to Eat, Sleep, Wander, or Idle based on their current needs.
-   **Visualization**: Real-time 3D rendering of the characters and their status overlaid on screen.
