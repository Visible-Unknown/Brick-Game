# NEO SHOOTER 3D (Visual Enhanced Edition)

A high-performance fixed-screen arcade shooter featuring advanced 3D modeling, premium lighting, and a refined power-up arsenal.

## 🕹️ Gameplay Mechanics

### 🛸 Detailed 3D Spaceship
The player ship features a multi-component 3D mesh:
*   AERODYNAMIC FUSELAGE: Glossy blue-metallic body.
*   DUAL ENGINE GLOW: Additive-blended pulsing engine flames.
*   TRANSLUCENT COCKPIT: Glassy canopy with internal reflections.

### 👾 Tactical Enemy Interaction
Enemies are tiered based on HP, with colors indicating their threat level:
*   🔴 **RED (Standard Fighter)**: 1 HP - Quick to dispatch.
*   🟠 **ORANGE (Heavy Interceptor)**: 2 HP - Requires sustained fire.
*   🟣 **PURPLE (Elite Fighter)**: 3 HP - Heavily armored threat.

### 🔋 Power-Up Arsenal
Management of power-ups is key to survival:
*   🟨 **TRIPLE SHOT (5s)**: Spread fire covering 30% of the arena.
*   🟦 **SHIELD BUBBLE**: Fully protects the ship from one impact.
*   🟧 **HYPER-DRIVE (5s)**: Overclocks weapon systems for rapid fire.
*   🟪 **NEON OVERDRIVE (5s)**: Slows the physics time-step for enemies.
*   🟩 **REPAIR KIT**: Instantly restores 1 HP (System Health).

## ⚡ Tech Specs
*   **Visuals**: 600 twinkling stars (white/blue/gold), decorative drifting asteroids, and additive additive particle explosions.
*   **Lighting**: Two-light setup (Cool White Key light + Purple Rim light) for a premium 3D depth perception.
*   **Engine**: Smooth delta-time driven framerate (MSAA enabled).
*   **Audio**: Windows Multimedia (MCI/PlaySound) system.

## ⌨️ Controls
| Key | Action |
| :--- | :--- |
| **Arrow Left/Right** | Move Ship |
| **SPACE** | Fire Laser |
| **P** | Pause / Resume |
| **R** | Fast Restart |
| **ESC** | Quit to OS |

## 🛠️ Build Requirements
*   **Compiler**: `g++` (MinGW)
*   **Libraries**: GL, GLU, FreeGLUT, WinMM
*   **Assets**: Place `shooter music/` folder in the game directory.

### Build Command:
```powershell
g++ shooter.cpp -o shooter.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
```

---
*Developed by @alamin. Optimized with AG-1 AI.*
