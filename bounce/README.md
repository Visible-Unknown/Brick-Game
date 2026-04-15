# 🔴 Neo Bounce: Reactor Core
### *High-Fidelity Physics Platformer*

**Neo Bounce** is a challenging physics platformer inspired by the beloved Nokia classic. Navigate a glowing sphere through procedurally generated obstacle courses, collect rings to unlock the exit, and master the Inflate/Deflate mechanics.

---

## 🕹️ Gameplay Features
- **🌐 Full-Screen Immersion**: Side-scrolling 3D camera that tracks the ball through expansive levels.
- **✨ Professional UI**: Stroke-font titles, transparent HUD with ring counter, and "Core Meltdown" game-over screens.
- **🎈 Inflate/Deflate Mechanics**:
    - **Inflate (Z)**: Grow larger, float gently, and smash through glass platforms.
    - **Deflate (X)**: Shrink down to squeeze through tight gaps.
- **💎 Ring Checkpoints**: Collect all rings in a level to unlock the glowing exit gate.
- **🧊 Breakable Glass**: Semi-transparent platforms shatter when hit while inflated.
- **⚡ Spike Hazards**: Red spiked platforms mean instant death on contact.
- **🔋 Power-Up System**:
    - 🟢 **Super Bounce (Green)**: Automatically bounce off platforms for 12 seconds.
    - 🔵 **Time Freeze (Blue)**: Slows physics to 30% speed for 10 seconds.
- **📈 10 Procedural Levels**: Increasing complexity with more spikes, glass, and rings.
- **🎵 Pro Audio Engine**: Cross-linked synthwave BGM and SFX.

---

## 🎮 Controls
| Action | Key |
| :--- | :--- |
| **Move Ball** | Left / Right Arrow |
| **Jump** | Up Arrow / `SPACE` |
| **Inflate** | `Z` (hold) |
| **Deflate** | `X` (hold) |
| **Start / Restart** | `SPACE` (on menus) |
| **Pause / Resume** | `P` |
| **Quit** | `ESC` |

---

## 🛠️ Build Instructions
```bash
g++ bounce.cpp -o bounce.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
```

---

## 👨‍💻 Credits
Developed with ❤️ by **AL AMIN HOSSAIN**.

Part of the **Neo Arcade** Collection.
