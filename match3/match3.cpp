/*
 * ================================================================
 *  NEO MATCH-3: CRYSTAL FLUX  –  OpenGL / FreeGLUT [PRO EDITION]
 *  ================================================================
 *
 *  FEATURES
 *  ─────────────────────────────────────────────────────────────
 *  • Full-screen immersive high-fidelity 3D match-3 combat
 *  • Professional Stroke-Font UI with neon glowing titles
 *  • Special Gems: Line Flux (4-Match), Pulse Bomb (T/L), Spectrum (5)
 *  • Tactical Items: Neural Hammer (1), Vortex Shuffle (2), Temporal Freeze (3)
 *  • Smooth swap animations and gravity-based cascading
 *  • Additive glow effects for matches and selection
 *  • Mission Failed "Crystal Depletion" specialized UI
 *
 *  CONTROLS
 *  ─────────────────────────────────────────────────────────────
 *  Arrow Keys        Move Cursor
 *  SPACE             Select / Swap Crystal
 *  1, 2, 3           Tactical Items
 *  P                 Pause / Resume (EMP Stasis)
 *  R                 Restart Game
 *  ESC               Quit
 *
 *  BUILD (Windows / MinGW)
 *  ─────────────────────────────────────────────────────────────
 *    g++ match3.cpp -o match3.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
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

const int GRID_SZ = 8;
const float CELL_SZ = 2.0f;
const float HALF_GRID = (GRID_SZ * CELL_SZ) / 2.0f;

enum GemType { GEM_RED, GEM_GREEN, GEM_BLUE, GEM_YELLOW, GEM_PURPLE, GEM_EMPTY };
enum SpecialType { SP_NONE, SP_LINE_H, SP_LINE_V, SP_BOMB, SP_SPECTRUM };
enum GameState { STATE_HOME, STATE_PLAY, STATE_GAMEOVER };

struct Vec3 { float x,y,z; Vec3(float x=0,float y=0,float z=0):x(x),y(y),z(z) {} };

struct Gem {
    GemType type;
    SpecialType special;
    float visualX, visualY;
    bool matched;
    float scale;
    bool markedForClear;
};

// ──────────────────────────────────────────────────────────────
//  Global State
// ──────────────────────────────────────────────────────────────
static GameState gState = STATE_HOME;
static Gem gGrid[GRID_SZ][GRID_SZ];
static int gCursorX=0, gCursorY=0;
static int gSelectedX=-1, gSelectedY=-1;
static int gScore=0, gHighScore=0;
static float gTimeRemaining = 60.0f;
static float gGlobalTime=0;
static bool gPaused=false;
static int gLastMs=0;
static bool gIsAnimating = false;
static float gCameraShake = 0.0f;

// Tactical Items
static int gHammerCharges = 3;
static int gShuffleCharges = 2;
static int gFreezeCharges = 1;
static float gFreezeTimer = 0.0f;

const char* SAVE_FILE = "highscore.dat";

// ──────────────────────────────────────────────────────────────
//  Utilities
// ──────────────────────────────────────────────────────────────
static float frand(float lo, float hi) { return lo + (hi-lo)*(float)rand()/(float)RAND_MAX; }
static void saveHighScore(){ FILE* f = fopen(SAVE_FILE, "w"); if(f){ fprintf(f, "%d", gHighScore); fclose(f); } }
static void loadHighScore(){ FILE* f = fopen(SAVE_FILE, "r"); if(f){ if(fscanf(f, "%d", &gHighScore) != 1) gHighScore = 0; fclose(f); } }

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
    mciSendStringA("stop bgm", NULL, 0, NULL); mciSendStringA("close bgm", NULL, 0, NULL);
#endif
}
static void startBgm(const char* filename) {
#ifdef _WIN32
    stopBgm(); char cmd[512];
    snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias bgm", filename);
    if (mciSendStringA(cmd, NULL, 0, NULL) != 0) {
        snprintf(cmd, sizeof(cmd), "open \"%s\" alias bgm", filename); mciSendStringA(cmd, NULL, 0, NULL);
    }
    mciSendStringA("play bgm repeat", NULL, 0, NULL);
#endif
}

static void initGrid(){
    for(int y=0; y<GRID_SZ; y++){
        for(int x=0; x<GRID_SZ; x++){
            do { gGrid[y][x].type = (GemType)(rand() % 5); } 
            while((x >= 2 && gGrid[y][x].type == gGrid[y][x-1].type && gGrid[y][x].type == gGrid[y][x-2].type) ||
                  (y >= 2 && gGrid[y][x].type == gGrid[y-1][x].type && gGrid[y][x].type == gGrid[y-2][x].type));
            gGrid[y][x].special = SP_NONE;
            gGrid[y][x].visualX = x * CELL_SZ - HALF_GRID + CELL_SZ/2;
            gGrid[y][x].visualY = y * CELL_SZ - HALF_GRID + CELL_SZ/2;
            gGrid[y][x].matched = false; gGrid[y][x].markedForClear = false; gGrid[y][x].scale = 1.0f;
        }
    }
}

static void triggerSpecial(int x, int y, SpecialType sp, GemType color);

static bool checkMatches(){
    bool found = false;
    for(int y=0; y<GRID_SZ; y++) for(int x=0; x<GRID_SZ; x++) { gGrid[y][x].matched = false; gGrid[y][x].markedForClear = false; }
    
    for(int y=0; y<GRID_SZ; y++){
        for(int x=0; x<GRID_SZ-2; x++){
            GemType t = gGrid[y][x].type; if(t == GEM_EMPTY) continue;
            int count = 1; while(x+count < GRID_SZ && gGrid[y][x+count].type == t) count++;
            if(count >= 3){
                found = true; for(int i=0; i<count; i++) gGrid[y][x+i].matched = true;
                if(count == 4) { gGrid[y][x+(rand()%4)].special = SP_LINE_H; }
                else if(count >= 5) { gGrid[y][x+(rand()%5)].special = SP_SPECTRUM; }
                x += count - 1;
            }
        }
    }
    for(int x=0; x<GRID_SZ; x++){
        for(int y=0; y<GRID_SZ-2; y++){
            GemType t = gGrid[y][x].type; if(t == GEM_EMPTY) continue;
            int count = 1; while(y+count < GRID_SZ && gGrid[y+count][x].type == t) count++;
            if(count >= 3){
                found = true; for(int i=0; i<count; i++) gGrid[y+i][x].matched = true;
                if(count == 4) { gGrid[y+(rand()%4)][x].special = SP_LINE_V; }
                else if(count >= 5) { gGrid[y+(rand()%5)][x].special = SP_SPECTRUM; }
                y += count - 1;
            }
        }
    }
    return found;
}

static void markForClear(int x, int y){
    if(x<0 || x>=GRID_SZ || y<0 || y>=GRID_SZ || gGrid[y][x].markedForClear || gGrid[y][x].type == GEM_EMPTY) return;
    gGrid[y][x].markedForClear = true;
    if(gGrid[y][x].special != SP_NONE) triggerSpecial(x, y, gGrid[y][x].special, gGrid[y][x].type);
}

static void triggerSpecial(int x, int y, SpecialType sp, GemType color){
    if(sp != SP_NONE) playSfx("match3 music/special.wav");
    if(sp == SP_LINE_H){ for(int ix=0; ix<GRID_SZ; ix++) markForClear(ix, y); }
    else if(sp == SP_LINE_V){ for(int iy=0; iy<GRID_SZ; iy++) markForClear(x, iy); }
    else if(sp == SP_BOMB){ for(int dy=-1; dy<=1; dy++) for(int dx=-1; dx<=1; dx++) markForClear(x+dx, y+dy); }
    else if(sp == SP_SPECTRUM){
        for(int iy=0; iy<GRID_SZ; iy++) for(int ix=0; ix<GRID_SZ; ix++) { if(gGrid[iy][ix].type == color) markForClear(ix, iy); }
    }
}

static void applyMatches(){
    int count = 0;
    for(int y=0; y<GRID_SZ; y++) for(int x=0; x<GRID_SZ; x++) if(gGrid[y][x].matched) markForClear(x, y);
    for(int y=0; y<GRID_SZ; y++){
        for(int x=0; x<GRID_SZ; x++){
            if(gGrid[y][x].markedForClear){ gGrid[y][x].type = GEM_EMPTY; gGrid[y][x].special = SP_NONE; count++; }
        }
    }
    gScore += count * 60; if(count > 0) { playSfx("match3 music/match.wav"); gTimeRemaining += count * 0.4f; gCameraShake += count * 0.04f; }
}

static void cascade(){
    for(int x=0; x<GRID_SZ; x++){
        int writeY = 0;
        for(int readY=0; readY<GRID_SZ; readY++){
            if(gGrid[readY][x].type != GEM_EMPTY){
                if(readY != writeY){ gGrid[writeY][x] = gGrid[readY][x]; gGrid[readY][x].type = GEM_EMPTY; }
                writeY++;
            }
        }
        for(int y=writeY; y<GRID_SZ; y++) { 
            gGrid[y][x].type = (GemType)(rand() % 5); gGrid[y][x].special = SP_NONE;
            gGrid[y][x].visualY = HALF_GRID + (y-writeY+1)*CELL_SZ; gGrid[y][x].scale = 1.0f;
        }
    }
}

static void shuffleBoard() {
    std::vector<GemType> gems;
    for(int y=0; y<GRID_SZ; y++) for(int x=0; x<GRID_SZ; x++) gems.push_back(gGrid[y][x].type);
    std::random_shuffle(gems.begin(), gems.end());
    int i=0; for(int y=0; y<GRID_SZ; y++) for(int x=0; x<GRID_SZ; x++) gGrid[y][x].type = gems[i++];
}

static void swap(int x1, int y1, int x2, int y2){
    Gem t1 = gGrid[y1][x1], t2 = gGrid[y2][x2];
    gGrid[y1][x1].type = t2.type; gGrid[y1][x1].special = t2.special;
    gGrid[y2][x2].type = t1.type; gGrid[y2][x2].special = t1.special;
}

static void resetGame(){
    initGrid(); gScore = 0; gTimeRemaining = 60.0f; gState = STATE_PLAY; gSelectedX = -1; gCameraShake = 0;
    gHammerCharges = 3; gShuffleCharges = 2; gFreezeCharges = 1; gFreezeTimer = 0;
    startBgm("match3 music/bgm_play_chill.wav");
}

static void update(float dt){
    if(gState == STATE_HOME) { gGlobalTime += dt; return; }
    if(gPaused) return;
    gGlobalTime += dt;
    if(gState == STATE_PLAY){
        if(gFreezeTimer > 0) gFreezeTimer -= dt; else gTimeRemaining -= dt;
        if(gTimeRemaining <= 0){ gState = STATE_GAMEOVER; playSfx("match3 music/explosion.wav"); startBgm("match3 music/bgm_menu.mp3"); if(gScore > gHighScore){ gHighScore = gScore; saveHighScore(); } }
    }
    gIsAnimating = false;
    for(int y=0; y<GRID_SZ; y++){
        for(int x=0; x<GRID_SZ; x++){
            float tx = x * CELL_SZ - HALF_GRID + CELL_SZ/2, ty = y * CELL_SZ - HALF_GRID + CELL_SZ/2;
            float dx = tx - gGrid[y][x].visualX, dy = ty - gGrid[y][x].visualY;
            if(fabs(dx) > 0.01f || fabs(dy) > 0.01f){ gGrid[y][x].visualX += dx * 0.22f; gGrid[y][x].visualY += dy * 0.22f; gIsAnimating = true; }
        }
    }
    if(!gIsAnimating && gState == STATE_PLAY){ if(checkMatches()){ applyMatches(); cascade(); } }
    gCameraShake *= 0.92f;
}

// ──────────────────────────────────────────────────────────────
//  UI Utilities
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

// ──────────────────────────────────────────────────────────────
//  Rendering
// ──────────────────────────────────────────────────────────────
static void drawGem(int x, int y, const Gem& g){
    if(g.type == GEM_EMPTY) return;
    glPushMatrix(); glTranslatef(g.visualX, g.visualY, 0.5f);
    glRotatef(gGlobalTime * 60.0f, 0, 1, 1);
    float s = g.scale * 0.72f; glScalef(s, s, s);
    switch(g.type){
        case GEM_RED:    glColor3f(1.0f, 0.1f, 0.2f); glutSolidCube(1.2f); break;
        case GEM_GREEN:  glColor3f(0.1f, 1.0f, 0.4f); glutSolidSphere(0.85f, 16, 16); break;
        case GEM_BLUE:   glColor3f(0.1f, 0.6f, 1.0f); glutSolidTorus(0.35f, 0.65f, 12, 18); break;
        case GEM_YELLOW: glColor3f(1.0f, 0.9f, 0.1f); glutSolidCone(0.85f, 1.3f, 8, 4); break;
        case GEM_PURPLE: glColor3f(0.8f, 0.1f, 1.0f); glutSolidIcosahedron(); break;
        default: break;
    }
    glDisable(GL_LIGHTING); glColor3f(1, 1, 1); glLineWidth(2.0f);
    switch(g.type){
        case GEM_RED:    glutWireCube(1.22f); break;
        case GEM_GREEN:  glutWireSphere(0.87f, 8, 8); break;
        case GEM_BLUE:   glutWireTorus(0.35f, 0.67f, 6, 12); break;
        case GEM_YELLOW: glutWireCone(0.87f, 1.32f, 6, 2); break;
        case GEM_PURPLE: glutWireIcosahedron(); break;
        default: break;
    }
    if(g.special != SP_NONE){
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        float p = 0.5f + 0.5f*sinf(gGlobalTime*10.0f); glColor4f(1, 1, 1, 0.3f*p);
        if(g.special == SP_LINE_H || g.special == SP_LINE_V) glutSolidTorus(0.1f, 1.2f, 8, 16);
        else if(g.special == SP_BOMB) glutSolidSphere(1.4f, 16, 16);
        else if(g.special == SP_SPECTRUM) { glRotatef(gGlobalTime*100, 1,0,0); glutWireSphere(1.6f, 12, 12); }
        glDisable(GL_BLEND);
    }
    glEnable(GL_LIGHTING); glPopMatrix();
}

static void drawScene(){
    glDisable(GL_LIGHTING); glColor3f(0, 0, 0.08f);
    glBegin(GL_QUADS); glVertex3f(-HALF_GRID-1, -HALF_GRID-1, -0.2f); glVertex3f(HALF_GRID+1, -HALF_GRID-1, -0.2f); glVertex3f(HALF_GRID+1, HALF_GRID+1, -0.2f); glVertex3f(-HALF_GRID-1, HALF_GRID+1, -0.2f); glEnd();
    float pulse = 0.12f + 0.05f*sinf(gGlobalTime*2.0f); glColor3f(pulse*0.4f, pulse*0.7f, pulse*1.0f);
    glLineWidth(1.5f); glBegin(GL_LINES); for(float i=-HALF_GRID; i<=HALF_GRID; i+=CELL_SZ){ glVertex3f(i, -HALF_GRID, 0); glVertex3f(i, HALF_GRID, 0); glVertex3f(-HALF_GRID, i, 0); glVertex3f(HALF_GRID, i, 0); } glEnd();
    glEnable(GL_LIGHTING);
    for(int y=0; y<GRID_SZ; y++) for(int x=0; x<GRID_SZ; x++) drawGem(x, y, gGrid[y][x]);
    glDisable(GL_LIGHTING);
    float cx = gCursorX * CELL_SZ - HALF_GRID + CELL_SZ/2, cy = gCursorY * CELL_SZ - HALF_GRID + CELL_SZ/2;
    glColor3f(1, 1, 1); glLineWidth(3.5f); glPushMatrix(); glTranslatef(cx, cy, 0.8f); glutWireCube(CELL_SZ * 0.98f); glPopMatrix();
    if(gSelectedX != -1){
        float sx = gSelectedX * CELL_SZ - HALF_GRID + CELL_SZ/2, sy = gSelectedY * CELL_SZ - HALF_GRID + CELL_SZ/2;
        glColor3f(1, 1, 0); glPushMatrix(); glTranslatef(sx, sy, 0.82f); glutWireCube(CELL_SZ * 0.92f); glPopMatrix();
    }
    glEnable(GL_LIGHTING);
}

static void drawHUD(){
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity(); glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0, 0.04, 0.12, 0.88); glRecti(0, WIN_H-65, WIN_W, WIN_H);
    glColor4f(0, 0.7, 1.0, 0.7); glLineWidth(2.5f); glBegin(GL_LINES); glVertex2f(0, WIN_H-65); glVertex2f(WIN_W, WIN_H-65); glEnd();
    char buf[128]; glColor3f(0, 1, 1);
    snprintf(buf, sizeof(buf), "NEURAL SCORE: %06d", gScore); bitmapAt(30, WIN_H-42, buf, GLUT_BITMAP_TIMES_ROMAN_24);
    if(gFreezeTimer > 0) glColor3f(0.4, 0.8, 1);
    snprintf(buf, sizeof(buf), "TIME FLUX: %.1fs", gTimeRemaining); bitmapCentered(WIN_W*0.5f, WIN_H-42, buf, GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0, 1, 1); snprintf(buf, sizeof(buf), "PEAK RECORD: %06d", gHighScore); bitmapAt(WIN_W-280, WIN_H-42, buf, GLUT_BITMAP_TIMES_ROMAN_24);
    // Items
    glColor3f(1, 0.8, 0.2); 
    snprintf(buf, sizeof(buf), "1:HAMMER[%d]  2:SHUFFLE[%d]  3:FREEZE[%d]", gHammerCharges, gShuffleCharges, gFreezeCharges);
    bitmapCentered(WIN_W*0.5f, 30, buf, GLUT_BITMAP_HELVETICA_18);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

static void drawHomeScreen() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity(); glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glColor4f(0, 0, 0.05, 0.9); glRecti(0, 0, WIN_W, WIN_H);
    float p = 0.8f + 0.2f*sinf(gGlobalTime*3.0f); strokeCentered(WIN_W*0.5, WIN_H*0.65, "NEO MATCH-3", 0.45, 0, 0.7, 1, p);
    strokeCentered(WIN_W*0.5, WIN_H*0.48, "CRYSTAL FLUX", 0.35, 1, 0.6, 0, p);
    float b = 0.5f + 0.5f*sinf(gGlobalTime*6.0f); glColor3f(b, b, b); bitmapCentered(WIN_W*0.5, WIN_H*0.25, "[ PRESS SPACE TO INITIALIZE CORE ]", GLUT_BITMAP_TIMES_ROMAN_24);
    glColor3f(0.3, 0.4, 0.6); bitmapCentered(WIN_W*0.5, WIN_H*0.1, "Developed by AL AMIN HOSSAIN", GLUT_BITMAP_HELVETICA_18);
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

static void drawOverlay() {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity(); glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    float cx = WIN_W*0.5f;
    if(gState == STATE_GAMEOVER) {
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glColor4f(0.12, 0, 0, 0.92); glRecti(0, 0, WIN_W, WIN_H);
        glBegin(GL_QUADS); glColor4f(0.04, 0, 0.08, 0.75); glVertex2f(cx-300, WIN_H*0.2); glVertex2f(cx+300, WIN_H*0.2); glVertex2f(cx+300, WIN_H*0.8); glVertex2f(cx-300, WIN_H*0.8); glEnd();
        strokeCentered(cx, WIN_H*0.7, "CRYSTAL DEPLETION", 0.28, 1, 0.1, 0, 1);
        char buf[128]; glColor3f(1, 0.8, 0); snprintf(buf, sizeof(buf), "FINAL FLUX: %06d", gScore); bitmapCentered(cx, WIN_H*0.55, buf, GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.4, 0.7, 1); snprintf(buf, sizeof(buf), "PEAK RECORD: %06d", gHighScore); bitmapCentered(cx, WIN_H*0.48, buf, GLUT_BITMAP_HELVETICA_18);
        float blink = 0.5f + 0.5f*sinf(gGlobalTime*5.0f); glColor4f(blink, blink, blink, 1); bitmapCentered(cx, WIN_H*0.35, "[ Press R to Restart Neural Connection ]", GLUT_BITMAP_HELVETICA_18);
    } else if(gPaused) {
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glColor4f(0, 0, 0, 0.6); glRecti(0, 0, WIN_W, WIN_H);
        strokeCentered(cx, WIN_H*0.55, "PAUSED", 0.35, 1, 1, 0, 1);
    }
    glEnable(GL_DEPTH_TEST); glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

static void display(){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glLoadIdentity();
    float sx = gCameraShake*frand(-0.5,0.5), sy = gCameraShake*frand(-0.5,0.5); gluLookAt(sx, sy, 25, 0, 0, 0, 0, 1, 0);
    GLfloat lp[] = { 5, 5, 20, 1 }; glLightfv(GL_LIGHT0, GL_POSITION, lp);
    if(gState == STATE_HOME) drawHomeScreen(); else { drawScene(); drawHUD(); drawOverlay(); }
    glutSwapBuffers();
}

static void timer(int v){
    int cur = glutGet(GLUT_ELAPSED_TIME); float dt = (cur - gLastMs) / 1000.0f; gLastMs = cur;
    if(dt > 0.05f) dt = 0.05f; update(dt); glutPostRedisplay(); glutTimerFunc(16, timer, 0);
}

static void special(int k, int x, int y){
    if(gState != STATE_PLAY || gPaused) return;
    if(k == GLUT_KEY_UP) gCursorY++; if(k == GLUT_KEY_DOWN) gCursorY--; if(k == GLUT_KEY_LEFT) gCursorX--; if(k == GLUT_KEY_RIGHT) gCursorX++;
    if(gCursorX < 0) gCursorX = 0; if(gCursorX >= GRID_SZ) gCursorX = GRID_SZ-1;
    if(gCursorY < 0) gCursorY = 0; if(gCursorY >= GRID_SZ) gCursorY = GRID_SZ-1;
}

static void keyboard(unsigned char k, int x, int y){
    if(k == 27) { stopBgm(); exit(0); }
    if(k == 'p' || k == 'P') { gPaused = !gPaused; 
#ifdef _WIN32
        if(gPaused) mciSendStringA("pause bgm",0,0,0); else mciSendStringA("resume bgm",0,0,0);
#endif
    }
    if(k == 'r' || k == 'R') if(gState == STATE_GAMEOVER) resetGame();
    if(gState == STATE_PLAY && !gPaused) {
        if(k == '1' && gHammerCharges > 0) { playSfx("match3 music/hammer.wav"); markForClear(gCursorX, gCursorY); applyMatches(); cascade(); gHammerCharges--; gCameraShake += 0.5f; }
        if(k == '2' && gShuffleCharges > 0) { playSfx("match3 music/shuffle.wav"); shuffleBoard(); gShuffleCharges--; gCameraShake += 0.8f; }
        if(k == '3' && gFreezeCharges > 0) { playSfx("match3 music/freeze.wav"); gFreezeTimer = 10.0f; gFreezeCharges--; }
    }
    if(k == ' '){
        if(gState != STATE_PLAY) resetGame();
        else {
            if(gIsAnimating || gPaused) return;
            if(gSelectedX == -1){ gSelectedX = gCursorX; gSelectedY = gCursorY; playSfx("match3 music/swap.wav"); } 
            else {
                int dx = abs(gCursorX - gSelectedX), dy = abs(gCursorY - gSelectedY);
                if((dx == 1 && dy == 0) || (dx == 0 && dy == 1)){
                    playSfx("match3 music/swap.wav");
                    swap(gSelectedX, gSelectedY, gCursorX, gCursorY);
                    if(!checkMatches()){ swap(gSelectedX, gSelectedY, gCursorX, gCursorY); playSfx("match3 music/swap.wav"); }
                    gSelectedX = -1;
                } else { gSelectedX = gCursorX; gSelectedY = gCursorY; }
            }
        }
    }
}

static void reshape(int w, int h){
    WIN_W = w; WIN_H = h?h:1; glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(45, (float)w/h, 1, 100); glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv){
    srand(time(0)); loadHighScore(); glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(WIN_W, WIN_H); glutCreateWindow("Neo Match-3: Crystal Flux");
    glutFullScreen();
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_COLOR_MATERIAL);
    glutDisplayFunc(display); glutReshapeFunc(reshape); glutTimerFunc(0, timer, 0);
    glutKeyboardFunc(keyboard); glutSpecialFunc(special);
    gLastMs = glutGet(GLUT_ELAPSED_TIME); 
    startBgm("match3 music/bgm_menu.mp3");
    glutMainLoop();
    return 0;
}
