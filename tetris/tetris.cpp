/*
 * ================================================================
 *  TETRIS: NEURAL GRID  –  OpenGL / FreeGLUT
 *  ================================================================
 *
 *  FEATURES
 *  ─────────────────────────────────────────────────────────────
 *  • Adaptive AI Difficulty (APM-based gravity)
 *  • Cyberpunk "Neural" HUD with Glassmorphism panels
 *  • Advanced Piece Management: Hold System & Next Queue (5)
 *  • Chaos Mode: Static/Dynamic random events (Tilt, Invert)
 *  • Advanced Scoring: Combo multipliers & Back-to-Back
 *  • Pro-Level Analytics: APM and PPS tracking
 *
 *  CONTROLS
 *  ─────────────────────────────────────────────────────────────
 *  Arrow Left/Right   Move piece (Inverted in Chaos)
 *  Arrow Up           Rotate piece
 *  Arrow Down         Soft drop
 *  SPACE              Hard drop / Start Game
 *  C / H              Hold piece
 *  R                  Restart game (Game Over)
 *  ESC                Quit
 */

#ifdef _WIN32
    #include <windows.h>
#endif
#include <GL/glut.h>
#include <GL/glu.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <ctime>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

// ──────────────────────────────────────────────────────────────
//  System Basics
// ──────────────────────────────────────────────────────────────
static int WIN_W = 1280;
static int WIN_H = 720;

static float frand(float lo, float hi) {
    return lo + (hi - lo) * (float)rand() / (float)RAND_MAX;
}

struct Vec3 {
    float x, y, z;
    Vec3(float x=0, float y=0, float z=0) : x(x), y(y), z(z) {}
};

// ──────────────────────────────────────────────────────────────
//  Constants & Shapes
// ──────────────────────────────────────────────────────────────
static const int BOARD_W = 10;
static const int BOARD_H = 20;
static const float BLOCK_SIZE = 1.0f;
static const float BOARD_W_UNITS = BOARD_W * BLOCK_SIZE;
static const float BOARD_H_UNITS = BOARD_H * BLOCK_SIZE;
static const float BOARD_X = -BOARD_W_UNITS / 2.0f;
static const float BOARD_Y = -BOARD_H_UNITS / 2.0f;

static const float PIECE_COLORS[8][3] = {
    {0,0,0}, {0,1,1}, {0,0,1}, {1,0.5,0}, {1,1,0}, {0,1,0}, {0.5,0,1}, {1,0,0}
};

static const int SHAPES[8][4][4][4] = {
    // 0: Empty
    { { {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0} } },
    // 1: I
    { { {0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0} }, { {0,0,1,0}, {0,0,1,0}, {0,0,1,0}, {0,0,1,0} }, { {0,0,0,0}, {0,0,0,0}, {1,1,1,1}, {0,0,0,0} }, { {0,1,0,0}, {0,1,0,0}, {0,1,0,0}, {0,1,0,0} } },
    // 2: J
    { { {2,0,0,0}, {2,2,2,0}, {0,0,0,0}, {0,0,0,0} }, { {0,2,2,0}, {0,2,0,0}, {0,2,0,0}, {0,0,0,0} }, { {0,0,0,0}, {2,2,2,0}, {0,0,2,0}, {0,0,0,0} }, { {0,2,0,0}, {0,2,0,0}, {2,2,0,0}, {0,0,0,0} } },
    // 3: L
    { { {0,0,3,0}, {3,3,3,0}, {0,0,0,0}, {0,0,0,0} }, { {0,3,0,0}, {0,3,0,0}, {0,3,3,0}, {0,0,0,0} }, { {0,0,0,0}, {3,3,3,0}, {3,0,0,0}, {0,0,0,0} }, { {3,3,0,0}, {0,3,0,0}, {0,3,0,0}, {0,0,0,0} } },
    // 4: O
    { { {0,4,4,0}, {0,4,4,0}, {0,0,0,0}, {0,0,0,0} }, { {0,4,4,0}, {0,4,4,0}, {0,0,0,0}, {0,0,0,0} }, { {0,4,4,0}, {0,4,4,0}, {0,0,0,0}, {0,0,0,0} }, { {0,4,4,0}, {0,4,4,0}, {0,0,0,0}, {0,0,0,0} } },
    // 5: S
    { { {0,5,5,0}, {5,5,0,0}, {0,0,0,0}, {0,0,0,0} }, { {0,5,0,0}, {0,5,5,0}, {0,0,5,0}, {0,0,0,0} }, { {0,0,0,0}, {0,5,5,0}, {5,5,0,0}, {0,0,0,0} }, { {5,0,0,0}, {5,5,0,0}, {0,5,0,0}, {0,0,0,0} } },
    // 6: T
    { { {0,6,0,0}, {6,6,6,0}, {0,0,0,0}, {0,0,0,0} }, { {0,6,0,0}, {0,6,6,0}, {0,6,0,0}, {0,0,0,0} }, { {0,0,0,0}, {6,6,6,0}, {0,6,0,0}, {0,0,0,0} }, { {0,6,0,0}, {6,6,0,0}, {0,6,0,0}, {0,0,0,0} } },
    // 7: Z
    { { {7,7,0,0}, {0,7,7,0}, {0,0,0,0}, {0,0,0,0} }, { {0,0,7,0}, {0,7,7,0}, {0,7,0,0}, {0,0,0,0} }, { {0,0,0,0}, {7,7,0,0}, {0,7,7,0}, {0,0,0,0} }, { {0,7,0,0}, {7,7,0,0}, {7,0,0,0}, {0,0,0,0} } }
};

// ──────────────────────────────────────────────────────────────
//  Visual Structures & State
// ──────────────────────────────────────────────────────────────
enum GameState { STATE_HOME, STATE_PLAY, STATE_PAUSE, STATE_GAMEOVER };

struct Particle { Vec3 pos, vel, ang, angVel; float life; float r, g, b; int type; };
struct Lightning { std::vector<Vec3> points; float life; float r, g, b; };
struct Star { Vec3 pos; float speed, brightness, twinkle, twinkleSpd; float r, g, b; };
struct GridLine { float pos, speed; };
struct Notification { char text[64]; float x, y, life, r, g, b, scale; };

static const int MAX_STARS = 300;
static const int GRID_LINES = 15;

static Star gStarArr[MAX_STARS];
static GridLine gNeuralGridV[GRID_LINES];
static GridLine gNeuralGridH[GRID_LINES];
static std::vector<Particle> gParticles;
static std::vector<Lightning> gLightnings;

static int gBoard[BOARD_H][BOARD_W];
static int gScore=0, gLines=0, gLevel=1, gHighScore=0;
static GameState gState = STATE_HOME;

static int   gCombo=0;
static float gComboTimer=0;
static bool  gBackToBack=false;
static float gAPM=0, gPPS=0, gPlayTime=0;
static int   gActionCount=0, gPieceCount=0;

static int   gPieceType=1, gPieceRot=0;
static float gPieceX=3.0f, gPieceY=19.0f;
static int   gHoldPiece=0;
static bool  gCanHold=true;
static std::vector<int> gNextQueue;
static const int QUEUE_SIZE = 5;

static float gFallTimer=0, gFallSpeed=1, gLockDelay=0, gGlobalTime=0;
static float gCameraTiltX=0, gCameraTiltY=0, gCameraShake=0, gScreenPulse=0;
static float gChaosTimer=0, gChaosRotation=0;
static int   gChaosEvent=0;

// Modern Locking Engine
static float gLockTimer = 0.0f;
static int   gMoveResetCount = 0;
static const float LOCK_DELAY_MAX = 0.5f;
static const int   MOVE_RESET_MAX = 15;

// HUD effects
static float gRippleTime = 0.0f;
static float gChaosWarningTimer = 0.0f;
static std::vector<Notification> gNotifications;
static bool gShowGhost = true;
static float gHardDropPulse = 0.0f;

// SRS Wall Kick Tables (dx, dy)
static const int SRS_KICKS_JLSTZ[8][5][2] = {
    {{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}}, // 0->1
    {{0,0}, {1,0}, {1,1}, {0,-2}, {1,-2}},   // 1->0
    {{0,0}, {1,0}, {1,-1}, {0,2}, {1,2}},    // 1->2
    {{0,0}, {-1,0}, {-1,-1}, {0,2}, {-1,2}}, // 2->1
    {{0,0}, {1,0}, {1,1}, {0,-2}, {1,-2}},   // 2->3
    {{0,0}, {-1,0}, {-1,1}, {0,-2}, {-1,-2}}, // 3->2
    {{0,0}, {-1,0}, {-1,-1}, {0,2}, {-1,2}}, // 3->0
    {{0,0}, {1,0}, {1,-1}, {0,2}, {1,2}}     // 0->3
};
static const int SRS_KICKS_I[8][5][2] = {
    {{0,0}, {-2,0}, { 1,0}, {-2,-1}, { 1,2}}, // 0->1
    {{0,0}, { 2,0}, {-1,0}, { 2,1}, {-1,-2}}, // 1->0
    {{0,0}, {-1,0}, { 2,0}, {-1,2}, { 2,-1}}, // 1->2
    {{0,0}, { 1,0}, {-2,0}, { 1,-2}, {-2,1}}, // 2->1
    {{0,0}, { 2,0}, {-1,0}, { 2,1}, {-1,-2}}, // 2->3
    {{0,0}, {-2,0}, { 1,0}, {-2,-1}, { 1,2}}, // 3->2
    {{0,0}, { 1,0}, {-2,0}, { 1,-2}, {-2,1}}, // 3->0
    {{0,0}, {-1,0}, { 2,0}, {-1,2}, { 2,-1}}  // 0->3
};

static int getSRSIndex(int from, int to) {
    if(from==0 && to==1) return 0; if(from==1 && to==0) return 1;
    if(from==1 && to==2) return 2; if(from==2 && to==1) return 3;
    if(from==2 && to==3) return 4; if(from==3 && to==2) return 5;
    if(from==3 && to==0) return 6; if(from==0 && to==3) return 7;
    return 0;
}

// Control DAS (Delayed Auto Shift)
static bool  gSpecialKeys[256];
static float gDASWait = 0.0f, gARRTimer = 0.0f;
static const float DAS_DELAY = 0.18f;
static const float ARR_INTERVAL = 0.04f;

// 3D Physics Momentum
static float gRecoilY = 0.0f, gRecoilVelY = 0.0f;
static float gTiltX = 0.0f, gTiltY = 0.0f;
static float gTargetTiltX = 0.0f, gTargetTiltY = 0.0f;

// Neural Power-Ups
enum PowerType { PWR_NONE, PWR_SLOW, PWR_WIPE, PWR_SCORE };
static PowerType gActivePower = PWR_NONE;
static float     gPowerTimer = 0.0f;
static int       gPieceSpecial = -1; // Index 0-3 of the block in current piece that is a "core"

// ──────────────────────────────────────────────────────────────
//  Background & Logic Functions
// ──────────────────────────────────────────────────────────────
static void initStars() {
    for (int i=0; i<MAX_STARS; i++) {
        Star& s = gStarArr[i];
        // Mix of background stars and "bit-stream" rising particles
        if (i < 100) {
            s.pos = Vec3(frand(-40,40), frand(-30,30), frand(-40,-20));
            s.speed = frand(0.1f, 0.4f); // background drift
        } else {
            s.pos = Vec3(frand(-25,25), frand(-30,30), frand(-15,-5));
            s.speed = frand(2.0f, 5.0f); // Fast bit-stream
        }
        s.brightness = frand(0.3f, 1.0f);
        s.twinkle = frand(0, 6.28f); s.twinkleSpd = frand(0.5f, 3.0f);
        s.r = 0.5f; s.g = 0.8f; s.b = 1.0f;
    }
    for (int i=0; i<GRID_LINES; i++) {
        gNeuralGridV[i].pos = frand(-30, 30); gNeuralGridV[i].speed = frand(0.5f, 2.0f);
        gNeuralGridH[i].pos = frand(-30, 30); gNeuralGridH[i].speed = frand(0.5f, 2.0f);
    }
}

static void notify(const char* txt, float r, float g, float b, float sc=0.18f) {
    Notification n; snprintf(n.text, 64, "%s", txt);
    n.x = (float)WIN_W/2; n.y = (float)WIN_H/2 + 50; n.life = 1.5f; n.r=r; n.g=g; n.b=b; n.scale=sc;
    gNotifications.push_back(n);
}

static void updateFX(float dt) {
    for (auto it = gNotifications.begin(); it != gNotifications.end(); ) {
        it->life -= dt; it->y += 30.0f * dt;
        if (it->life <= 0) it = gNotifications.erase(it); else ++it;
    }
    gHardDropPulse = std::max(0.0f, gHardDropPulse - dt*5.0f);
    for (int i=0; i<MAX_STARS; i++) {
        gStarArr[i].pos.y -= gStarArr[i].speed * dt;
        gStarArr[i].twinkle += gStarArr[i].twinkleSpd * dt;
        if (gStarArr[i].pos.y < -30) { gStarArr[i].pos.y = 30; gStarArr[i].pos.x = frand(-30,30); }
    }
    for (int i=0; i<GRID_LINES; i++) {
        gNeuralGridV[i].pos += gNeuralGridV[i].speed * dt; if (gNeuralGridV[i].pos > 30) gNeuralGridV[i].pos = -30;
        gNeuralGridH[i].pos += gNeuralGridH[i].speed * dt; if (gNeuralGridH[i].pos > 30) gNeuralGridH[i].pos = -30;
    }
    for (auto it = gLightnings.begin(); it != gLightnings.end(); ) {
        it->life -= dt; if (it->life <= 0) it = gLightnings.erase(it); else ++it;
    }
    gComboTimer = std::max(0.0f, gComboTimer - dt);
    if (gComboTimer <= 0) gCombo = 0;
    gScreenPulse *= 0.9f; gCameraShake *= 0.8f;
}

static void drawNeuralGrid() {
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    // Background Data Stream (Starfield)
    for (int i=0; i<MAX_STARS; i++) {
        const Star& s = gStarArr[i];
        float b = s.brightness * (0.5f + 0.5f*sinf(s.twinkle));
        glColor4f(s.r * b, s.g * b, s.b * b, b * 0.8f);
        if (i >= 100) { // Bit-stream look
             glLineWidth(1.0f);
             glBegin(GL_LINES);
             glVertex3f(s.pos.x, s.pos.y, s.pos.z);
             glVertex3f(s.pos.x, s.pos.y + 1.5f, s.pos.z);
             glEnd();
        } else {
             glPointSize(1.5f);
             glBegin(GL_POINTS);
             glVertex3f(s.pos.x, s.pos.y, s.pos.z);
             glEnd();
        }
    }

    // The Neural Cage
    glColor4f(0.0f, 0.4f, 1.0f, 0.05f);
    glBegin(GL_QUADS);
    // Back wall
    glVertex3f(BOARD_X, BOARD_Y, -0.6f); glVertex3f(BOARD_X+BOARD_W, BOARD_Y, -0.6f);
    glVertex3f(BOARD_X+BOARD_W, BOARD_Y+BOARD_H, -0.6f); glVertex3f(BOARD_X, BOARD_Y+BOARD_H, -0.6f);
    // Left/Right walls
    glColor4f(0.0f, 0.3f, 0.8f, 0.03f);
    glVertex3f(BOARD_X, BOARD_Y, -0.6f); glVertex3f(BOARD_X, BOARD_Y+BOARD_H, -0.6f);
    glVertex3f(BOARD_X, BOARD_Y+BOARD_H, 1.0f); glVertex3f(BOARD_X, BOARD_Y, 1.0f);
    glVertex3f(BOARD_X+BOARD_W, BOARD_Y, -0.6f); glVertex3f(BOARD_X+BOARD_W, BOARD_Y+BOARD_H, -0.6f);
    glVertex3f(BOARD_X+BOARD_W, BOARD_Y+BOARD_H, 1.0f); glVertex3f(BOARD_X+BOARD_W, BOARD_Y, 1.0f);
    glEnd();

    // Scrolling Circuit Patterns on walls
    float offset = gGlobalTime * 0.5f;
    glColor4f(0.0f, 0.8f, 1.0f, 0.2f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i=0; i<BOARD_H; i++) {
        float y = BOARD_Y + fmodf(i + offset, (float)BOARD_H);
        glVertex3f(BOARD_X, y, -0.6f); glVertex3f(BOARD_X+BOARD_W, y, -0.6f);
    }
    glEnd();
    
    // Grid Ripples
    if (gRippleTime > 0) {
        float ry = BOARD_Y + (1.0f - gRippleTime) * BOARD_H;
        glColor4f(1, 1, 1, gRippleTime * 0.5f);
        glLineWidth(3.0f);
        glBegin(GL_LINES);
        glVertex3f(BOARD_X, ry, 0.1f); glVertex3f(BOARD_X+BOARD_W, ry, 0.1f);
        glEnd();
    }

    // Lightning fx
    glLineWidth(2.5f);
    for (auto& ln : gLightnings) {
        float alpha = ln.life / 0.5f; glColor4f(ln.r, ln.g, ln.b, alpha);
        glBegin(GL_LINE_STRIP); for (auto& p : ln.points) glVertex3f(p.x, p.y, p.z); glEnd();
    }
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

static void spawnLightning(float x, float y) {
    Lightning ln; ln.r=0; ln.g=0.8f; ln.b=1; ln.life=0.5f; Vec3 cur(x,y,0.5f); ln.points.push_back(cur);
    for(int i=0; i<6; i++) { cur.x+=frand(-1.5f,1.5f); cur.y+=frand(0.5f,2.5f); ln.points.push_back(cur); }
    gLightnings.push_back(ln);
}

static void refillQueue() {
    while (gNextQueue.size() < QUEUE_SIZE + 2) {
        int bag[7] = { 1, 2, 3, 4, 5, 6, 7 };
        for (int i = 0; i < 7; i++) std::swap(bag[i], bag[rand() % 7]);
        for (int i = 0; i < 7; i++) gNextQueue.push_back(bag[i]);
    }
}

// ──────────────────────────────────────────────────────────────
//  Persistence & Logic
// ──────────────────────────────────────────────────────────────
static void saveHighScore() { FILE* f = fopen("highscore.dat", "w"); if(f){ fprintf(f, "%d", gHighScore); fclose(f); } }
static void loadHighScore() { FILE* f = fopen("highscore.dat", "r"); if(f){ if(fscanf(f, "%d", &gHighScore) != 1) gHighScore=0; fclose(f); } }

static bool checkCollision(float px, float py, int rot) {
    int ix=(int)std::floor(px), iy=(int)std::floor(py);
    for(int y=0; y<4; y++) for(int x=0; x<4; x++) if(SHAPES[gPieceType][rot][y][x]) {
        int cx=ix+x, cy=iy-y;
        if(cx<0 || cx>=BOARD_W || cy<0) return true;
        if(cy<BOARD_H && gBoard[cy][cx]) return true;
    }
    return false;
}

static void endGame() { gState = STATE_GAMEOVER; if(gScore>gHighScore){ gHighScore=gScore; saveHighScore(); } }

static void spawnPiece() {
    refillQueue(); gPieceType = gNextQueue[0]; gNextQueue.erase(gNextQueue.begin());
    gPieceRot=0; gPieceX=3.0f; gPieceY=(float)(BOARD_H-1);
    gFallTimer=0; gLockDelay=0; gLockTimer=0; gMoveResetCount=0; gCanHold=true; gPieceCount++;
    
    // 15% chance for a Neural Core
    gPieceSpecial = (frand(0,1) < 0.15f) ? (rand() % 4) : -1;

    float base = 1.0f + (gLevel-1)*0.5f; 
    float fallMult = (gActivePower == PWR_SLOW) ? 0.3f : 1.0f;
    float apmB = (gAPM/50.0f)*0.5f; gFallSpeed = (base + apmB) * fallMult;
    if(checkCollision(gPieceX, gPieceY, gPieceRot)) endGame();
}

static void holdPiece() {
    if(!gCanHold) return; gActionCount++;
    int target = (gHoldPiece == 0) ? gNextQueue[0] : gHoldPiece;
    // Safety check: can the held piece fit at spawn?
    if (checkCollision(3.0f, (float)(BOARD_H-1), 0)) {
        // If it can't fit, we don't swap (prevents instant game over inconsistency)
        return;
    }

    if(gHoldPiece==0) { gHoldPiece=gPieceType; spawnPiece(); }
    else { 
        int tmp=gPieceType; gPieceType=gHoldPiece; gHoldPiece=tmp; 
        gPieceRot=0; gPieceX=3.0f; gPieceY=(float)(BOARD_H-1); gFallTimer=0; 
        gLockTimer=0; gMoveResetCount=0;
    }
    gCanHold=false;
}

static void resetGame() {
    memset(gBoard, 0, sizeof(gBoard)); gScore=0; gLines=0; gLevel=1; gState=STATE_PLAY;
    gParticles.clear(); gLightnings.clear(); gNextQueue.clear(); 
    gHoldPiece=0; gCanHold=true; gCombo=0; gComboTimer=0; gActionCount=0; gPieceCount=0; gPlayTime=0;
    refillQueue(); spawnPiece();
}

static void spawnParticles(int x, int y, int type) {
    float wx = BOARD_X + x + 0.5f, wy = BOARD_Y + y + 0.5f;
    for(int i=0; i<8; i++) {
        Particle p; 
        p.pos = Vec3(wx + frand(-0.3f,0.3f), wy + frand(-0.3f,0.3f), frand(0.0f,1.0f)); 
        p.vel = Vec3(frand(-1.5f,1.5f), frand(1.0f,4.0f), frand(1.0f,3.0f));
        p.ang = Vec3(frand(0,360), frand(0,360), frand(0,360));
        p.angVel = Vec3(frand(-200,200), frand(-200,200), frand(-200,200));
        p.r = PIECE_COLORS[type][0]; p.g = PIECE_COLORS[type][1]; p.b = PIECE_COLORS[type][2]; 
        p.life = 2.0f; p.type = 1; // 1 = Physics Cube
        gParticles.push_back(p);
    }
}

static void lockPiece() {
    int ix=(int)std::floor(gPieceX), iy=(int)std::floor(gPieceY);
    
    // T-Spin Check (3-Corner Rule)
    bool isTSpin = false;
    if(gPieceType == 6) { // T-Piece
        int corners = 0;
        int cx[]={0,2,0,2}, cy[]={0,0,-2,-2};
        for(int i=0; i<4; i++) {
            int tx=ix+cx[i], ty=iy+cy[i];
            if(tx<0 || tx>=BOARD_W || ty<0 || ty>=BOARD_H || gBoard[ty][tx]) corners++;
        }
        if(corners >= 3) isTSpin = true;
    }

    int bCount = 0;
    for(int y=0; y<4; y++) for(int x=0; x<4; x++) if(SHAPES[gPieceType][gPieceRot][y][x]) {
        int cx=ix+x, cy=iy-y; 
        if(cy>=0 && cy<BOARD_H && cx>=0 && cx < BOARD_W) {
            // If this block was the special core, store it as type+10
            gBoard[cy][cx] = (bCount == gPieceSpecial) ? (gPieceType + 10) : gPieceType;
        }
        bCount++;
    }
    gCameraShake=0.2f; int lnC=0;
    bool powerTriggered = false;
    for(int y=0; y<BOARD_H; y++) {
        bool full=true; bool rowHasPower = false;
        for(int x=0; x<BOARD_W; x++) {
            if(!gBoard[y][x]) { full=false; break; }
            if(gBoard[y][x] > 10) rowHasPower = true;
        }
        if(full) {
            lnC++; if(gCombo>0) spawnLightning(BOARD_X+5, BOARD_Y+y);
            if(rowHasPower) powerTriggered = true;
            for(int x=0; x<BOARD_W; x++) spawnParticles(x, y, gBoard[y][x]%10);
            for(int yy=y; yy<BOARD_H-1; yy++) for(int x=0; x<BOARD_W; x++) gBoard[yy][x]=gBoard[yy+1][x];
            for(int x=0; x<BOARD_W; x++) gBoard[BOARD_H-1][x]=0; y--;
        }
    }
    
    if (powerTriggered) {
        int r = rand() % 3;
        if (r == 0) { gActivePower = PWR_SLOW; gPowerTimer = 15.0f; }
        else if (r == 1) { // Neural Wipe (Immediate)
            for (int i=0; i<3; i++) {
                for(int x=0; x<BOARD_W; x++) if(gBoard[0][x]) spawnParticles(x, 0, gBoard[0][x]%10);
                for(int yy=0; yy<BOARD_H-1; yy++) for(int x=0; x<BOARD_W; x++) gBoard[yy][x]=gBoard[yy+1][x];
                for(int x=0; x<BOARD_W; x++) gBoard[BOARD_H-1][x]=0;
            }
            gScreenPulse = 1.0f;
        } else { gActivePower = PWR_SCORE; gPowerTimer = 12.0f; }
    }

    if(lnC>0) {
        gLines+=lnC; int oldL = gLevel; gLevel=1+gLines/10;
        if(gLevel > oldL) notify("LEVEL UP", 1, 0.8f, 0);
        
        gRippleTime = 1.0f; 
        gRecoilVelY -= 8.0f; 
        int pts[]={0,100,300,500,800}; 
        int mult = (gActivePower == PWR_SCORE) ? 3 : 1;
        
        if(lnC==4) { gScreenPulse=1.0f; if(gBackToBack) mult *= 1.5; gBackToBack=true; notify("TETRIS!", 0, 0.8f, 1); } 
        else gBackToBack=false;
        
        if(isTSpin) { mult *= 2; notify("T-SPIN!", 1, 0, 1); }

        gCombo++; gComboTimer=3.0f; 
        if(gCombo > 1) { char cbuf[32]; snprintf(cbuf, 32, "COMBO X%d", gCombo); notify(cbuf, 1, 1, 0, 0.15f); }

        gScore += (pts[lnC]*gLevel*mult) + gCombo*50*mult;
        gCameraShake = 0.5f + lnC*0.2f;
    } else {
        if(isTSpin) notify("T-SPIN", 1, 0.5f, 1, 0.12f);
        gRecoilVelY -= 3.0f; 
    }
    spawnPiece();
}

static int getGhostY() { int gy=(int)gPieceY; while(!checkCollision(gPieceX, gy-1, gPieceRot)) gy--; return gy; }

// ──────────────────────────────────────────────────────────────
//  Drawing Helpers
// ──────────────────────────────────────────────────────────────
static void bitmapAt(float px, float py, const char* s, void* font=GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(px, py); for (const char* c = s; *c; c++) glutBitmapCharacter(font, *c);
}
static int bitmapWidth(const char* s, void* font) {
    int w = 0; for (const char* c = s; *c; c++) w += glutBitmapWidth(font, *c); return w;
}
static void bitmapCentered(float cx, float py, const char* s, void* font=GLUT_BITMAP_HELVETICA_18) {
    bitmapAt(cx - bitmapWidth(s, font) * 0.5f, py, s, font);
}
static void strokeCentered(float cx, float cy, const char* s, float scale, float r, float g, float b, float a) {
    float tw = 0; for (const char* c = s; *c; c++) tw += glutStrokeWidth(GLUT_STROKE_ROMAN, *c);
    float startX = cx - tw * scale * 0.5f;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(r, g, b, a * 0.3f); glLineWidth(6.0f);
    glPushMatrix(); glTranslatef(startX, cy, 0); glScalef(scale, scale, scale);
    for (const char* c = s; *c; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    glPopMatrix();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glColor4f(1, 1, 1, a); glLineWidth(2.5f);
    glPushMatrix(); glTranslatef(startX, cy, 0); glScalef(scale, scale, scale);
    for (const char* c = s; *c; c++) glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
    glPopMatrix(); glLineWidth(1.0f); glDisable(GL_BLEND);
}

static void drawCube(float x, float y, float z, float size, const float* col, bool ghost=false, Vec3 rot = Vec3(0,0,0), bool isCore = false) {
    glPushMatrix(); 
    glTranslatef(x, y, z);
    if (rot.x != 0 || rot.y != 0 || rot.z != 0) {
        glRotatef(rot.x, 1, 0, 0); glRotatef(rot.y, 0, 1, 0); glRotatef(rot.z, 0, 0, 1);
    }
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (ghost) { glColor4f(col[0], col[1], col[2], 0.25f); glutWireCube(size * 0.95f); }
    else {
        GLfloat df[]={col[0]*0.8f, col[1]*0.8f, col[2]*0.8f, 1}, am[]={col[0]*0.3f, col[1]*0.3f, col[2]*0.3f, 1};
        glMaterialfv(GL_FRONT, GL_DIFFUSE, df); glMaterialfv(GL_FRONT, GL_AMBIENT, am); glutSolidCube(size * 0.92f);
        glDisable(GL_LIGHTING); glColor4f(col[0], col[1], col[2], 0.8f); glLineWidth(2.0f); glutWireCube(size * 0.96f);
        
        // Spinning Neural Core
        if (isCore) {
            glPushMatrix(); glRotatef(gGlobalTime*240.0f, 0,1,1);
            glColor4f(1, 1, 1, 0.9f); glutWireSphere(size*0.35f, 8, 8);
            glColor4f(col[0], col[1], col[2], 0.4f); glutSolidSphere(size*0.25f, 10, 10);
            glPopMatrix();
        }
        glEnable(GL_LIGHTING);
    }
    glPopMatrix();
}

static void drawGlassPanel(float x, float y, float w, float h, const char* title) {
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.02f, 0.05f, 0.12f, 0.75f); glBegin(GL_QUADS); glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h); glEnd();
    glColor4f(0, 0.6f, 1, 0.5f); glLineWidth(2.0f); glBegin(GL_LINE_LOOP); glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h); glEnd();
    glColor4f(0, 0.6f, 1, 0.2f); glBegin(GL_QUADS); glVertex2f(x,y+h-25); glVertex2f(x+w,y+h-25); glVertex2f(x+w,y+h); glVertex2f(x,y+h); glEnd();
    glColor3f(0, 0.9f, 1); bitmapCentered(x+w/2, y+h-18, title, GLUT_BITMAP_HELVETICA_12);
}

static void drawPiecePreview(float x, float y, int type, float sc=20.0f) {
    if(type<=0) return;
    for(int j=0; j<4; j++) for(int i=0; i<4; i++) if(SHAPES[type][0][j][i]) {
        float px=x+i*sc, py=y-j*sc; glColor3fv(PIECE_COLORS[type]);
        glBegin(GL_QUADS); glVertex2f(px,py); glVertex2f(px+sc-1,py); glVertex2f(px+sc-1,py+sc-1); glVertex2f(px,py+sc-1); glEnd();
    }
}

// ──────────────────────────────────────────────────────────────
//  Core Displays
// ──────────────────────────────────────────────────────────────
static void drawHomeScreen() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, WIN_W, 0, WIN_H, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity(); glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glColor4f(0,0,0.05f, 0.85f); glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(WIN_W,0); glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H); glEnd();
    float cx=WIN_W*0.5f, p=0.8f+0.2f*sinf(gGlobalTime*3.0f);
    strokeCentered(cx, WIN_H*0.65f, "NEURAL GRID", 0.38f, 0, 0.8f, 1, p);
    strokeCentered(cx, WIN_H*0.52f, "TETRIS 3D", 0.55f, 1, 0.1f, 0.4f, p);
    char buf[128]; snprintf(buf, 128, "NEURAL RANK: %s", (gHighScore>50000?"DIAMOND":(gHighScore>20000?"GOLD" : "BRONZE")));
    strokeCentered(cx, WIN_H*0.38f, buf, 0.20f, 0.4f, 1, 0.8f, 0.9f);
    snprintf(buf, 128, "SYSTEM BEST: %06d", gHighScore); bitmapCentered(cx, WIN_H*0.32f, buf, GLUT_BITMAP_HELVETICA_18);
    float bl=0.5f+0.5f*sinf(gGlobalTime*6.0f); glColor3f(bl, bl, bl); bitmapCentered(cx, WIN_H*0.22f, "[ PRESS SPACE TO INITIALIZE ]", GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0.3f, 0.4f, 0.6f); bitmapCentered(cx, WIN_H*0.08f, "Developed by AL AMIN HOSSAIN", GLUT_BITMAP_HELVETICA_18);
    glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW); glPopMatrix();
}

static void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    float fov = 50.0f - gScreenPulse * 15.0f; gluPerspective(fov, (double)WIN_W/WIN_H, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    float sx=gCameraShake*frand(-1,1), sy=gCameraShake*frand(-1,1);
    gluLookAt(gCameraTiltX+sx, gCameraTiltY+sy + gRecoilY, 32, 0, gRecoilY, 0, 0, 1, 0);
    
    // Applying Board Physics Tilt
    glRotatef(gTiltX, 0, 1, 0); glRotatef(gTiltY, 1, 0, 0);
    
    drawNeuralGrid();
    GLfloat lp[]={0,10,20,1}; glLightfv(GL_LIGHT0, GL_POSITION, lp); glEnable(GL_LIGHT0); glEnable(GL_LIGHTING); glEnable(GL_DEPTH_TEST);
    if(gState==STATE_HOME){ drawHomeScreen(); glutSwapBuffers(); return; }
    // Glowing Outer Frame (Neural Style)
    float p=0.7f+0.3f*sinf(gGlobalTime*4); 
    if (gChaosWarningTimer > 0) {
        float warnPulse = 0.5f + 0.5f * sinf(gGlobalTime * 15.0f);
        glColor4f(1.0f, 0.2f, 0.0f, 0.8f * warnPulse);
    } else {
        glColor4f(0, 0.8f, 1, 0.8f*p); 
    }
    glLineWidth(4);
    glBegin(GL_LINE_LOOP); glVertex3f(BOARD_X, BOARD_Y, -0.5f); glVertex3f(BOARD_X+BOARD_W, BOARD_Y, -0.5f); glVertex3f(BOARD_X+BOARD_W, BOARD_Y+BOARD_H, -0.5f); glVertex3f(BOARD_X, BOARD_Y+BOARD_H, -0.5f); glEnd(); 
    glEnable(GL_LIGHTING);
    for(int y=0; y<BOARD_H; y++) for(int x=0; x<BOARD_W; x++) if(gBoard[y][x]) {
        drawCube(BOARD_X+x+0.5f, BOARD_Y+y+0.5f, 0, 1, PIECE_COLORS[gBoard[y][x]%10], false, Vec3(0,0,0), gBoard[y][x] > 10);
    }
    if(gState==STATE_PLAY || gState==STATE_PAUSE){
        int gx=(int)std::floor(gPieceX), gyL=getGhostY();
        int bCount = 0;
        for(int y=0; y<4; y++) for(int x=0; x<4; x++) if(SHAPES[gPieceType][gPieceRot][y][x]) {
            if(gShowGhost) drawCube(BOARD_X+(gx+x)+0.5f, BOARD_Y+(gyL-y)+0.5f, 0, 1, PIECE_COLORS[gPieceType], true);
            float yO=(gLockDelay>0)?sinf(gGlobalTime*20)*0.05f:0;
            drawCube(BOARD_X+(gx+x)+0.5f, BOARD_Y+(gPieceY-y)+0.5f+yO, 0, 1, PIECE_COLORS[gPieceType], false, Vec3(0,0,0), bCount == gPieceSpecial);
            bCount++;
        }
        
        // Hard Drop Beam Pulse
        if(gHardDropPulse > 0) {
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glColor4f(1, 1, 1, gHardDropPulse * 0.4f);
            glBegin(GL_QUADS);
            glVertex3f(BOARD_X+gx, BOARD_Y, 0.1f); glVertex3f(BOARD_X+gx+4, BOARD_Y, 0.1f);
            glVertex3f(BOARD_X+gx+4, BOARD_Y+BOARD_H, 0.1f); glVertex3f(BOARD_X+gx, BOARD_Y+BOARD_H, 0.1f);
            glEnd();
        }
    }
    glDisable(GL_LIGHTING); 
    for(auto& prt : gParticles){ 
        if(prt.type == 1) drawCube(prt.pos.x, prt.pos.y, prt.pos.z, 0.4f, &prt.r, false, prt.ang);
        else { glPointSize(4); glBegin(GL_POINTS); glColor4f(prt.r, prt.g, prt.b, prt.life); glVertex3f(prt.pos.x, prt.pos.y, prt.pos.z); glEnd(); }
    }

    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, WIN_W, 0, WIN_H, -1, 1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity(); glDisable(GL_DEPTH_TEST);
    drawGlassPanel(50, WIN_H-180, 120, 120, "HOLD"); drawPiecePreview(75, WIN_H-100, gHoldPiece, 18);
    drawGlassPanel(50, WIN_H-420, 120, 220, "NEURAL STATS"); char buf[128]; glColor3f(1,1,1);
    snprintf(buf, 128, "LEVEL: %d", gLevel); bitmapAt(65, WIN_H-230, buf, GLUT_BITMAP_HELVETICA_12);
    snprintf(buf, 128, "LINES: %d", gLines); bitmapAt(65, WIN_H-260, buf, GLUT_BITMAP_HELVETICA_12);
    snprintf(buf, 128, "APM: %.1f", gAPM); bitmapAt(65, WIN_H-290, buf, GLUT_BITMAP_HELVETICA_12);
    snprintf(buf, 128, "PPS: %.2f", gPPS); bitmapAt(65, WIN_H-320, buf, GLUT_BITMAP_HELVETICA_12);

    if (gActivePower != PWR_NONE) {
        const char* pTxt[] = {"", "CHRONOS DR.", "NEURAL WIPE", "SYNC MULT."};
        drawGlassPanel(50, WIN_H-560, 120, 60, "ACTIVE CORE");
        glColor3f(0.4f, 1, 0.8f); snprintf(buf, 128, "%s", pTxt[gActivePower]);
        bitmapCentered(110, WIN_H-520, buf, GLUT_BITMAP_HELVETICA_10);
        snprintf(buf, 128, "%.1fS", gPowerTimer);
        bitmapCentered(110, WIN_H-540, buf, GLUT_BITMAP_HELVETICA_12);
    }
    drawGlassPanel(WIN_W-170, WIN_H-580, 120, 520, "NEXT QUEUE");
    for(int i=0; i<5 && i<(int)gNextQueue.size(); i++) drawPiecePreview(WIN_W-145, WIN_H-100-i*90, gNextQueue[i], 16);
    
    // Repositioned Score HUD (Fixing overlap)
    drawGlassPanel(WIN_W/2-110, WIN_H-75, 220, 60, "NEURAL SCORE"); 
    glColor3f(1,1,0); snprintf(buf, 128, "%07d", gScore); bitmapCentered(WIN_W/2, WIN_H-55, buf, GLUT_BITMAP_TIMES_ROMAN_24);
    
    if(gCombo>1){ float cA=std::min(1.0f, gComboTimer); snprintf(buf, 128, "COMBO X%d", gCombo); strokeCentered(WIN_W/2, WIN_H/2, buf, 0.2f, 0, 0.9f, 1, cA); }

    // Floating Notifications
    for(auto& n : gNotifications) {
        strokeCentered(n.x, n.y, n.text, n.scale, n.r, n.g, n.b, std::min(1.0f, n.life*2.0f));
    }
    
    // Chaos Warning Flash
    if (gChaosWarningTimer > 0) {
        float warnAlpha = 0.4f + 0.4f * sinf(gGlobalTime * 20.0f);
        strokeCentered(WIN_W / 2, WIN_H / 2, "NEURAL INTERFERENCE DETECTED", 0.25f, 1.0f, 0.2f, 0.0f, warnAlpha);
    }

    if (gState == STATE_PAUSE) {
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0, 0, 0.1f, 0.7f); glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(WIN_W,0); glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H); glEnd();
        drawGlassPanel(WIN_W/2-160, WIN_H/2-50, 320, 100, "SYSTEM SUSPENDED");
        glColor3f(1,1,1); bitmapCentered(WIN_W/2, WIN_H/2-10, "NEURAL SIMULATION PAUSED", GLUT_BITMAP_HELVETICA_12);
        float bl = 0.5f + 0.5f * sinf(gGlobalTime * 5.0f); glColor3f(bl, bl, 1);
        bitmapCentered(WIN_W/2, WIN_H/2-35, "[ PRESS 'P' TO RESUME ]", GLUT_BITMAP_HELVETICA_10);
    }
    
    if(gState==STATE_GAMEOVER){
        float t=gGlobalTime, cx=WIN_W*0.5f, bg=0.03f+0.02f*sinf(t*1.2f);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(bg*0.25f, 0, bg*2, 0.88f); glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(WIN_W,0); glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H); glEnd();
        glColor4f(0.02f, 0, 0.07f, 0.72f); float px1=cx-285, px2=cx+285, py1=WIN_H*0.18f, py2=WIN_H*0.84f;
        glBegin(GL_QUADS); glVertex2f(px1,py1); glVertex2f(px2,py1); glVertex2f(px2,py2); glVertex2f(px1,py2); glEnd();
        float la=0.55f+0.45f*sinf(t*3); glColor4f(1, 0.12f, 0, la); glLineWidth(2);
        glBegin(GL_LINE_LOOP); glVertex2f(px1,py1); glVertex2f(px2,py1); glVertex2f(px2,py2); glVertex2f(px1,py2); glEnd();
        glBegin(GL_LINES); glVertex2f(px1+20, WIN_H*0.68f); glVertex2f(px2-20, WIN_H*0.68f); glVertex2f(px1+20, WIN_H*0.30f); glVertex2f(px2-20, WIN_H*0.30f); glEnd();
        strokeCentered(cx, WIN_H*0.72f, "MISSION FAILED", 0.26f, 1, 0, 0, 0.88f+0.12f*sinf(t*4));
        glColor3f(1, 0.85f, 0); snprintf(buf, 128, "FINAL SCORE : %06d", gScore); bitmapCentered(cx, WIN_H*0.55f, buf, GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.4f, 0.8f, 1); snprintf(buf, 128, "LINES CLEARED : %d", gLines); bitmapCentered(cx, WIN_H*0.48f, buf);
        snprintf(buf, 128, "PEAK APM : %.1f", gAPM); bitmapCentered(cx, WIN_H*0.41f, buf);
        float bl=0.55f+0.45f*sinf(t*2.5f); glColor4f(bl, bl, bl, 1); bitmapCentered(cx, WIN_H*0.24f, "[ Press R to Play Again ]");
        glColor4f(0.12f, 0.02f, 0, 0.78f); glBegin(GL_QUADS); glVertex2f(0,30); glVertex2f(WIN_W,30); glVertex2f(WIN_W,70); glVertex2f(0,70); glEnd();
        float cr=1, cg=0.5f+0.3f*sinf(t*2); glColor4f(cr, cg, 0, 0.9f); glLineWidth(2); glBegin(GL_LINES); glVertex2f(0,70); glVertex2f(WIN_W,70); glEnd();
        bitmapCentered(cx, 45, "Developed by AL AMIN HOSSAIN");
    }
    glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW); glPopMatrix(); glEnable(GL_DEPTH_TEST); glutSwapBuffers();
}

static void updatePhysics(int){
    float dt=1.0f/60.0f; gGlobalTime += dt;
    if(gState == STATE_PAUSE) { glutPostRedisplay(); glutTimerFunc(16, updatePhysics, 0); return; }
    
    updateFX(dt);
    if(gActivePower != PWR_NONE) {
        gPowerTimer -= dt; if(gPowerTimer <= 0) { gActivePower = PWR_NONE; spawnPiece(); }
    }

    if(gState==STATE_PLAY){
        gPlayTime+=dt; 
        
        // DAS Engine Logic (Fixes Arrow Problem)
        int dm=1; 
        bool left = gSpecialKeys[GLUT_KEY_LEFT], right = gSpecialKeys[GLUT_KEY_RIGHT];
        if(left || right) {
            if(gDASWait <= 0) {
                int dir = left ? -dm : dm;
                if(gARRTimer <= 0) {
                    if(!checkCollision(gPieceX+dir, gPieceY, gPieceRot)) { 
                        gPieceX += (float)dir; gTargetTiltX = (float)dir*5.0f; gARRTimer = ARR_INTERVAL;
                    }
                } else gARRTimer -= dt;
            } else gDASWait -= dt;
        } else { gDASWait = 0; gARRTimer = 0; }
        
        // Chaos Mode Transition Logic
        gChaosTimer -= dt;
        if (gChaosTimer <= 3.0f && gChaosTimer > 0) gChaosWarningTimer = 1.0f; // Visual warning
        else gChaosWarningTimer = 0.0f;

        if(gChaosTimer<=0){ 
            gChaosTimer=frand(15,25); gChaosEvent=rand()%2; // Reduced to 2 events (Speed and Gravity)
            gScreenPulse = 0.5f; // Transition flash
        }

        // 3D Physics Rebound / Tilt Update
        gRecoilY += gRecoilVelY * dt;
        gRecoilVelY += (-180.0f * gRecoilY - 12.0f * gRecoilVelY) * dt; // Spring-damper for landing
        
        gTiltX += (gTargetTiltX - gTiltX) * dt * 5.0f;
        gTiltY += (gTargetTiltY - gTiltY) * dt * 5.0f;
        gTargetTiltX *= 0.9f; gTargetTiltY *= 0.9f;

        if(gPlayTime>1){ gAPM=(gActionCount/gPlayTime)*60; gPPS=gPieceCount/gPlayTime; }
        gCameraTiltX=sinf(gGlobalTime*0.5f)*2; gCameraTiltY=cosf(gGlobalTime*0.7f)*2;
        
        float curSpd=gFallSpeed; if(gChaosEvent==2) curSpd*=2.0f; gFallTimer+=dt*curSpd;
        
        bool onGround = checkCollision(gPieceX, gPieceY - 0.1f, gPieceRot);

        if(gFallTimer >= 1.0f){
            gFallTimer -= 1.0f;
            if(!checkCollision(gPieceX, gPieceY - 1, gPieceRot)) {
                gPieceY -= 1;
                // If we move down, reset lock delay slightly (standard move reset)
                if (onGround) gLockTimer = 0; 
            }
        }

        // Modern Lock Delay Logic
        if (onGround) {
            gLockTimer += dt;
            if (gLockTimer >= LOCK_DELAY_MAX) lockPiece();
        } else {
            gLockTimer = 0;
        }
    }
    for(auto& p : gParticles){ 
        p.pos.x+=p.vel.x*dt*10.0f; p.pos.y+=p.vel.y*dt*10.0f; p.pos.z+=p.vel.z*dt*10.0f; 
        p.vel.y-=0.15f; // Simple gravity
        if(p.type == 1) { // Rotating cube logic
            p.ang.x += p.angVel.x*dt*5.0f; p.ang.y += p.angVel.y*dt*5.0f; p.ang.z += p.angVel.z*dt*5.0f;
        }
        p.life-=dt*1.2f; 
    }
    gParticles.erase(std::remove_if(gParticles.begin(), gParticles.end(), [](const Particle& p){ return p.life <= 0; }), gParticles.end());
    
    gRippleTime = std::max(0.0f, gRippleTime - dt);
    glutPostRedisplay(); glutTimerFunc(16, updatePhysics, 0);
}

static void specialKeyDown(int k, int, int){
    if(gState!=STATE_PLAY) return; gActionCount++;
    gSpecialKeys[k] = true;
    int dm=1; 
    if(k==GLUT_KEY_LEFT || k==GLUT_KEY_RIGHT) {
        int dir = (k == GLUT_KEY_LEFT) ? -dm : dm;
        if(!checkCollision(gPieceX+dir, gPieceY, gPieceRot)) {
            gPieceX += (float)dir; gTargetTiltX = (float)dir*5.0f;
        }
        gDASWait = DAS_DELAY; gARRTimer = ARR_INTERVAL;
    }
    if(k==GLUT_KEY_UP){ 
        int r=(gPieceRot+1)%4; 
        int srsIdx = getSRSIndex(gPieceRot, r);
        const int (*kTable)[2] = (gPieceType == 0) ? SRS_KICKS_I[srsIdx] : SRS_KICKS_JLSTZ[srsIdx];
        
        bool rotated = false;
        for(int i=0; i<5; i++) {
            float testX = gPieceX + kTable[i][0];
            float testY = gPieceY + kTable[i][1];
            if(!checkCollision(testX, testY, r)) {
                gPieceX = testX; gPieceY = testY; gPieceRot = r;
                rotated = true; break;
            }
        }
        if(rotated) gTargetTiltY = 3.0f;
    }
    if(k==GLUT_KEY_DOWN && !checkCollision(gPieceX, gPieceY-1, gPieceRot)){ gPieceY-=1; gScore+=1; gRecoilVelY -= 0.5f; }

    if(checkCollision(gPieceX, gPieceY-0.1f, gPieceRot)) {
        if(gMoveResetCount < MOVE_RESET_MAX) { gLockTimer = 0; gMoveResetCount++; }
    }
}

static void specialKeyUp(int k, int, int){
    gSpecialKeys[k] = false;
}

static void normalKeyboard(unsigned char k, int, int){
    if(k==27) exit(0);
    if(gState==STATE_HOME && (k==' ' || k==13)) resetGame();
    else if(gState==STATE_GAMEOVER && (k=='r' || k=='R')) resetGame();
    else if(gState==STATE_PLAY || gState==STATE_PAUSE){
        if(k=='p'||k=='P') gState = (gState == STATE_PLAY) ? STATE_PAUSE : STATE_PLAY;
        if(gState == STATE_PAUSE) return;
        if(k=='c'||k=='C'||k=='h'||k=='H') holdPiece();
        if(k=='g'||k=='G') gShowGhost = !gShowGhost;
        if(k==' '){ 
            gActionCount++; int g=getGhostY(); gScore+=((int)gPieceY-g)*2; 
            gPieceY=(float)g; gHardDropPulse = 1.0f; lockPiece(); 
        }
        if(k=='r'||k=='R') resetGame();
    }
}

int main(int argc, char** argv) {
    srand((unsigned int)time(NULL)); glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(WIN_W, WIN_H); glutCreateWindow("TETRIS: NEURAL GRID @alamin");
    glutFullScreen(); glEnable(GL_MULTISAMPLE); glEnable(GL_DEPTH_TEST); glDepthFunc(GL_LEQUAL); glEnable(GL_NORMALIZE);
    glClearColor(0.01f, 0.02f, 0.05f, 1.0f); glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
    loadHighScore(); initStars();
    glutDisplayFunc(display); glutReshapeFunc([](int w, int h){ WIN_W=w; WIN_H=h?h:1; glViewport(0,0,w,h); });
    glutKeyboardFunc(normalKeyboard); glutSpecialFunc(specialKeyDown); glutSpecialUpFunc(specialKeyUp); 
    glutTimerFunc(16, updatePhysics, 0);
    glutMainLoop(); return 0;
}
