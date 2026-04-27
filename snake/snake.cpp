/*
 * ================================================================
 *  NEO SNAKE 3D  –  OpenGL / FreeGLUT [PRO EDITION]
 *  ================================================================
 *
 *  FEATURES
 *  ─────────────────────────────────────────────────────────────
 *  • 3D rendering with neon glowing cubes
 *  • Full-screen immersive high-fidelity aesthetic
 *  • Mission Failed screens & Stroke-Font UI 
 *  • Dynamic camera tracking, tilting, and shake
 *  • Power-Up system (Spectrum Core, Phantom Protocol, Cryo Flux)
 *
 *  CONTROLS
 *  ─────────────────────────────────────────────────────────────
 *  Arrow Keys         Change direction (Up, Down, Left, Right)
 *  SPACE              Start/Restart Game
 *  P                  Pause / Resume
 *  R                  Restart game
 *  ESC                Quit
 *
 *  BUILD
 *  ─────────────────────────────────────────────────────────────
 *  Windows (MinGW):
 *    g++ snake.cpp -o snake.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
 * ================================================================
 */

#ifdef _WIN32
    #include <windows.h>
    #include <mmsystem.h>
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
#include <string>

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

// ──────────────────────────────────────────────────────────────
//  Window & Grid Dimensions
// ──────────────────────────────────────────────────────────────
static int WIN_W = 1280;
static int WIN_H = 720;

static const int GRID_W = 25;
static const int GRID_H = 25;
static const float BLOCK_SIZE = 1.0f;

static const float BORD_W = GRID_W * BLOCK_SIZE;
static const float BORD_H = GRID_H * BLOCK_SIZE;
static const float BORD_X = -BORD_W / 2.0f;
static const float BORD_Y = -BORD_H / 2.0f;

// ──────────────────────────────────────────────────────────────
//  Audio Engine
// ──────────────────────────────────────────────────────────────
static void playAudio(const char* file, bool loop) {
    char cmd[256]; snprintf(cmd, sizeof(cmd), "close bgm"); mciSendStringA(cmd, NULL, 0, NULL);
    if (!file) return;
    snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias bgm", file);
    if (mciSendStringA(cmd, NULL, 0, NULL) == 0) {
        snprintf(cmd, sizeof(cmd), "play bgm %s", loop ? "repeat" : "");
        mciSendStringA(cmd, NULL, 0, NULL);
    }
}
static void playSFX(const char* file) { PlaySoundA(file, NULL, SND_ASYNC | SND_FILENAME | SND_NODEFAULT); }

enum Direction { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
enum GameState { STATE_HOME, STATE_PLAY, STATE_GAMEOVER };
enum PowerUpType { PU_SHRINK, PU_GHOST, PU_SLOW };

struct Vec2i {
    int x, y;
    Vec2i(int x=0, int y=0): x(x), y(y) {}
    bool operator==(const Vec2i& o) const { return x == o.x && y == o.y; }
};

struct Vec3 { float x, y, z; Vec3(float x=0, float y=0, float z=0): x(x), y(y), z(z) {} };

struct Particle { Vec3 pos, vel; float life, r, g, b; };

struct PowerUp { Vec2i pos; PowerUpType type; float pulse; };

// ──────────────────────────────────────────────────────────────
//  Game State
// ──────────────────────────────────────────────────────────────
static GameState gState = STATE_HOME;
static std::vector<Vec2i> gSnake;
static Vec2i gApple;
static std::vector<PowerUp> gPowerUps;
static Direction gDir = DIR_UP;
static Direction gNextDir = DIR_UP;

static bool gPaused = false;
static int gScore = 0;
static int gHighScore = 0;
static int gApplesEaten = 0;

static float gMoveTimer = 0.0f;
static float gMoveSpeed = 8.0f; 

static float gGlobalTime = 0.0f;
static float gCameraShake = 0.0f;
static float gApplePulse = 0.0f;
static float gPowerUpSpawnTimer = 5.0f;
static float gGhostTimer = 0.0f;
static float gSlowTimer = 0.0f;

static std::vector<Particle> gParticles;
const char* SAVE_FILE = "highscore.dat";

// ──────────────────────────────────────────────────────────────
//  Helpers
// ──────────────────────────────────────────────────────────────
static float frand(float lo, float hi) { return lo + (hi - lo) * (float)rand() / (float)RAND_MAX; }

static void saveHighScore(){ FILE* f = fopen(SAVE_FILE, "w"); if(f){ fprintf(f, "%d", gHighScore); fclose(f); } }
static void loadHighScore(){ FILE* f = fopen(SAVE_FILE, "r"); if(f){ if(fscanf(f, "%d", &gHighScore) != 1) gHighScore = 0; fclose(f); } }

static void spawnParticles(Vec2i gridPos, float r, float g, float b, int count) {
    float wx = BORD_X + gridPos.x * BLOCK_SIZE + BLOCK_SIZE/2.0f;
    float wy = BORD_Y + gridPos.y * BLOCK_SIZE + BLOCK_SIZE/2.0f;
    for(int i = 0; i < count; i++) {
        Particle p; p.pos = Vec3(wx, wy, 0.5f);
        p.vel = Vec3(frand(-0.4f, 0.4f), frand(-0.4f, 0.4f), frand(0.1f, 0.8f));
        p.r = r; p.g = g; p.b = b; p.life = 1.0f;
        gParticles.push_back(p);
    }
}

static void placeApple() {
    bool valid = false;
    while (!valid) {
        gApple.x = rand() % GRID_W; gApple.y = rand() % GRID_H;
        valid = true;
        for(size_t i=0; i<gSnake.size(); i++){ if(gSnake[i] == gApple) { valid = false; break; } }
        for(auto& p : gPowerUps){ if(p.pos == gApple) { valid = false; break; } }
    }
}

static void resetGame() {
    playAudio("../racing/audio/bgm.mp3", true);
    gSnake.clear();
    gSnake.push_back(Vec2i(GRID_W/2, GRID_H/2));
    gSnake.push_back(Vec2i(GRID_W/2, GRID_H/2 - 1));
    gSnake.push_back(Vec2i(GRID_W/2, GRID_H/2 - 2));
    
    gDir = DIR_UP; gNextDir = DIR_UP; gScore = 0; gApplesEaten = 0;
    gMoveSpeed = 6.0f; gMoveTimer = 0.0f; gState = STATE_PLAY; gPaused = false;
    gParticles.clear(); gPowerUps.clear(); gCameraShake = 0.0f;
    gGhostTimer = 0.0f; gSlowTimer = 0.0f; gPowerUpSpawnTimer = 10.0f;
    placeApple();
}

static void die() {
    playSFX("../tanks/tanks music/explosion.wav");
    gState = STATE_GAMEOVER;
    playAudio(nullptr, false);
    gCameraShake = 1.5f;
    spawnParticles(gSnake[0], 1.0f, 0.0f, 0.0f, 40);
    if(gScore > gHighScore) { gHighScore = gScore; saveHighScore(); }
}

// ──────────────────────────────────────────────────────────────
//  Physics / Update
// ──────────────────────────────────────────────────────────────
static void updatePhysics(int) {
    gGlobalTime += 1.0f / 60.0f; float dt = 1.0f / 60.0f;
    gApplePulse += dt * 5.0f;
    
    if(gState == STATE_PLAY && !gPaused){
        if(gGhostTimer > 0) gGhostTimer -= dt;
        if(gSlowTimer > 0) gSlowTimer -= dt;
        
        gPowerUpSpawnTimer -= dt;
        if(gPowerUpSpawnTimer <= 0 && gPowerUps.size() < 2) {
            gPowerUpSpawnTimer = frand(10.0f, 20.0f);
            PowerUp p; p.pulse = 0;
            int r = rand() % 100;
            if(r < 30) p.type = PU_SHRINK; else if (r<65) p.type = PU_GHOST; else p.type = PU_SLOW;
            
            bool valid = false;
            while(!valid){
                p.pos.x = rand() % GRID_W; p.pos.y = rand() % GRID_H; valid = true;
                if(p.pos == gApple) valid = false;
                for(auto& sn : gSnake) if(sn == p.pos) valid = false;
                for(auto& pu : gPowerUps) if(pu.pos == p.pos) valid = false;
            }
            gPowerUps.push_back(p);
        }
        
        float currentSpeed = gMoveSpeed * (gSlowTimer > 0 ? 0.6f : 1.0f);
        gMoveTimer += dt * currentSpeed;
        
        if (gMoveTimer >= 1.0f) {
            gMoveTimer -= 1.0f; gDir = gNextDir;
            Vec2i head = gSnake[0];
            if (gDir == DIR_UP) head.y += 1; else if (gDir == DIR_DOWN) head.y -= 1;
            else if (gDir == DIR_LEFT) head.x -= 1; else if (gDir == DIR_RIGHT) head.x += 1;
            
            // Wall Collision
            if (head.x < 0 || head.x >= GRID_W || head.y < 0 || head.y >= GRID_H) die();
            else {
                // Self Collision
                bool hitSelf = false;
                for (size_t i = 0; i < gSnake.size() - 1; i++) { // -1 because tail will move
                    if (head == gSnake[i]) {
                        if(gGhostTimer > 0) { hitSelf = false; } // Phased through
                        else { die(); hitSelf = true; }
                        break;
                    }
                }
                
                if (!hitSelf) {
                    gSnake.insert(gSnake.begin(), head);
                    bool grew = false;
                    
                    // Apple Collision
                    if (head == gApple) {
                        playSFX("../racing/audio/pickup.wav");
                        gScore += 10 + (gApplesEaten / 5) * 5; gApplesEaten++;
                        gMoveSpeed += 0.2f; gCameraShake = 0.2f;
                        spawnParticles(gApple, 1.0f, 0.2f, 0.2f, 20);
                        placeApple();
                        grew = true;
                    }
                    
                    // Power-Up Collision
                    for(size_t i = 0; i < gPowerUps.size(); i++){
                        if(head == gPowerUps[i].pos){
                            playSFX("../tanks/tanks music/powerup.wav");
                            gScore += 100; gCameraShake = 0.3f;
                            if(gPowerUps[i].type == PU_SHRINK){
                                spawnParticles(gPowerUps[i].pos, 1.0f, 1.0f, 0.1f, 30);
                                int chop = 4;
                                while(chop-- > 0 && gSnake.size() > 3) gSnake.pop_back();
                            } else if (gPowerUps[i].type == PU_GHOST) {
                                spawnParticles(gPowerUps[i].pos, 0.8f, 0.2f, 1.0f, 30);
                                gGhostTimer = 10.0f;
                            } else if (gPowerUps[i].type == PU_SLOW) {
                                spawnParticles(gPowerUps[i].pos, 0.2f, 0.6f, 1.0f, 30);
                                gSlowTimer = 15.0f;
                            }
                            gPowerUps.erase(gPowerUps.begin()+i); i--;
                        }
                    }
                    
                    if(!grew) gSnake.pop_back();
                }
            }
        }
    }
    
    // Update Particles
    for (size_t i = 0; i < gParticles.size(); i++) {
        gParticles[i].pos.x += gParticles[i].vel.x; gParticles[i].pos.y += gParticles[i].vel.y; gParticles[i].pos.z += gParticles[i].vel.z;
        gParticles[i].vel.z -= dt * 1.5f; gParticles[i].life -= dt * 1.2f;
    }
    gParticles.erase(std::remove_if(gParticles.begin(), gParticles.end(), [](const Particle& p){ return p.life <= 0.0f; }), gParticles.end());
    gCameraShake *= 0.85f;
    glutPostRedisplay(); glutTimerFunc(16, updatePhysics, 0);
}

// ──────────────────────────────────────────────────────────────
//  UI & Drawing Helpers
// ──────────────────────────────────────────────────────────────
static void bitmapAt(float px, float py, const char* s, void* font=GLUT_BITMAP_HELVETICA_18){
    glRasterPos2f(px,py); for(const char* c=s;*c;c++) glutBitmapCharacter(font,*c);
}
static int bitmapWidth(const char* s, void* font=GLUT_BITMAP_HELVETICA_18){
    int w=0; for(const char* c=s;*c;c++) w+=glutBitmapWidth(font,*c); return w;
}
static void bitmapCentered(float cx, float py, const char* s, void* font=GLUT_BITMAP_HELVETICA_18){
    bitmapAt(cx - bitmapWidth(s,font)*0.5f, py, s, font);
}
static void strokeCentered(float cx, float cy, const std::string& s, float scale, float r, float g, float b, float a){
    float tw=0; for(char c:s) tw += glutStrokeWidth(GLUT_STROKE_ROMAN,c);
    float startX = cx - tw*scale*0.5f;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(r,g,b, a*0.35f); glLineWidth(6.0f);
    glPushMatrix(); glTranslatef(startX,cy,0); glScalef(scale,scale,scale);
    for(char c:s) glutStrokeCharacter(GLUT_STROKE_ROMAN,c); glPopMatrix();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,1, a); glLineWidth(2.5f);
    glPushMatrix(); glTranslatef(startX,cy,0); glScalef(scale,scale,scale);
    for(char c:s) glutStrokeCharacter(GLUT_STROKE_ROMAN,c); glPopMatrix();
    glLineWidth(1.0f); glDisable(GL_BLEND);
}

static void drawCube(float x, float y, float z, float size, float r, float g, float b, float alpha=1.0f) {
    glPushMatrix(); glTranslatef(x, y, z);
    GLfloat diff[] = { r*0.8f, g*0.8f, b*0.8f, alpha };
    GLfloat ambi[] = { r*0.3f, g*0.3f, b*0.3f, alpha };
    GLfloat spec[] = { 1.0f, 1.0f, 1.0f, alpha };
    glMaterialfv(GL_FRONT, GL_DIFFUSE,  diff); glMaterialfv(GL_FRONT, GL_AMBIENT,  ambi);
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec); glMaterialf (GL_FRONT, GL_SHININESS, 60.0f);
    if(alpha<1.0f){ glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); }
    glutSolidCube(size * 0.85f);
    glDisable(GL_LIGHTING); glColor4f(r, g, b, alpha); glLineWidth(2.0f); glutWireCube(size * 0.90f); glEnable(GL_LIGHTING);
    if(alpha<1.0f) glDisable(GL_BLEND);
    glPopMatrix();
}

// ──────────────────────────────────────────────────────────────
//  Display
// ──────────────────────────────────────────────────────────────
static void drawScene() {
    // Dynamic grid glow based on snake near edge
    glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(0.0f, 0.4f, 0.8f, 0.15f); glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (int i = 0; i <= GRID_W; i++) { glVertex3f(BORD_X + i * BLOCK_SIZE, BORD_Y, -0.5f); glVertex3f(BORD_X + i * BLOCK_SIZE, BORD_Y + BORD_H, -0.5f); }
    for (int i = 0; i <= GRID_H; i++) { glVertex3f(BORD_X, BORD_Y + i * BLOCK_SIZE, -0.5f); glVertex3f(BORD_X + BORD_W, BORD_Y + i * BLOCK_SIZE, -0.5f); }
    glEnd();

    // Board Border
    glColor4f(0.0f, 0.8f, 1.0f, 0.6f); glLineWidth(3.0f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(BORD_X, BORD_Y, -0.5f); glVertex3f(BORD_X + BORD_W, BORD_Y, -0.5f);
    glVertex3f(BORD_X + BORD_W, BORD_Y + BORD_H, -0.5f); glVertex3f(BORD_X, BORD_Y + BORD_H, -0.5f);
    glEnd(); glDisable(GL_BLEND); glEnable(GL_LIGHTING);

    // Draw Apple
    float appleW = BORD_X + gApple.x * BLOCK_SIZE + BLOCK_SIZE/2.0f, appleH = BORD_Y + gApple.y * BLOCK_SIZE + BLOCK_SIZE/2.0f;
    float aSize = BLOCK_SIZE * (0.8f + 0.15f * sin(gApplePulse));
    drawCube(appleW, appleH, 0.0f, aSize, 1.0f, 0.1f, 0.2f);
    
    // Draw Power Ups
    for (auto& p : gPowerUps) {
        p.pulse += 0.1f;
        float pW = BORD_X + p.pos.x * BLOCK_SIZE + BLOCK_SIZE/2.0f, pH = BORD_Y + p.pos.y * BLOCK_SIZE + BLOCK_SIZE/2.0f;
        float hover = 0.2f * sin(p.pulse);
        float r=1, g=1, b=1;
        if(p.type == PU_SHRINK) { r=1.0f; g=0.9f; b=0.1f; }
        else if (p.type == PU_GHOST) { r=0.8f; g=0.2f; b=1.0f; }
        else if (p.type == PU_SLOW) { r=0.2f; g=0.6f; b=1.0f; }
        drawCube(pW, pH, 0.4f + hover, BLOCK_SIZE*0.75f, r, g, b);
    }
    
    // Draw Snake
    for (size_t i = 0; i < gSnake.size(); i++) {
        float wx = BORD_X + gSnake[i].x * BLOCK_SIZE + BLOCK_SIZE/2.0f, wy = BORD_Y + gSnake[i].y * BLOCK_SIZE + BLOCK_SIZE/2.0f;
        float t = (float)i / (float)gSnake.size();
        float r = 0.1f + 0.2f * (1.0f - t), g = 1.0f - 0.4f * t, b = 0.5f + 0.5f * t;
        float bSize = BLOCK_SIZE * (0.9f - t * 0.2f);
        if (i == 0) bSize *= 1.1f;
        
        float alpha = 1.0f;
        if (gGhostTimer > 0.0f) { r=0.8f; g=0.2f; b=1.0f; alpha = 0.6f + 0.3f*sin(gGlobalTime*10.0f); }
        drawCube(wx, wy, 0.0f, bSize, r, g, b, alpha);
    }

    // Draw Particles
    glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); glPointSize(5.0f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < gParticles.size(); i++) {
        glColor4f(gParticles[i].r, gParticles[i].g, gParticles[i].b, gParticles[i].life);
        glVertex3f(gParticles[i].pos.x, gParticles[i].pos.y, gParticles[i].pos.z);
    }
    glEnd(); glDisable(GL_BLEND);
}

static void drawHUD(){
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity(); glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING);
    
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0, 0.04, 0.12, 0.88); glRecti(0, WIN_H-65, WIN_W, WIN_H);
    glColor4f(0, 1.0, 0.4, 0.7); glLineWidth(2.5f); glBegin(GL_LINES); glVertex2f(0, WIN_H-65); glVertex2f(WIN_W, WIN_H-65); glEnd();
    
    char buf[128]; glColor3f(0, 1, 0.4);
    snprintf(buf, sizeof(buf), "NEURAL SCORE: %06d", gScore); bitmapAt(30, WIN_H-42, buf, GLUT_BITMAP_TIMES_ROMAN_24);
    snprintf(buf, sizeof(buf), "SNAKE LENGTH: %03d", (int)gSnake.size()); bitmapCentered(WIN_W*0.5f, WIN_H-42, buf, GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0, 1, 1); snprintf(buf, sizeof(buf), "PEAK RECORD: %06d", gHighScore); bitmapAt(WIN_W-280, WIN_H-42, buf, GLUT_BITMAP_TIMES_ROMAN_24);
    
    if(gGhostTimer > 0){ glColor3f(0.8f, 0.2f, 1.0f); snprintf(buf, sizeof(buf), "PHANTOM PROTOCOL: %.1fs", gGhostTimer); bitmapCentered(WIN_W*0.5f, WIN_H-90, buf, GLUT_BITMAP_HELVETICA_18); }
    if(gSlowTimer > 0){ glColor3f(0.2f, 0.6f, 1.0f); snprintf(buf, sizeof(buf), "CRYO FLUX: %.1fs", gSlowTimer); bitmapCentered(WIN_W*0.5f, WIN_H-120, buf, GLUT_BITMAP_HELVETICA_18); }
    
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

static void drawOverlay(){
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity(); glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING);
    float cx = WIN_W*0.5f;
    
    if(gState == STATE_HOME){
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glColor4f(0, 0.05, 0.02, 0.85); glRecti(0, 0, WIN_W, WIN_H);
        float p = 0.8f + 0.2f*sinf(gGlobalTime*3.0f);
        strokeCentered(cx, WIN_H*0.65, "NEO SNAKE 3D", 0.45, 0, 1.0, 0.4, p);
        strokeCentered(cx, WIN_H*0.48, "CYBER LINK", 0.35, 1, 0.6, 0, p);
        float b = 0.5f + 0.5f*sinf(gGlobalTime*6.0f); glColor3f(b, b, b);
        bitmapCentered(cx, WIN_H*0.25, "[ PRESS SPACE TO INITIALIZE ]", GLUT_BITMAP_TIMES_ROMAN_24);
        glDisable(GL_BLEND);
    } else if(gState == STATE_GAMEOVER){
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glColor4f(0.12, 0, 0, 0.92); glRecti(0, 0, WIN_W, WIN_H);
        glBegin(GL_QUADS); glColor4f(0.04, 0, 0.08, 0.75); glVertex2f(cx-300, WIN_H*0.2); glVertex2f(cx+300, WIN_H*0.2); glVertex2f(cx+300, WIN_H*0.8); glVertex2f(cx-300, WIN_H*0.8); glEnd();
        float la = 0.6f + 0.4f*sinf(gGlobalTime*4.0f); glColor4f(1, 0.15, 0.05, la); glLineWidth(2.5f); glBegin(GL_LINE_LOOP); glVertex2f(cx-300, WIN_H*0.2); glVertex2f(cx+300, WIN_H*0.2); glVertex2f(cx+300, WIN_H*0.8); glVertex2f(cx-300, WIN_H*0.8); glEnd();
        strokeCentered(cx, WIN_H*0.7, "SYSTEM TERMINATED", 0.28, 1, 0.1, 0, 1);
        char buf[128]; glColor3f(1, 0.8, 0); snprintf(buf, sizeof(buf), "FINAL SCORE: %06d", gScore); bitmapCentered(cx, WIN_H*0.55, buf, GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.4, 0.7, 1); snprintf(buf, sizeof(buf), "PEAK RECORD: %06d", gHighScore); bitmapCentered(cx, WIN_H*0.48, buf, GLUT_BITMAP_HELVETICA_18);
        float blink = 0.5f + 0.5f*sinf(gGlobalTime*5.0f); glColor4f(blink, blink, blink, 1); bitmapCentered(cx, WIN_H*0.35, "[ Press SPACE to Reboot Neural Link ]", GLUT_BITMAP_HELVETICA_18);
        glDisable(GL_BLEND);
    } else if(gPaused){
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glColor4f(0, 0, 0, 0.6); glRecti(0, 0, WIN_W, WIN_H);
        strokeCentered(cx, WIN_H*0.55, "PAUSED", 0.35, 1, 1, 0, 1); glDisable(GL_BLEND);
    }
    
    glEnable(GL_DEPTH_TEST); glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

static void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glLoadIdentity();
    gluPerspective(45.0, (double)WIN_W / WIN_H, 1.0, 100.0);

    float shakeX = gCameraShake * frand(-1.0f, 1.0f), shakeY = gCameraShake * frand(-1.0f, 1.0f);
    float tiltX = sin(gGlobalTime * 0.3f) * 2.0f, tiltY = -28.0f; 
    gluLookAt(tiltX + shakeX, tiltY + shakeY, 32.0f, shakeX * 0.5f, shakeY * 0.5f, 0.0f, 0.0f, 1.0f, 0.0f);

    GLfloat lpos[] = { 0.0f, 0.0f, 20.0f, 1.0f }; glLightfv(GL_LIGHT0, GL_POSITION, lpos); glEnable(GL_LIGHT0); glEnable(GL_LIGHTING); glEnable(GL_DEPTH_TEST);

    if (gState == STATE_HOME) { drawScene(); drawOverlay(); }
    else { drawScene(); drawHUD(); drawOverlay(); }

    glutSwapBuffers();
}

static void reshape(int w, int h) { WIN_W = w; WIN_H = h ? h : 1; glViewport(0, 0, WIN_W, WIN_H); }

// ──────────────────────────────────────────────────────────────
//  Input
// ──────────────────────────────────────────────────────────────
static void specialKeyDown(int key, int, int) {
    if(gState != STATE_PLAY || gPaused) return;
    if (key == GLUT_KEY_UP    && gDir != DIR_DOWN)  gNextDir = DIR_UP;
    if (key == GLUT_KEY_DOWN  && gDir != DIR_UP)    gNextDir = DIR_DOWN;
    if (key == GLUT_KEY_LEFT  && gDir != DIR_RIGHT) gNextDir = DIR_LEFT;
    if (key == GLUT_KEY_RIGHT && gDir != DIR_LEFT)  gNextDir = DIR_RIGHT;
}

static void normalKeyboard(unsigned char key, int, int) {
    if (key == 27) exit(0);
    if(key == 'p' || key == 'P') gPaused = !gPaused;
    if(key == 'r' || key == 'R') if(gState == STATE_GAMEOVER) resetGame();
    if(key == ' ' && gState != STATE_PLAY) resetGame();
}

// ──────────────────────────────────────────────────────────────
//  Main
// ──────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    srand((unsigned int)time(nullptr)); loadHighScore();
    glutInit(&argc, argv); glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIN_W, WIN_H); glutCreateWindow("Neo Snake 3D: Cyber Link");
    glutFullScreen();

    glClearColor(0.01f, 0.02f, 0.05f, 1.0f);
    gSnake.push_back(Vec2i(GRID_W/2, GRID_H/2)); placeApple(); // Fake spawn for homescreen background
    
    glutDisplayFunc(display); glutReshapeFunc(reshape);
    glutKeyboardFunc(normalKeyboard); glutSpecialFunc(specialKeyDown);
    glutTimerFunc(16, updatePhysics, 0);

    glutMainLoop();
    return 0;
}
