# TETRIS: NEURAL GRID

A professional, high-performance 3D Tetris implementation featuring an adaptive difficulty engine and competitive-grade rotation physics.

## 🕹️ Gameplay Mechanics

### 🧩 Super Rotation System (SRS)
The game uses the standard SRS wall-kick tables, allowing for tactical piece kicks and high-level maneuvers such as:
*   Wall Kicks
*   Floor Kicks
*   T-Spin Triple/Double detection

### 🧠 Neural Difficulty AI
The game monitors your **APM (Actions Per Minute)** and **PPS (Pieces Per Second)** in real-time. If you play too efficiently, the "Neural Grid" will aggressively increase gravity to challenge your reaction speed.

### ⏳ Neural Core Power-Ups
Randomly, a piece will contain a glowing **Neural Core**. Clearing a line with one of these blocks triggers a system-wide ability:
*   **CHRONOS DELAY**: Halves gravity for 15s (Ideal for recovery).
*   **NEURAL WIPE**: Instant vaporization of the bottom 3 lines.
*   **SYNAPTIC MULTIPLIER**: 3x Score and Combo gains for 12s.

## ⚡ Tech Specs
*   **Engine**: Custom raw C++ / OpenGL 3D Engine.
*   **Physics**: 60Hz DAS (Delayed Auto Shift) movement engine with 180ms startup and 40ms repeat rates.
*   **Audio**: Windows Multimedia (MCI) engine for seamless .MP3 looping.
*   **Aesthetics**: Glassmorphism HUD, additive particle explosions, and dynamic camera recoil.

## ⌨️ Controls
| Key | Action |
| :--- | :--- |
| **Arrow Left/Right** | Move Piece |
| **Arrow Up** | Rotate (SRS Kick) |
| **Arrow Down** | Soft Drop |
| **SPACE** | Hard Drop (Vertical Beam) |
| **P** | System Suspend (Pause) |
| **M** | Toggle Neural Soundtrack |
| **G** | Toggle Ghost Piece |
| **C / H** | Hold Piece |
| **R** | Initialize / Restart |
| **ESC** | Power Off |

## 🛠️ Build Requirements
*   **OS**: Windows (Required for `winmm` audio)
*   **Compiler**: `g++` (MinGW)
*   **Libraries**: GL, GLU, FreeGLUT, WinMM

### Build Command:
```powershell
g++ tetris.cpp -o tetris.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
```

---
*Developed by @alamin. Optimized with AG-1 AI.*
