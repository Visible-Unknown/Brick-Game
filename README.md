# Neo Brick Game 3D: Classic Collection

A modernized, neon-infused, fully-3D interpretation of the classic 90s generic "9999-in-1 Brick Game" LCD handheld consoles. Built from scratch in raw C++ and OpenGL.

This project strips the classic LCD dot-matrix algorithms and revamps them into a high-speed synthwave aesthetic featuring dynamic lighting, real-time particle physics, and a responsive custom framework.

## 🕹️ The Games

### 🏎️ Neo Racing 3D (Pro Edition)
An infinite, pseudo-3D high-speed lane racer. Dodge incoming grid traffic and survive increasing terminal velocities as you race down a massive synthetic 3-lane highway.

**Power-Up System:**
*   🟦 **Shield:** Absorbs the impact of exactly one collision.
*   🟪 **Ghost Mode (8s):** Phase directly through enemy traffic.
*   🟨 **Time Slow (5s):** Halves the physics engine's time-step.
*   🟩 **Score Bonus:** Instantly grants +100 points.

---

### 🚀 Neo Shooter 3D (Blast Edition)
A classic fixed-screen arcade shooter. Fend off descending waves of hostile geometry while managing rapid-fire lasers and advanced powerups.

**Advanced Powerup Arsenal:**
*   🟨 **Triple Shot (5s):** Fires three lasers simultaneously for massive area coverage.
*   🟦 **Shield Bubble:** Protects your ship from one collision.
*   🟧 **Hyper-Drive (5s):** Overclocks your ship, doubling your rate of fire.
*   🟪 **Neon Overdrive (5s):** Dilates time, slowing enemies but keeping your movement normal.
*   🟩 **System Repair:** Restores 1 lost life (Repair Kit).

**Notable Features:** Features animated powerup notifications, dynamic camera shaking, and professional 50/50 audio balancing for high-impact gameplay.

---

### ⏳ Future Expansion
*   **Neo Tetris 3D:** Timeless block-stacking with volumetric depth.
*   **Neo Snake 3D:** Fully 3-dimensional take on the classic grid-crawler.

---

## ✨ Core Features

*   **Custom Lightweight Physics:** Handles collision, structural destruction, and camera banking natively.
*   **Synthwave Visuals:** Fixed-function 3D scaling and additive-blended particle bursts for a retro-future glow.
*   **Dynamic Audio Engine:** Professional mixing (50% BGM / 100% SFX) using the Windows Multimedia (`mmsystem.h`) backend.
*   **Stand-Alone Architectures:** Every game is a self-contained, high-performance C++ executable.

## 🛠️ Build & Installation

### Requirements
*   C++ Compiler (`g++` / MinGW)
*   OpenGL / FreeGLUT Libraries
*   Windows (Required for the `winmm` native audio engine).

### Compiling from Source
To compile any module, use the following commands in your terminal:

**Neo Racing:**
```powershell
g++ racing.cpp -o racing.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
```

**Neo Shooter:**
```powershell
g++ shooter.cpp -o shooter.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
```

### Audio Setup
The audio system expects specific sub-directories alongside the compiled `.exe`:

*   **For Racing**: Place the `audio/` folder containing `bgm.mp3`, `crash.wav`, and `pickup.wav`.
*   **For Shooter**: Place the `shooter music/` folder containing `bgm_new.mp3`, `fire_new.mp3`, `pickup_new.mp3`, and `crash_new.mp3`.

## ⌨️ Global Controls

*   **Arrow Keys**: Move ship / Steer car
*   **SPACE**: Fire Laser (Shooter Only)
*   **P**: **Pause / Resume** the game and music
*   **R**: Fast Restart / Respawn
*   **ESC**: Power off / Quit to Desktop

---
*Developed by @alamin. Optimized with AG-1 AI.*
