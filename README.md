# Neo Brick Game 3D: Classic Collection

A modernized, neon-infused, completely fully-3D interpretation of the classic 90s generic "9999-in-1 Brick Game" LCD handheld consoles. Built from scratch in raw C++ and OpenGL.

This project strips the classic LCD dot-matrix algorithms and revamps them into a high-speed synthwave aesthetic featuring dynamic lighting, real-time particle physics, and a responsive custom framework.

## 🕹️ The Games

### 🏎️ Neo Racing 3D (Pro Edition)
An infinite, pseudo-3D high-speed lane racer that pushes your reaction times to the limit. Dodge incoming grid traffic and survive increasing terminal velocities as you race down a massive synthetic 3-lane highway.

**Power-Up System:**
Look out for rotating geometric 3D crystals on the track. They grant temporary tactical advantages:
* 🟦 **Ice Blue Shield:** Absorbs the impact of exactly one collision, saving you from a fatal crash.
* 🟪 **Purple Ghost Mode (8s):** Renders your vehicle translucent and phases you directly through enemy traffic without harm.
* 🟨 **Yellow Time Slow (5s):** Halves the physics engine's time-step, plunging the world into slow-motion to let you navigate dense enemy clusters.
* 🟩 **Green Score Bonus:** Instantly grants +100 to your high score.

**Physics & Aesthetics:** Features smooth steering interpolation, realistic vehicle banking (roll) during high-speed turns, immersive dynamic camera shakes on crashes, and an integrated audio engine.

---

### *Other Titles (Currently in Development)*
* **Neo Tetris 3D:** The timeless block-stacking puzzle projected geometrically with volumetric depth, dynamic grid trails, and glowing neon clears.
* **Neo Space Shooter:** A classic fixed-screen arcade shooter. Fend off descending waves of hostile geometry while managing rapid-fire lasers. 
* **Neo Snake 3D:** A fully 3-dimensional take on the classic grid-crawler. 
* **Brick Breaker 3D:** The foundational 3D bouncy-ball demolition baseline.

## ✨ Core Features

* **Custom Lightweight Physics Loop:** Handles collision detection, procedural structural destruction, and smooth camera banking natively without heavy external libraries.
* **Synthwave Visuals:** Heavy use of fixed-function 3D scaling, fog gradients, and additive-blended particle bursts to mimic modern post-processing blooms purely in raw OpenGL.
* **Dynamic Audio Engine:** Custom background tracking hooked directly into the Windows Multimedia (`mmsystem.h`) backend for zero-latency SFX and looping BGM.
* **Stand-Alone Architectures:** Every game runs entirely within an independent, un-bloated executable.

## 🛠️ Build & Installation

### 🎮 Play Without Installation (Windows)
You don't need to set up a C++ development environment to play! Simply download the compiled `racing.exe` file and the `audio/` folder from this repository, place them together in the exact same directory, and double-click `racing.exe` to start racing instantly.

### Requirements (For Compiling from Source)
* C++ Compiler (`g++` / MinGW)
* OpenGL / FreeGLUT Libraries
* Windows environment (Recommended for native audio engine support, though the core gameplay runs fundamentally on Linux/macOS).

### Compiling on Windows (MinGW)
Each game is self-contained. To compile the flagship racing module, run the following via CMD or PowerShell in the root directory:

```text
g++ racing.cpp -o racing.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
```
*(Note: The `-lwinmm` flag is strictly required to link the Windows audio engine module).*

### Audio Setup
The audio system expects a valid sub-directory. Ensure the `audio/` folder is placed exactly alongside the compiled `.exe` files containing:
* `audio/bgm.mp3`
* `audio/crash.wav`
* `audio/pickup.wav`

## ⌨️ Global Controls

* **Arrow Keys:** Steer, Move, Navigate Grid
* **R:** Fast Restart / Respawn
* **ESC:** Power off / Quit to Desktop

## 🤝 Contributing
Feel free to fork the repository and open pull requests. If you'd like to add an implementation of another classic LCD algorithm (e.g., Tanks, Frogger, Match-3), please ensure it adheres to the established visual synthwave rendering guidelines.

---
*Developed by @alamin.*
