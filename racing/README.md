# NEO RACING 3D (Pro Edition)

A professional, high-fidelity pseudo-3D lane racer featuring smooth steering interpolation and a premium synthwave aesthetic.

## 🕹️ Gameplay Mechanics

### 🏎️ Professional Physics Profile
The game features a custom steering and banking (roll) system. Moving between lanes transitions the camera and the car model smoothly, creating an immersive sense of momentum.

### ⚡ FOV-Locked Camera Tracking
To prevent the common "edge stretching" found in many web/arcade racers, this edition uses a narrow 45.0 FOV with active camera tracking. The camera firmly tracks the player's X-position to keep the 3D car model centered and distortion-free.

### 🔋 Power-Up System
Dodge high-intensity grid traffic and collect power-ups to survive:
*   🟦 **SHIELD**: Absorbs the impact of exactly one collision.
*   🟪 **GHOST MODE (8s)**: Vaporizes collision logic, allowing you to phase through enemy traffic.
*   🟨 **TIME SLOW (5s)**: Dilates the physics engine, slowing everything except the player's reaction.
*   🟩 **SCORE BONUS**: Instantly grants +100 to your current Personal Best.

## ⚡ Tech Specs
*   **Aesthetics**: Spinning 3D alloy wheels, high-gloss metallic car body, and custom neon under-glow.
*   **Environment**: Fog-shrouded infinite grid with dynamic scroll offsets.
*   **Performance**: Locked 60FPS physics loop with asynchronous audio.

## ⌨️ Controls
| Key | Action |
| :--- | :--- |
| **Arrow Left/Right** | Steer / Change Lanes |
| **R** | Restart Mission |
| **ESC** | Quit to OS |

## 🛠️ Build Requirements
*   **Compiler**: `g++` (MinGW)
*   **Libraries**: GL, GLU, FreeGLUT, WinMM
*   **Assets**: Place `audio/` folder (bgm.mp3, crash.wav, pickup.wav) in the game directory.

### Build Command:
```powershell
g++ racing.cpp -o racing.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
```

---
*Developed by @alamin. Optimized with AG-1 AI.*
