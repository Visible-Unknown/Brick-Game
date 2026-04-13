# 💎 Neo Match-3: Crystal Flux
### *High-Fidelity Neural Grid Puzzle*

**Neo Match-3: Crystal Flux** is a professional-grade, full-screen puzzle experience built using C++ and OpenGL. Match high-fidelity 3D crystals to generate neural flux, unlock powerful special stones, and climb the high-score boards.

---

## 🕹️ Gameplay Features
- **🌐 Full-Screen Immersion**: Optimized for a seamless, high-resolution arcade experience.
- **✨ Professional UI**: Using custom Stroke-Font technology for glowing, neon-aesthetic titles and a refined, semi-transparent HUD.
- **⚡ Special Gem System**: Harness the power of strategic matches to clear the board:
    - **Line Flux (4-Match)**: Clears an entire horizontal or vertical row.
    - **Pulse Bomb (T/L-Match)**: Detonates in a 3x3 radius, shattering nearby crystals.
    - **Spectrum Cluster (5-Match)**: Swap with any color to clear all visible gems of that type.
- **🔄 Dynamic Cascading**: Realistic gravity-based falling physics and smooth swap animations.
- **⏱️ Time-Attack Mode**: Frantic gameplay with score-based time bonuses and increasing multiplier scaling.
- **💥 Visual Resonance**: Features camera shake, addictive glow effects, and additive-blended particle bursts.

---

## 🎮 Controls
| Action | Key |
| :--- | :--- |
| **Move Cursor** | Arrow Keys |
| **Select / Swap** | `SPACE` |
| **Pause / Resume** | `P` |
| **Restart Game** | `R` (on Game Over) |
| **Quit** | `ESC` |

---

## 🛠️ Build Instructions
To compile the game on Windows (requires MinGW and FreeGLUT):

```bash
g++ match3.cpp -o match3.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
```

The game automatically handles high-score persistence via `highscore.dat`.

---

## 👨‍💻 Credits
Developed with ❤️ by **AL AMIN HOSSAIN**.

Part of the **Neo Arcade** Collection.
