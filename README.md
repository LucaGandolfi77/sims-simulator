# 🏠 Sims Style Simulator - 3D Edition

<div align="center">

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Made with Three.js](https://img.shields.io/badge/Made%20with-Three.js-black?logo=three.js)](https://threejs.org/)
[![Vanilla JavaScript](https://img.shields.io/badge/JavaScript-ES6+-F7DF1E?logo=javascript)](https://www.javascript.com/)
[![Status: Active](https://img.shields.io/badge/Status-Active-brightgreen)](https://github.com/LucaGandolfi77/sims-simulator)

A delightful 3D life simulation game inspired by The Sims, featuring autonomous AI-driven characters navigating a procedural chessboard world with realistic behaviors and attributes.

[**🎮 Play Now**](#getting-started) • [**📖 Features**](#-features) • [**🛠️ Tech Stack**](#-tech-stack) • [**📝 License**](#-license)

</div>

---

## ✨ Features

### 🤖 Intelligent AI Characters
- **Autonomous Movement**: Characters navigate intelligently with directional pathfinding
- **Realistic Behaviors**: Eating, sleeping, socializing, and mood management
- **Dynamic Interactions**: Characters interact when nearby, affecting each other's moods
- **Natural Navigation**: Move straight with random left/right turns for organic pathfinding

### 🎮 Engaging Gameplay
- **Real-time Stats**: Monitor hunger, energy, and mood for each character
- **Interactive Control**: Manually command characters to eat, sleep, or move
- **Activity Logging**: Track all character actions in a live activity feed
- **Game Control**: Pause, resume, and reset the simulation anytime

### 🎨 Beautiful 3D Graphics
- **Three.js Rendering**: Smooth, performant 3D graphics
- **Procedural Chessboard**: Dynamic white/grey tiled game world
- **Realistic Lighting**: Shadow mapping and ambient lighting
- **Responsive Design**: Works seamlessly on desktop and tablet
- **Polygonal Character Models**: Customized 3D models with body, head, arms, and legs

### ⚡ Performance
- **Optimized Rendering**: 60 FPS smooth animation
- **Efficient Game Loop**: 1-second update interval for responsive gameplay
- **Scalable Architecture**: Easy to extend with new features and characters

---

## 🎮 Gameplay Mechanics

### Character Attributes
Each character has three core attributes (0-100):
- **🍽️ Hunger**: Decreases over time, managed by eating
- **⚡ Energy**: Decreases slowly, restored by sleeping
- **😊 Mood**: Affected by hunger, energy, and social interactions

### Character Actions
- **Eat**: Restores hunger and improves mood
- **Sleep**: Fully restores energy
- **Move**: Character autonomously explores the map
- **Socialize**: Characters interact when adjacent, boosting each other's mood

### Game World
- **10x10 Grid**: Chessboard-patterned tile system
- **Boundary Walls**: Characters intelligently turn at boundaries
- **Collision Detection**: Characters avoid overlapping
- **Real-time Updates**: Seamless 3D visualization

---

## 🚀 Getting Started

### Prerequisites
- Modern web browser (Chrome, Firefox, Safari, Edge)
- No installation required!

### How to Play

1. **Open the Game**
   ```
   Simply open index.html in your web browser
   ```

2. **Observe Characters**
   - Watch Alex, Bella, and Carl explore the map
   - Monitor their attributes in the sidebar

3. **Interact**
   - Click **🍽️ Eat** to feed a character
   - Click **💤 Sleep** to restore energy
   - Click **🚶 Move** to reposition a character

4. **Control**
   - **⏯️ Pause/Resume**: Freeze or resume the simulation
   - **🔄 Reset Game**: Start over with fresh characters

---

## 🛠️ Tech Stack

- **Rendering**: [Three.js](https://threejs.org/) - 3D JavaScript library
- **Graphics**: WebGL with shadow mapping and PBR materials
- **Language**: Vanilla JavaScript (ES6+)
- **Styling**: Modern CSS with gradients and animations
- **Architecture**: Object-oriented with Game/Sim classes

### Key Technologies

```javascript
// Core Dependencies
Three.js r128 - 3D Graphics
Vanilla JS - No frameworks

// Features
Object-Oriented Design
Real-time Game Loop
Procedural Generation
AI Pathfinding
```

---

## 📁 Project Structure

```
sims-simulator/
├── index.html          # Main game file (single-file app)
├── README.md           # Documentation
└── LICENSE             # MIT License
```

### Core Components

**Game Class**: Main game controller
- Manages game state and characters
- Handles UI updates and logging
- Controls game loop timing

**Sim Class**: Character AI system
- Autonomous movement with directional pathfinding
- Attribute management and decay
- Action execution and timers
- 3D model positioning

**Three.js Scene**: 3D rendering engine
- Chessboard floor generation
- Character model creation
- Lighting and shadow setup
- Real-time animation loop

---

## ⚙️ Configuration

Edit the `CONFIG` object in `index.html` to customize:

```javascript
const CONFIG = {
    GRID_SIZE: 10,           // World grid dimensions
    UPDATE_INTERVAL: 1000,   // Milliseconds between updates
    ATTRIBUTE_DECAY: 0.5,    // Rate of attribute loss
    MAX_LOG_ENTRIES: 10,     // Activity log size
    CELL_SIZE: 1             // Grid cell size
};
```

---

## 🎨 Character Models

Each character is composed of:
- **Body**: Colored cylinder (character-specific color)
- **Head**: Skin-tone sphere with eyes
- **Eyes**: Black spheres for expression
- **Arms**: Two cylindrical limbs
- **Legs**: Two dark pants

Colors:
- **Alex**: `#FF6B6B` (Red)
- **Bella**: `#4ECDC4` (Teal)
- **Carl**: `#FFD93D` (Yellow)

---

## 📊 Performance Metrics

- **Rendering**: 60 FPS (60 frames per second)
- **Game Updates**: Every 1 second
- **Scene Objects**: ~150 polygons + dynamic characters
- **Memory**: Lightweight, <5MB
- **File Size**: Single HTML file (~30KB)

---

## 🎯 Future Enhancements

- [ ] Multiple game maps with different themes
- [ ] Character customization and creation
- [ ] Economy system with objects and trades
- [ ] Procedural world generation
- [ ] Multiplayer/network support
- [ ] Mobile touch controls
- [ ] Audio and sound effects
- [ ] Achievement system
- [ ] Character memories and relationships
- [ ] Day/night cycle

---

## 🤝 Contributing

Contributions are welcome! Feel free to:
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## 📝 License

This project is licensed under the MIT License - see the LICENSE file for details.

```
MIT License

Copyright (c) 2025 Luca Gandolfi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
```

---

## 🙌 Credits

- Inspired by **The Sims** (Maxis/Electronic Arts)
- Built with **Three.js** - JavaScript 3D library
- Developed by **Luca Gandolfi**

---

## 📞 Support

Have questions or found a bug? 
- 🐛 [Report an Issue](https://github.com/LucaGandolfi77/sims-simulator/issues)
- 💬 [Start a Discussion](https://github.com/LucaGandolfi77/sims-simulator/discussions)
- 📧 Reach out via GitHub

---

<div align="center">

### Made with ❤️ and lots of ☕

**[⬆ back to top](#-sims-style-simulator---3d-edition)**

</div>