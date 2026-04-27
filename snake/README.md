# 🐍 Neo Snake 3D: Cyber Link
### *High-Fidelity Physics Grid Simulator*

**Neo Snake 3D: Cyber Link** is a radically overhauled Pro Edition of the classic snake formula. Navigate a fully 3D isometric cyberpunk grid, consume data apples, manage immense lengths, and utilize rare Cyber Cores to keep your drone alive.

---

## 🕹️ Gameplay Features
- **🌐 Full-Screen High-Resolution Engine**: Launched immediately into 1280x720 fullscreen output featuring deeply immersive glowing wireframe depth.
- **✨ Professional Interface**: Powered by Custom Stroke-Font architecture. Features transparent overlays, high-tech top-bar HUD, and an atmospheric title screen seamlessly integrated with the 3D physics engine.
- **💣 Visceral Visuals**: Cinematic sweeping grid glows, heavy particle explosions that respond to physics, camera shake loops, and custom additive-blended cube generation representing your growing snake tail.
- **🧬 Dynamic Mechanics**: A custom speed-scaling algorithm gently raises your underlying speed as you grow, forcing intense reflexes and tight navigation.
- **🔋 Cyber Power-Up System**: Randomly spawning power nodes dictate high-level tactical recovery:
    - 🟡 **Spectrum Core (Yellow Gem)**: Instantly truncates the last 4 cubes of your tail, giving you immense breathing room when the grid becomes overcrowded. 
    - 🟣 **Phantom Protocol (Purple Gem)**: Grants 10 seconds of true ghost-phasing. Steer the snake's head directly through your tail walls without triggering a system failure!
    - 🔵 **Cryo Flux (Blue Gem)**: Drops your movement speed down significantly for 15 seconds, letting you execute complex maneuvers effortlessly.
- **🎵 Pro Audio Engine**: Driven by `winmm` cross-linked assets. Hear thumping synthwave racing music driving you forward, backed by satisfying digital blips whenever data apples are collected.

---

## 🎮 Controls
| Action | Key |
| :--- | :--- |
| **Change Vector** | Arrow Keys |
| **Initialize Link** | `SPACE` |
| **Pause / Resume** | `P` |
| **Force Restart** | `R` (on Game Over) |
| **Terminate Process** | `ESC` |

---

## 🛠️ Build Instructions
To compile the game natively on Windows (requires MinGW, FreeGLUT):

```bash
g++ snake.cpp -o snake.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
```

Ensure all audio dependencies (`../racing/audio/` and `../tanks/tanks music/`) are present in the repository tree to allow the cross-module Audio Engine to function.

---

## 👨‍💻 Credits
Developed with ❤️ by **AL AMIN HOSSAIN**.

Part of the **Neo Arcade** Collection.
