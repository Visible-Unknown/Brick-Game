/*
 * ================================================================
 *  NEO TANKS: IRON GRID  –  OpenGL / FreeGLUT [PRO EDITION]
 *  ================================================================
 *
 *  FEATURES
 *  ─────────────────────────────────────────────────────────────
 *  • Full-screen immersive tactical tank combat
 *  • Professional Stroke-Font UI with neon glowing titles
 *  • Pro Audio System: High-energy synthwave BGM tracks
 *  • Power-up System: Shield, Rapid Fire, Speed Boost, Mega Bomb
 *  • Dynamic camera shake and shell resonance
 *  • Additive-blended particle explosions and power-up beams
 *
 *  CONTROLS
 *  ─────────────────────────────────────────────────────────────
 *  Arrow Keys        Move Tank
 *  SPACE             Fire Main Cannon
 *  P                 Pause / Resume
 *  R                 Restart Game
 *  ESC               Quit
 *
 *  BUILD (Windows / MinGW)
 *  ─────────────────────────────────────────────────────────────
 *    g++ tanks.cpp -o tanks.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
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
//  Constants & Types
// ──────────────────────────────────────────────────────────────
static int WIN_W = 1280;
static int WIN_H = 720;

const int GRID_SIZE = 17;
const float CELL_SIZE = 2.2f;
const float HALF_GRID = (GRID_SIZE * CELL_SIZE) / 2.0f;

enum BlockType  { BT_EMPTY=0, BT_BRICK=1, BT_STEEL=2 };
enum GameState  { STATE_HOME, STATE_PLAY, STATE_GAMEOVER };
enum PowerupType { PT_RAPID, PT_SHIELD, PT_SPEED, PT_BOMB };

struct Vec3 {
    float x, y, z;
    Vec3(float x=0, float y=0, float z=0) : x(x), y(y), z(z) {}
};

struct Shell {
    Vec3 pos;
    Vec3 dir;
    bool active;
    bool enemyOwned;
};

struct Tank {
    Vec3 pos;
    float angle;
    int hp;
    bool active;
    float fireCooldown;
};

struct Particle {
    Vec3 pos, vel;
    float life;
    float r, g, b;
};

struct Powerup {
    Vec3 pos;
    PowerupType type;
    bool active;
    float life;
};

static void setupLighting();

// ──────────────────────────────────────────────────────────────
//  Global State
// ──────────────────────────────────────────────────────────────
static GameState gState = STATE_HOME;
static int gGrid[GRID_SIZE][GRID_SIZE];
static Tank gPlayer;
static std::vector<Tank> gEnemies;
static std::vector<Shell> gShells;
static std::vector<Particle> gParticles;
static std::vector<Powerup> gPowerups;

static bool gKeyLeft=0, gKeyRight=0, gKeyUp=0, gKeyDown=0, gKeySpace=0;
static int gScore=0, gHighScore=0;
static float gGlobalTime=0.0f, gDifficulty=1.0f;
static float gPlayerShieldTimer=0.0f, gPlayerRapidTimer=0.0f, gPlayerSpeedTimer=0.0f;
static float gCameraShake = 0.0f;
static bool gPaused=false;
static int gLastMs=0;

static char gPowerupMsg[64] = "";
static float gPowerupMsgTimer = 0.0f;

const char* SAVE_FILE = "highscore.dat";

// ──────────────────────────────────────────────────────────────
//  Utilities
// ──────────────────────────────────────────────────────────────
static float frand(float lo, float hi) { return lo + (hi-lo)*(float)rand()/(float)RAND_MAX; }

static void saveHighScore() {
    FILE* f = fopen(SAVE_FILE, "w");
    if(f) { fprintf(f, "%d", gHighScore); fclose(f); }
}

static void loadHighScore() {
    FILE* f = fopen(SAVE_FILE, "r");
    if(f) { if(fscanf(f, "%d", &gHighScore) != 1) gHighScore = 0; fclose(f); }
}

static void spawnParticles(Vec3 pos, float r, float g, float b, int count) {
    for(int i=0; i<count; i++) {
        Particle p;
        p.pos = pos;
        p.vel = Vec3(frand(-0.5f,0.5f), frand(-0.5f,0.5f), frand(0.2f,0.8f));
        p.r=r; p.g=g; p.b=b; p.life=1.0f;
        gParticles.push_back(p);
    }
}

// ──────────────────────────────────────────────────────────────
//  Audio Helpers
// ──────────────────────────────────────────────────────────────
static void playSfx(const char* filename) {
#ifdef _WIN32
    PlaySoundA(filename, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
#endif
}

static void stopBgm() {
#ifdef _WIN32
    mciSendStringA("stop bgm", NULL, 0, NULL);
    mciSendStringA("close bgm", NULL, 0, NULL);
#endif
}

static void startBgm(const char* filename) {
#ifdef _WIN32
    stopBgm();
    char cmd[512];
    // Attempt to open with mpegvideo type
    snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias bgm", filename);
    if (mciSendStringA(cmd, NULL, 0, NULL) != 0) {
        // Fallback: let MCI infer the type from extension
        snprintf(cmd, sizeof(cmd), "open \"%s\" alias bgm", filename);
        mciSendStringA(cmd, NULL, 0, NULL);
    }
    mciSendStringA("play bgm repeat", NULL, 0, NULL);
#endif
}

// ──────────────────────────────────────────────────────────────
//  UI Drawing Utilities
// ──────────────────────────────────────────────────────────────
static void bitmapAt(float px, float py, const char* s, void* font=GLUT_BITMAP_HELVETICA_18){
    glRasterPos2f(px,py);
    for(const char* c=s;*c;c++) glutBitmapCharacter(font,*c);
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

// ──────────────────────────────────────────────────────────────
//  Game Logic
// ──────────────────────────────────────────────────────────────
static void initArena() {
    for(int y=0; y<GRID_SIZE; y++) {
        for(int x=0; x<GRID_SIZE; x++) {
            if(x==0 || x==GRID_SIZE-1 || y==0 || y==GRID_SIZE-1) gGrid[y][x] = BT_STEEL;
            else if(rand()%6 == 0) gGrid[y][x] = BT_BRICK;
            else if(rand()%25 == 0) gGrid[y][x] = BT_STEEL;
            else gGrid[y][x] = BT_EMPTY;
        }
    }
    gGrid[1][GRID_SIZE/2] = gGrid[2][GRID_SIZE/2] = BT_EMPTY;
    gGrid[GRID_SIZE-2][GRID_SIZE/2] = gGrid[GRID_SIZE-3][GRID_SIZE/2] = BT_EMPTY;
}

static void resetGame() {
    initArena();
    gPlayer.pos = Vec3(0, -HALF_GRID + CELL_SIZE*1.5f, 0.5f);
    gPlayer.angle = 90.0f;
    gPlayer.hp = 3;
    gPlayer.active = true;
    gPlayer.fireCooldown = 0;
    gEnemies.clear(); gShells.clear(); gParticles.clear(); gPowerups.clear();
    gScore = 0; gDifficulty = 1.0f; gPlayerShieldTimer = 0; gPlayerRapidTimer = 0; gPlayerSpeedTimer = 0;
    gCameraShake = 0; gState = STATE_PLAY;
    startBgm("tanks music/bgm_play.mp3");
}

static void spawnEnemy() {
    Tank e;
    e.pos = Vec3(frand(-HALF_GRID+CELL_SIZE, HALF_GRID-CELL_SIZE), HALF_GRID - CELL_SIZE*1.5f, 0.5f);
    e.angle = 270.0f; e.hp = 1; e.active = true; e.fireCooldown = frand(1.0f, 2.0f);
    gEnemies.push_back(e);
}

static bool checkCollision(float x, float y) {
    int gx = (int)((x + HALF_GRID) / CELL_SIZE);
    int gy = (int)((y + HALF_GRID) / CELL_SIZE);
    if(gx<0 || gx>=GRID_SIZE || gy<0 || gy>=GRID_SIZE) return true;
    return (gGrid[gy][gx] != BT_EMPTY);
}

static void update(float dt) {
    if(gState == STATE_HOME) { gGlobalTime += dt; return; }
    if(gPaused) return;
    gGlobalTime += dt;
    if(gPowerupMsgTimer > 0) gPowerupMsgTimer -= dt;

    if(gState == STATE_PLAY) {
        float moveBase = (gPlayerSpeedTimer > 0 ? 9.5f : 6.0f);
        float moveSpd = moveBase * dt;
        float nextX = gPlayer.pos.x, nextY = gPlayer.pos.y;
        if(gKeyUp)    { nextY += moveSpd; gPlayer.angle = 90.0f; }
        if(gKeyDown)  { nextY -= moveSpd; gPlayer.angle = 270.0f; }
        if(gKeyLeft)  { nextX -= moveSpd; gPlayer.angle = 180.0f; }
        if(gKeyRight) { nextX += moveSpd; gPlayer.angle = 0.0f; }
        
        if(!checkCollision(nextX, nextY) && !checkCollision(nextX+0.5f, nextY+0.5f) && !checkCollision(nextX-0.5f, nextY-0.5f)) {
            gPlayer.pos.x = nextX; gPlayer.pos.y = nextY;
        }

        if(gPlayer.fireCooldown > 0) gPlayer.fireCooldown -= dt;
        if(gKeySpace && gPlayer.fireCooldown <= 0) {
            Shell s; s.pos = gPlayer.pos;
            float rad = gPlayer.angle * 3.14159f / 180.0f;
            s.dir = Vec3(cosf(rad), sinf(rad), 0);
            s.active = true; s.enemyOwned = false;
            gShells.push_back(s);
            gPlayer.fireCooldown = (gPlayerRapidTimer > 0) ? 0.12f : 0.35f;
            gCameraShake += 0.1f;
            playSfx("tanks music/fire.wav");
        }

        if(gPlayerShieldTimer > 0) gPlayerShieldTimer -= dt;
        if(gPlayerRapidTimer > 0) gPlayerRapidTimer -= dt;
        if(gPlayerSpeedTimer > 0) gPlayerSpeedTimer -= dt;
        if(gEnemies.size() < (size_t)(2 + gScore/1200)) { if(frand(0,1) < 0.012f) spawnEnemy(); }
    }

    // Shells
    float shellSpd = 16.0f * dt;
    for(auto& s : gShells) {
        if(!s.active) continue;
        s.pos.x += s.dir.x * shellSpd; s.pos.y += s.dir.y * shellSpd;
        int gx = (int)((s.pos.x + HALF_GRID) / CELL_SIZE);
        int gy = (int)((s.pos.y + HALF_GRID) / CELL_SIZE);
        if(gx>=0 && gx<GRID_SIZE && gy>=0 && gy<GRID_SIZE && gGrid[gy][gx] != BT_EMPTY) {
            if(gGrid[gy][gx] == BT_BRICK) { gGrid[gy][gx] = BT_EMPTY; spawnParticles(s.pos, 1, 0.5, 0.2, 12); gScore += 15; }
            else spawnParticles(s.pos, 0.6, 0.8, 1.0, 6);
            s.active = false; gCameraShake += 0.05f; continue;
        }
        if(!s.enemyOwned) {
            for(auto& e : gEnemies) {
                float dx = e.pos.x - s.pos.x, dy = e.pos.y - s.pos.y;
                if(dx*dx + dy*dy < 1.0f) {
                    e.active = false; s.active = false; spawnParticles(e.pos, 1, 0.3, 0.1, 25);
                    gScore += 150; gCameraShake += 0.3f;
                    playSfx("tanks music/explosion.wav");
                    if(rand()%10 == 0) {
                        Powerup p; p.pos = e.pos; p.active = true; p.life = 12.0f;
                        int r = rand()%10; 
                        if(r<3) p.type = PT_SHIELD; else if(r<6) p.type = PT_RAPID; else if(r<9) p.type = PT_SPEED; else p.type = PT_BOMB;
                        gPowerups.push_back(p);
                    }
                    break;
                }
            }
        } else {
            float dx = gPlayer.pos.x - s.pos.x, dy = gPlayer.pos.y - s.pos.y;
            if(dx*dx + dy*dy < 1.0f) {
                s.active = false;
                if(gPlayerShieldTimer <= 0) {
                    gPlayer.hp--; gCameraShake = 0.8f; spawnParticles(gPlayer.pos, 1, 0, 0, 20);
                    playSfx("tanks music/explosion.wav");
                    if(gPlayer.hp <= 0) { 
                        gState = STATE_GAMEOVER; if(gScore > gHighScore) { gHighScore = gScore; saveHighScore(); } 
                        startBgm("tanks music/bgm_menu.mp3");
                    }
                } else { spawnParticles(s.pos, 0, 1, 1, 12); }
            }
        }
        if(fabs(s.pos.x) > HALF_GRID || fabs(s.pos.y) > HALF_GRID) s.active = false;
    }
    gShells.erase(std::remove_if(gShells.begin(), gShells.end(), [](const Shell& s){return !s.active;}), gShells.end());

    // Enemies
    for(auto& e : gEnemies) {
        float edx = gPlayer.pos.x - e.pos.x, edy = gPlayer.pos.y - e.pos.y;
        if(fabs(edx) > fabs(edy)) { e.pos.x += (edx > 0 ? 1 : -1) * 2.5f * dt; e.angle = (edx > 0 ? 0.0f : 180.0f); }
        else { e.pos.y += (edy > 0 ? 1 : -1) * 2.5f * dt; e.angle = (edy > 0 ? 90.0f : 270.0f); }
        if(e.fireCooldown > 0) e.fireCooldown -= dt;
        else {
            Shell s; s.pos = e.pos; float rad = e.angle * 3.14159f / 180.0f;
            s.dir = Vec3(cosf(rad), sinf(rad), 0); s.active = true; s.enemyOwned = true;
            gShells.push_back(s); e.fireCooldown = frand(1.2f, 2.5f);
        }
    }
    gEnemies.erase(std::remove_if(gEnemies.begin(), gEnemies.end(), [](const Tank& t){return !t.active;}), gEnemies.end());

    // Powerups & Particles
    for(auto& p : gPowerups) {
        if(pow(gPlayer.pos.x-p.pos.x,2)+pow(gPlayer.pos.y-p.pos.y,2) < 1.8) {
            p.active = false; gPowerupMsgTimer = 3.0f;
            playSfx("tanks music/powerup.wav");
            if(p.type == PT_SHIELD) { gPlayerShieldTimer = 10.0f; strcpy(gPowerupMsg, "IRON SHIELD REINFORCED"); }
            else if(p.type == PT_RAPID) { gPlayerRapidTimer = 10.0f; strcpy(gPowerupMsg, "HYPER CANNON ACTIVE"); }
            else if(p.type == PT_SPEED) { gPlayerSpeedTimer = 10.0f; strcpy(gPowerupMsg, "TURBO ENGINES ENGAGED"); }
            else if(p.type == PT_BOMB) {
                strcpy(gPowerupMsg, "MEGA BOMB DETONATED"); gCameraShake = 2.5f;
                for(auto& e : gEnemies) { e.active = false; spawnParticles(e.pos, 1, 0.4, 0, 30); gScore += 100; }
                playSfx("tanks music/explosion.wav");
            }
            spawnParticles(p.pos, 1, 1, 1, 15);
        }
        p.life -= dt; if(p.life <= 0) p.active = false;
    }
    gPowerups.erase(std::remove_if(gPowerups.begin(), gPowerups.end(), [](const Powerup& p){return !p.active;}), gPowerups.end());
    for(auto& p : gParticles) { p.pos.x += p.vel.x; p.pos.y += p.vel.y; p.pos.z += p.vel.z; p.life -= dt * 1.5f; }
    gParticles.erase(std::remove_if(gParticles.begin(), gParticles.end(), [](const Particle& p){return p.life<=0;}), gParticles.end());
    gCameraShake *= 0.92f;
}

// ──────────────────────────────────────────────────────────────
//  Rendering
// ──────────────────────────────────────────────────────────────
static void drawPowerup(const Powerup& p) {
    glPushMatrix(); glTranslatef(p.pos.x, p.pos.y, 0.5f);
    glRotatef(gGlobalTime * 150.0f, 0, 1, 0);
    float bob = 0.2f * sinf(gGlobalTime * 4.0f); glTranslatef(0, 0, bob);
    float r=1, g=1, b=1;
    if(p.type == PT_SHIELD) { r=0; g=0.7; b=1; }
    else if(p.type == PT_RAPID) { r=1; g=1; b=0; }
    else if(p.type == PT_SPEED) { r=0.8; g=0.2; b=1; }
    else { r=1; g=0.2; b=0; }
    glDisable(GL_LIGHTING); glColor3f(r, g, b); glutWireTorus(0.15, 0.4, 8, 12);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(r, g, b, 0.4f); glutSolidSphere(0.35, 12, 12);
    glDisable(GL_BLEND); glEnable(GL_LIGHTING); glPopMatrix();
}

static void drawTank(float x, float y, float angle, float r, float g, float b, bool shield=false) {
    glPushMatrix(); glTranslatef(x, y, 0.25f); glRotatef(angle, 0, 0, 1);
    glColor3f(r*0.3f, g*0.3f, b*0.3f); glPushMatrix(); glScalef(1.4f, 1.1f, 0.5f); glutSolidCube(1.0f); glPopMatrix();
    glDisable(GL_LIGHTING); glColor3f(r, g, b); glLineWidth(2.5f);
    glBegin(GL_LINES); glVertex3f(-0.7f, -0.6f, 0); glVertex3f(0.7f, -0.6f, 0); glVertex3f(-0.7f, 0.6f,0); glVertex3f(0.7f,0.6f,0); glEnd();
    glEnable(GL_LIGHTING); glTranslatef(0,0,0.4f); glColor3f(r*0.6f, g*0.6f, b*0.6f); glutSolidSphere(0.45f, 16, 12);
    glTranslatef(0.45f, 0, 0); glPushMatrix(); glScalef(1.0f, 0.18f, 0.18f); glutSolidCube(1.0f); glPopMatrix();
    if(shield) {
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(0, 0.7f, 1, 0.3f + 0.15f*sinf(gGlobalTime*8.0f)); glutSolidSphere(1.2f, 18, 18); glDisable(GL_BLEND);
    }
    glPopMatrix();
}

static void drawScene() {
    glDisable(GL_LIGHTING);
    float pulse = 0.08f + 0.04f*sinf(gGlobalTime*1.5f);
    glColor3f(pulse * 0.5f, pulse * 0.5f, pulse * 2.0f);
    glBegin(GL_LINES);
    for(float i=-HALF_GRID; i<=HALF_GRID; i+=CELL_SIZE) {
        glVertex3f(i, -HALF_GRID, 0); glVertex3f(i, HALF_GRID, 0);
        glVertex3f(-HALF_GRID, i, 0); glVertex3f(HALF_GRID, i, 0);
    }
    glEnd();
    glEnable(GL_LIGHTING);
    for(int y=0; y<GRID_SIZE; y++) {
        for(int x=0; x<GRID_SIZE; x++) {
            if(gGrid[y][x] == BT_EMPTY) continue;
            float bx = x*CELL_SIZE-HALF_GRID+CELL_SIZE/2, by = y*CELL_SIZE-HALF_GRID+CELL_SIZE/2;
            glPushMatrix(); glTranslatef(bx, by, 0.6f);
            if(gGrid[y][x] == BT_BRICK) { glColor3f(0.7, 0.3, 0.1); glutSolidCube(CELL_SIZE*0.92f);
                glDisable(GL_LIGHTING); glColor3f(1, 0.4, 0); glLineWidth(1.5f); glutWireCube(CELL_SIZE*0.94f); glEnable(GL_LIGHTING);
            } else { glColor3f(0.3, 0.4, 0.6); glutSolidCube(CELL_SIZE*0.95f);
                glDisable(GL_LIGHTING); glColor3f(0.5, 0.8, 1); glLineWidth(2.0f); glutWireCube(CELL_SIZE*0.97f); glEnable(GL_LIGHTING);
            }
            glPopMatrix();
        }
    }
    for(auto& p : gPowerups) drawPowerup(p);
    drawTank(gPlayer.pos.x, gPlayer.pos.y, gPlayer.angle, 0.1f, 0.7f, 1.0f, gPlayerShieldTimer > 0);
    for(auto& e : gEnemies) drawTank(e.pos.x, e.pos.y, e.angle, 1.0f, 0.1f, 0.2f);
    glDisable(GL_LIGHTING); glPointSize(6.0f); glBegin(GL_POINTS);
    for(auto& s : gShells) { if(s.enemyOwned) glColor3f(1,0,0); else glColor3f(1,1,0); glVertex3f(s.pos.x, s.pos.y, s.pos.z); }
    glEnd();
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); glBegin(GL_POINTS);
    for(auto& p : gParticles) { glColor4f(p.r, p.g, p.b, p.life); glVertex3f(p.pos.x, p.pos.y, p.pos.z); }
    glEnd(); glDisable(GL_BLEND);
}

static void drawHUD() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0, 0.04, 0.12, 0.88); glBegin(GL_QUADS); glVertex2f(0, WIN_H-65); glVertex2f(WIN_W, WIN_H-65); glVertex2f(WIN_W, WIN_H); glVertex2f(0, WIN_H); glEnd();
    glColor4f(0, 0.7, 1.0, 0.7); glLineWidth(2.5f); glBegin(GL_LINES); glVertex2f(0, WIN_H-65); glVertex2f(WIN_W, WIN_H-65); glEnd();
    char buf[128]; glColor3f(0, 1, 1);
    snprintf(buf, sizeof(buf), "CORE STATUS: %d HP", gPlayer.hp); bitmapAt(30, WIN_H-42, buf, GLUT_BITMAP_TIMES_ROMAN_24);
    snprintf(buf, sizeof(buf), "NEURAL FLUX: %06d", gScore); bitmapCentered(WIN_W*0.5f, WIN_H-42, buf, GLUT_BITMAP_TIMES_ROMAN_24);
    snprintf(buf, sizeof(buf), "MAX RECORD: %06d", gHighScore); bitmapAt(WIN_W-280, WIN_H-42, buf, GLUT_BITMAP_TIMES_ROMAN_24);
    if(gPowerupMsgTimer > 0) {
        float a = std::min(1.0f, gPowerupMsgTimer);
        glColor4f(1, 0.8, 0, a); bitmapCentered(WIN_W*0.5f, WIN_H*0.72f, gPowerupMsg, GLUT_BITMAP_TIMES_ROMAN_24);
    }
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

static void drawHomeScreen() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0, 0, 0.05, 0.9); glRecti(0, 0, WIN_W, WIN_H);
    float p = 0.8f + 0.2f*sinf(gGlobalTime*3.0f);
    strokeCentered(WIN_W*0.5, WIN_H*0.65, "NEO TANKS", 0.45, 0, 0.7, 1, p);
    strokeCentered(WIN_W*0.5, WIN_H*0.48, "IRON GRID", 0.35, 1, 0.6, 0, p);
    float b = 0.5f + 0.5f*sinf(gGlobalTime*6.0f); glColor3f(b, b, b);
    bitmapCentered(WIN_W*0.5, WIN_H*0.25, "[ PRESS SPACE TO INITIALIZE SYSTEM ]", GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0.3, 0.4, 0.6); bitmapCentered(WIN_W*0.5, WIN_H*0.1, "Developed by AL AMIN HOSSAIN", GLUT_BITMAP_HELVETICA_18);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

static void drawOverlay() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    float cx = WIN_W*0.5f;
    if(gState == STATE_GAMEOVER) {
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.12, 0, 0, 0.92); glRecti(0, 0, WIN_W, WIN_H);
        glColor4f(0.04, 0, 0.08, 0.75); glBegin(GL_QUADS); glVertex2f(cx-300, WIN_H*0.2); glVertex2f(cx+300, WIN_H*0.2); glVertex2f(cx+300, WIN_H*0.8); glVertex2f(cx-300, WIN_H*0.8); glEnd();
        float la = 0.6f + 0.4f*sinf(gGlobalTime*4.0f); glColor4f(1, 0.15, 0.05, la); glLineWidth(2.5f); glBegin(GL_LINE_LOOP); glVertex2f(cx-300, WIN_H*0.2); glVertex2f(cx+300, WIN_H*0.2); glVertex2f(cx+300, WIN_H*0.8); glVertex2f(cx-300, WIN_H*0.8); glEnd();
        strokeCentered(cx, WIN_H*0.7, "MISSION FAILED", 0.28, 1, 0.1, 0, 1);
        char buf[128]; glColor3f(1, 0.8, 0); snprintf(buf, sizeof(buf), "FINAL FLUX: %06d", gScore); bitmapCentered(cx, WIN_H*0.55, buf, GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.4, 0.7, 1); snprintf(buf, sizeof(buf), "PEAK RECORD: %06d", gHighScore); bitmapCentered(cx, WIN_H*0.48, buf, GLUT_BITMAP_HELVETICA_18);
        float blink = 0.5f + 0.5f*sinf(gGlobalTime*5.0f); glColor4f(blink, blink, blink, 1);
        bitmapCentered(cx, WIN_H*0.35, "[ Press R to Restart Neural Connection ]", GLUT_BITMAP_HELVETICA_18);
        glDisable(GL_BLEND);
    } else if(gPaused) {
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glColor4f(0, 0, 0, 0.6); glRecti(0, 0, WIN_W, WIN_H);
        strokeCentered(cx, WIN_H*0.55, "PAUSED", 0.35, 1, 1, 0, 1); glDisable(GL_BLEND);
    }
    glEnable(GL_DEPTH_TEST); glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

static void setupLighting() {
    glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_LIGHT1);
    GLfloat l0p[] = { 10, 10, 20, 1 }; GLfloat l0a[] = { 0.1, 0.1, 0.2, 1 }; GLfloat l0d[] = { 0.8, 0.8, 1, 1 };
    glLightfv(GL_LIGHT0, GL_POSITION, l0p); glLightfv(GL_LIGHT0, GL_AMBIENT, l0a); glLightfv(GL_LIGHT0, GL_DIFFUSE, l0d);
    GLfloat l1p[] = { -10, -5, 5, 1 }; GLfloat l1d[] = { 0.4, 0.2, 0.6, 1 };
    glLightfv(GL_LIGHT1, GL_POSITION, l1p); glLightfv(GL_LIGHT1, GL_DIFFUSE, l1d);
}

static void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glLoadIdentity();
    float sx = gCameraShake*frand(-0.5,0.5), sy = gCameraShake*frand(-0.5,0.5);
    gluLookAt(sx, -22+sy, 22, 0, 0, 0, 0, 1, 0);
    setupLighting();
    if(gState == STATE_HOME) drawHomeScreen();
    else { drawScene(); drawHUD(); drawOverlay(); }
    glutSwapBuffers();
}

static void idle() {
    int cur = glutGet(GLUT_ELAPSED_TIME); float dt = (cur-gLastMs)*0.001f; gLastMs=cur;
    if(dt>0.05) dt=0.05; update(dt); glutPostRedisplay();
}

static void keyboard(unsigned char k, int x, int y) {
    if(k == 27) { stopBgm(); exit(0); }
    if(gState == STATE_HOME && (k == ' ' || k == 13)) resetGame();
    if(gState == STATE_GAMEOVER && (k == 'r' || k == 'R')) resetGame();
    if(gState == STATE_PLAY) { 
        if(k == ' ' ) gKeySpace = true; 
        if(k == 'p' || k == 'P') {
            gPaused = !gPaused;
#ifdef _WIN32
            if(gPaused) mciSendStringA("pause bgm", NULL, 0, NULL);
            else mciSendStringA("resume bgm", NULL, 0, NULL);
#endif
        }
    }
}
static void keyboardUp(unsigned char k, int x, int y) { if(k == ' ') gKeySpace = false; }
static void special(int k, int x, int y) { if(k == GLUT_KEY_LEFT) gKeyLeft = true; if(k == GLUT_KEY_RIGHT) gKeyRight = true; if(k == GLUT_KEY_UP) gKeyUp = true; if(k == GLUT_KEY_DOWN) gKeyDown = true; }
static void specialUp(int k, int x, int y) { if(k == GLUT_KEY_LEFT) gKeyLeft = false; if(k == GLUT_KEY_RIGHT) gKeyRight = false; if(k == GLUT_KEY_UP) gKeyUp = false; if(k == GLUT_KEY_DOWN) gKeyDown = false; }
static void reshape(int w, int h) { 
    WIN_W=w; WIN_H=h?h:1; 
    glViewport(0, 0, WIN_W, WIN_H); 
    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity(); 
    gluPerspective(45, (float)WIN_W/WIN_H, 1, 100); 
    glMatrixMode(GL_MODELVIEW); 
}

int main(int argc, char** argv) {
    srand(time(0)); loadHighScore(); glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(WIN_W, WIN_H); glutCreateWindow("Neo Tanks: Iron Grid");
    glutFullScreen();
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_COLOR_MATERIAL);
    GLfloat lp[] = { 0, 0, 30, 1 }; glLightfv(GL_LIGHT0, GL_POSITION, lp);
    glutDisplayFunc(display); glutReshapeFunc(reshape); glutIdleFunc(idle);
    glutKeyboardFunc(keyboard); glutKeyboardUpFunc(keyboardUp); glutSpecialFunc(special); glutSpecialUpFunc(specialUp);
    gLastMs = glutGet(GLUT_ELAPSED_TIME); 
    startBgm("tanks music/bgm_menu.mp3");
    glutMainLoop();
    return 0;
}
