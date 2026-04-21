# 🔫 Neo Cyber-Doom: Mainframe Breach
### *Retro-3D Raycasting First-Person Shooter*

**Neo Cyber-Doom** is a full raycasting first-person shooter inspired by Wolfenstein 3D and Doom, rendered in a glowing TRON-style neon wireframe aesthetic. Navigate a hostile labyrinthine mainframe, find keycards, blast security turrets, and reach the exit portal.

---

## 🕹️ Gameplay Features
- **🌐 Full-Screen Raycasting Engine**: Real-time Wolfenstein-style column rendering with distance fog and perspective correction.
- **✨ Professional UI**: Stroke-font titles, HP bar, minimap overlay, and "System Terminated" game-over screens.
- **🗺️ Hand-Crafted Map**: A 16x16 maze with strategic corridors, hidden items, and locked doors.
- **🔑 Colored Keycards**: Find the Red Key and Blue Key to unlock corresponding doors blocking your path.
- **🤖 Enemy Turrets**: 8 AI turrets that track your position and fire plasma bolts. Each requires 3 hits to destroy.
- **🔫 Weapon View Model**: A fully rendered plasma gun at the bottom of the screen with muzzle flash effects.
- **🗺️ Tactical Minimap**: Live overhead minimap showing walls, doors, player position, direction, and enemy locations.
- **🔋 Item Pickups**:
    - ❤️ **Health Pack**: Restores 30 HP.
    - ⚡ **Speed Boost**: 15 seconds of 1.67x movement speed.
    - 🔫 **Rapid Fire**: 15 seconds of 2.5x fire rate.
- **🎵 Pro Audio Engine**: Aggressive driving BGM with tactical SFX.

---

## 🎮 Controls
| Action | Key |
| :--- | :--- |
| **Move Forward / Back** | `W` / `S` or Up / Down Arrow |
| **Turn Left / Right** | `A` / `D` or Left / Right Arrow |
| **Fire Plasma Bolt** | `SPACE` / `Z` |
| **Start / Restart** | `SPACE` (on menus) |
| **Pause / Resume** | `P` |
| **Quit** | `ESC` |

---

## 🛠️ Build Instructions
```bash
g++ cyber_doom.cpp -o cyber_doom.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
```

---

## 👨‍💻 Credits
Developed with ❤️ by **AL AMIN HOSSAIN**.

Part of the **Neo Arcade** Collection.
