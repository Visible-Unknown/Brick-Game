# 🛡️ Neo Tanks: Iron Grid
### *High-Fidelity Tactical Arcade Combat* 

**Neo Tanks: Iron Grid** is a professional-grade, full-screen tactical shooter built using C++ and OpenGL. Engage in high-intensity tank warfare on a neon-lit cyber grid, featuring a sophisticated power-up system, dynamic audio, and premium visual effects.

---

## 🕹️ Gameplay Features
- **🌐 Full-Screen Immersion**: Optimized for a seamless, distraction-free arcade experience.
- **✨ Professional UI**: Utilizing custom Stroke-Font technology for glowing, neon-aesthetic titles and a sleek HUD.
- **🔋 Tactical Power-Up System**: 8 unique collectible items to turn the tide of battle:
    - **🛡️ Iron Shield**: Invulnerability for 10 seconds.
    - **🔫 Spread Shot**: Fires a devastating 3-shell arc.
    - **⚡ Hyper Cannon**: Overclocked rapid-fire firing mode.
    - **🚀 Turbo Engines**: Drastic speed boost for tactical positioning.
    - **❄️ EMP Freeze**: Paralyze all enemies on the grid.
    - **💣 Plasma Mines**: Deploy stationary hazards to trap enemies.
    - **🔧 Hull Repair**: Restore tank health (+1 HP).
    - **💥 Mega Bomb**: Screen-clearing tactical detonation.
- **🎵 Pro Audio Engine**: Features studio-produced synthwave background music and procedurally synthesized mechanical sound effects.
- **💥 Dynamic Effects**: Camera shake, screen resonance, and additive-blended particle explosions.

---

## 🎮 Controls
| Action | Key |
| :--- | :--- |
| **Move Tank** | Arrow Keys |
| **Fire Cannon** | `SPACE` |
| **Pause / Resume** | `P` |
| **Restart Game** | `R` (on Game Over) |
| **Quit** | `ESC` |

---

## 🛠️ Build Instructions
To compile the game on Windows (requires MinGW and FreeGLUT):

```bash
g++ tanks.cpp -o tanks.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
```

Ensure the `tanks music/` directory is present in the same folder as the executable for full audio support.

---

## 👨‍💻 Credits
Developed with ❤️ by **AL AMIN HOSSAIN**.

Part of the **Neo Arcade** Collection.
