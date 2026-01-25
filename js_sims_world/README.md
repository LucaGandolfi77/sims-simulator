# JS Sims World

A pure JavaScript/WebGL port of the Sims Simulator, using [Three.js](https://threejs.org/) and a custom Entity-Component-System (ECS).

## Features
- **3D Rendering**: Uses Three.js for high-performance 3D visualization.
- **ECS Architecture**: Custom `Registry` class managing thousands of entities efficiently, mirroring C++ `entt`.
- **Procedural World**: Generates cities, roads, and highways procedurally.
- **Simulation**:
  - **Needs**: Sleep, Money, Energy.
  - **Behaviors**: Work/Home cycles, Driving, Walking.
  - **Pathfinding**: Manhattan-style grid movement with "lane" logic.

## How to Run
Due to browser security policies (CORS) regarding ES6 Modules, you cannot simply open `index.html` file:// directly. You must use a local web server.

### Option 1: VS Code Live Server
1. Install the "Live Server" extension for VS Code.
2. Right-click `index.html` and select "Open with Live Server".

### Option 2: Python (Command Line)
If you have Python installed:
```bash
cd js_sims_world
python3 -m http.server
```
Then open [http://localhost:8000](http://localhost:8000) in your browser.

### Option 3: Node.js
If you have Node.js:
```bash
npx http-server .
```

## Structure
- `index.html`: Main entry point and UI overlay.
- `src/Main.js`: Main loop, Three.js setup, and bridging ECS to Render.
- `src/Components.js`: Data containers (SimStats, Transform, State, etc.).
- `src/ECS.js`: The Entity Component System engine.
- `src/World.js`: Procedural generation logic (Cities, Buildings, Roads).
- `src/Systems.js`: Logic systems (Movement, State Machine, Stats).
