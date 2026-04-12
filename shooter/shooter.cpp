/*
 * ================================================================
 *  NEO SHOOTER 3D  –  OpenGL / FreeGLUT  [VISUAL ENHANCED EDITION]
 *  ================================================================
 *
 *  FEATURES
 *  ─────────────────────────────────────────────────────────────
 *  • MSAA anti-aliasing & smooth idle-based timing
 *  • 600 twinkling colored stars (white / blue-white / warm gold)
 *  • 5 drifting background asteroids (decorative, no collision)
 *  • Detailed 3D player ship – cockpit, swept wings, dual engine glow
 *  • 3 enemy visual types based on HP (red fighter / orange heavy / purple elite)
 *  • Quad-based additive particle explosions
 *  • Glowing quad-based bullet rendering
 *  • Two-light scene (key + rim) for premium 3D look
 *  • Pixel-coord semi-transparent HUD panel
 *  • Stroke-font "MISSION FAILED" title + polished overlay screens
 *  • All original gameplay, controls, orientation, audio UNCHANGED
 *
 *  CONTROLS  (identical to original)
 *  ─────────────────────────────────────────────────────────────
 *  Arrow Left/Right   Move ship
 *  SPACE              Fire laser
 *  P                  Pause / Resume
 *  R                  Restart
 *  ESC                Quit
 *
 *  BUILD (Windows / MinGW)
 *  ─────────────────────────────────────────────────────────────
 *    g++ shooter.cpp -o shooter.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
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
//  Window / Scene constants  (unchanged from original)
// ──────────────────────────────────────────────────────────────
static int WIN_W = 700;
static int WIN_H = 800;

static const float ARENA_W  = 15.0f;
static const float ARENA_H  = 25.0f;
static const int   MAX_STARS = 600;
static const int   MAX_AST   = 5;      // background asteroids (decorative)

// ──────────────────────────────────────────────────────────────
//  Structs
// ──────────────────────────────────────────────────────────────
struct Vec3 {
    float x, y, z;
    Vec3(float x=0,float y=0,float z=0):x(x),y(y),z(z){}
};

struct Bullet   { Vec3 pos; bool active; };
struct Enemy    { Vec3 pos; int hp; bool active; };
struct Particle { Vec3 pos, vel; float life; float r, g, b; };

struct Star {
    Vec3  pos;
    float speed, brightness, twinkle, twinkleSpd;
    float r, g, b;
};

struct AsteroidBg {
    Vec3  pos;
    float vx, vy;
    float rotAngle, rotSpd;
    float rotAx, rotAy, rotAz;
    float scale;
    int   shape;
    float r, g, b;
};

enum PowerupType { PT_TRIPLE, PT_SHIELD, PT_RAPID, PT_SLOW, PT_HEALTH };
struct Powerup   { Vec3 pos; PowerupType type; bool active; };

// ──────────────────────────────────────────────────────────────
//  Game State  (unchanged semantics from original)
// ──────────────────────────────────────────────────────────────
static float gPlayerX = 0.0f;
static const float gPlayerY = 2.0f;

static std::vector<Bullet>   gBullets;
static std::vector<Enemy>    gEnemies;
static std::vector<Particle> gParticles;
static std::vector<Powerup>  gPowerups;

static Star       gStarArr[MAX_STARS];
static AsteroidBg gAstArr[MAX_AST];

static bool gKeyLeft  = false;
static bool gKeyRight = false;
static bool gKeySpace = false;

static float gFireCooldown    = 0.0f;
static float gEnemySpawnTimer = 0.0f;
static float gDifficulty      = 1.0f;

static int  gScore     = 0;
static int  gHighScore = 0;
static int  gPlayerHP  = 3;
enum GameState { STATE_HOME, STATE_PLAY, STATE_GAMEOVER };
static GameState gState = STATE_HOME;

static float gGlobalTime     = 0.0f;
static float gCameraShake    = 0.0f;
static bool  gShieldActive   = false;
static bool  gPaused         = false;
static float gTripleShotTimer= 0.0f;
static float gRapidFireTimer = 0.0f;
static float gTimeSlowTimer  = 0.0f;
static char  gPowerupMsg[64] = "";
static float gPowerupMsgTimer= 0.0f;
static float gMsgColor[3]    = {1,1,1};

static int   gLastMs = 0;
static const char* SAVE_FILE = "highscore.dat";

// ──────────────────────────────────────────────────────────────
//  Persistence
// ──────────────────────────────────────────────────────────────
static void saveHighScore(){
    FILE* f = fopen(SAVE_FILE, "w");
    if(f){
        fprintf(f, "%d", gHighScore);
        fclose(f);
    }
}
static void loadHighScore(){
    FILE* f = fopen(SAVE_FILE, "r");
    if(f){
        if(fscanf(f, "%d", &gHighScore) != 1) gHighScore = 0;
        fclose(f);
    }
}
static float frand(float lo, float hi){
    return lo + (hi-lo)*(float)rand()/(float)RAND_MAX;
}

// ──────────────────────────────────────────────────────────────
//  Starfield  (600 twinkling colored stars)
// ──────────────────────────────────────────────────────────────
static void initStars(){
    for(int i=0;i<MAX_STARS;i++){
        Star& s = gStarArr[i];
        s.pos        = Vec3(frand(-ARENA_W*2,ARENA_W*2), frand(0,ARENA_H), frand(-5,-2));
        s.speed      = frand(2.0f,5.0f);
        s.brightness = frand(0.2f,1.0f);
        s.twinkle    = frand(0,6.283f);
        s.twinkleSpd = frand(0.5f,2.5f);
        int ct = rand()%10;
        if(ct<6)      { s.r=1.0f; s.g=1.0f; s.b=1.0f; }   // white
        else if(ct<8) { s.r=0.7f; s.g=0.8f; s.b=1.0f; }   // blue-white
        else          { s.r=1.0f; s.g=0.9f; s.b=0.6f; }   // warm gold
    }
}

static void updateStars(float dt, float timeScale){
    for(int i=0;i<MAX_STARS;i++){
        gStarArr[i].pos.y   -= gStarArr[i].speed * dt * timeScale;
        gStarArr[i].twinkle += gStarArr[i].twinkleSpd * dt;
        if(gStarArr[i].pos.y < 0.0f){
            gStarArr[i].pos.y = ARENA_H;
            gStarArr[i].pos.x = frand(-ARENA_W*2, ARENA_W*2);
        }
    }
}

static void drawStars(){
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPointSize(1.8f);
    glBegin(GL_POINTS);
    for(int i=0;i<MAX_STARS;i++){
        const Star& s = gStarArr[i];
        float tw = 0.5f + 0.5f*sinf(s.twinkle);
        float b  = s.brightness * tw;
        glColor4f(s.r*b, s.g*b, s.b*b, 1.0f);
        glVertex3f(s.pos.x, s.pos.y, s.pos.z);
    }
    glEnd();
    glPointSize(1.0f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// ──────────────────────────────────────────────────────────────
//  Background Asteroids  (purely decorative – NO collision)
// ──────────────────────────────────────────────────────────────
static void resetAsteroid(AsteroidBg& a, bool fromTop){
    a.pos.x    = frand(-ARENA_W+1, ARENA_W-1);
    a.pos.y    = fromTop ? ARENA_H + frand(2,6) : frand(0, ARENA_H);
    a.pos.z    = frand(-3,-1);
    a.vx       = frand(-0.5f, 0.5f);
    a.vy       = frand(-2.0f,-0.5f);
    a.rotAngle = frand(0,360);
    a.rotSpd   = frand(20,80)*(rand()%2?1:-1);
    a.rotAx = frand(-1,1); a.rotAy = frand(-1,1); a.rotAz = frand(-1,1);
    float len  = sqrtf(a.rotAx*a.rotAx+a.rotAy*a.rotAy+a.rotAz*a.rotAz);
    if(len<0.01f) len=1;
    a.rotAx/=len; a.rotAy/=len; a.rotAz/=len;
    a.scale = frand(0.3f, 0.85f);
    a.shape = rand()%3;
    int ct  = rand()%3;
    if(ct==0){ a.r=0.40f;a.g=0.35f;a.b=0.30f; }
    else if(ct==1){ a.r=0.50f;a.g=0.48f;a.b=0.44f; }
    else{ a.r=0.34f;a.g=0.31f;a.b=0.28f; }
}

static void initAsteroidsBg(){
    for(int i=0;i<MAX_AST;i++) resetAsteroid(gAstArr[i], false);
}

static void updateAsteroidsBg(float dt){
    for(int i=0;i<MAX_AST;i++){
        gAstArr[i].pos.x    += gAstArr[i].vx * dt;
        gAstArr[i].pos.y    += gAstArr[i].vy * dt;
        gAstArr[i].rotAngle += gAstArr[i].rotSpd * dt;
        if(gAstArr[i].pos.y < -2.0f) resetAsteroid(gAstArr[i], true);
    }
}

static void drawAsteroidsBg(){
    glEnable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    for(int i=0;i<MAX_AST;i++){
        AsteroidBg& a = gAstArr[i];
        GLfloat diff[] = {a.r,a.g,a.b,1};
        GLfloat amb[]  = {a.r*0.25f,a.g*0.25f,a.b*0.25f,1};
        GLfloat spec[] = {0.12f,0.12f,0.12f,1};
        glMaterialfv(GL_FRONT,GL_DIFFUSE, diff);
        glMaterialfv(GL_FRONT,GL_AMBIENT, amb);
        glMaterialfv(GL_FRONT,GL_SPECULAR,spec);
        glMaterialf (GL_FRONT,GL_SHININESS,10);
        glPushMatrix();
        glTranslatef(a.pos.x, a.pos.y, a.pos.z);
        glRotatef(a.rotAngle, a.rotAx, a.rotAy, a.rotAz);
        glPushMatrix();
        glScalef(a.scale,a.scale,a.scale);
        switch(a.shape){
            case 0: glutSolidSphere(0.6f,6,4); break;
            case 1: glScalef(1.5f,0.7f,0.9f); glutSolidSphere(0.55f,5,4); break;
            default: glutSolidCube(0.62f); break;
        }
        glPopMatrix();
        glPopMatrix();
    }
    glEnable(GL_COLOR_MATERIAL);
}

// ──────────────────────────────────────────────────────────────
//  Particles  (quad-based additive blending for bright explosions)
// ──────────────────────────────────────────────────────────────
static void spawnParticles(Vec3 pos, float r, float g, float b, int count){
    for(int i=0;i<count;i++){
        Particle p;
        p.pos = pos;
        p.vel = Vec3(frand(-0.3f,0.3f), frand(-0.3f,0.3f), frand(0.1f,0.5f));
        p.r=r; p.g=g; p.b=b;
        p.life = 1.0f;
        gParticles.push_back(p);
    }
}

static void updateParticles(float dt){
    for(size_t i=0;i<gParticles.size();i++){
        gParticles[i].pos.x += gParticles[i].vel.x * dt * 60.0f;
        gParticles[i].pos.y += gParticles[i].vel.y * dt * 60.0f;
        gParticles[i].pos.z += gParticles[i].vel.z * dt * 60.0f;
        gParticles[i].vel.z -= dt * 1.5f;
        gParticles[i].life  -= dt * 1.5f;
    }
    gParticles.erase(
        std::remove_if(gParticles.begin(),gParticles.end(),
            [](const Particle& p){ return p.life<=0; }),
        gParticles.end());
}

static void drawParticles(){
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // additive for bright sparks
    glBegin(GL_QUADS);
    for(size_t i=0;i<gParticles.size();i++){
        float l  = gParticles[i].life;
        float a  = l * l;
        float sz = 0.20f * l;
        float x  = gParticles[i].pos.x;
        float y  = gParticles[i].pos.y;
        float z  = gParticles[i].pos.z;
        glColor4f(gParticles[i].r, gParticles[i].g, gParticles[i].b, a);
        glVertex3f(x-sz,y-sz,z);
        glVertex3f(x+sz,y-sz,z);
        glVertex3f(x+sz,y+sz,z);
        glVertex3f(x-sz,y+sz,z);
    }
    glEnd();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}

// ──────────────────────────────────────────────────────────────
//  Lighting  (key light + purple rim light)
// ──────────────────────────────────────────────────────────────
static void setupLighting(){
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_NORMALIZE);

    // Key light: cool white from upper-right
    GLfloat l0pos[] = { 5.0f, ARENA_H*0.65f, 12.0f, 1.0f };
    GLfloat l0amb[] = { 0.04f,0.04f,0.10f,1 };
    GLfloat l0dif[] = { 0.90f,0.90f,1.00f,1 };
    GLfloat l0spc[] = { 1.00f,1.00f,1.00f,1 };
    glLightfv(GL_LIGHT0,GL_POSITION,l0pos);
    glLightfv(GL_LIGHT0,GL_AMBIENT, l0amb);
    glLightfv(GL_LIGHT0,GL_DIFFUSE, l0dif);
    glLightfv(GL_LIGHT0,GL_SPECULAR,l0spc);

    // Rim light: purple-blue from lower-left
    GLfloat l1pos[] = { -4.0f, 3.0f, -5.0f, 1.0f };
    GLfloat l1dif[] = { 0.35f,0.10f,0.55f,1 };
    GLfloat l1amb[] = { 0.01f,0.00f,0.02f,1 };
    glLightfv(GL_LIGHT1,GL_POSITION,l1pos);
    glLightfv(GL_LIGHT1,GL_DIFFUSE, l1dif);
    glLightfv(GL_LIGHT1,GL_AMBIENT, l1amb);
}

// ──────────────────────────────────────────────────────────────
//  Player Ship  (detailed 3D: cockpit, swept wings, dual glow engines)
// ──────────────────────────────────────────────────────────────
static void drawPlayerShip(float x, float y){
    glPushMatrix();
    glTranslatef(x, y, 0.5f);
    glDisable(GL_COLOR_MATERIAL);

    // ── Fuselage ──
    GLfloat bodyAmb[]  = {0.02f,0.07f,0.22f,1};
    GLfloat bodyDiff[] = {0.15f,0.52f,1.00f,1};
    GLfloat bodySpec[] = {0.70f,0.85f,1.00f,1};
    glMaterialfv(GL_FRONT,GL_AMBIENT,  bodyAmb);
    glMaterialfv(GL_FRONT,GL_DIFFUSE,  bodyDiff);
    glMaterialfv(GL_FRONT,GL_SPECULAR, bodySpec);
    glMaterialf (GL_FRONT,GL_SHININESS,90);
    glPushMatrix(); glScalef(0.30f,0.90f,0.28f); glutSolidSphere(1.0f,14,8); glPopMatrix();

    // ── Nose cone ──
    glPushMatrix();
    glTranslatef(0,1.1f,0); glRotatef(-90,1,0,0);
    glScalef(0.16f,0.16f,0.55f); glutSolidCone(1.0f,1.0f,10,4);
    glPopMatrix();

    // ── Cockpit canopy (glassy, translucent) ──
    GLfloat cockDiff[] = {0.20f,0.40f,0.85f,0.55f};
    GLfloat cockSpec[] = {1.00f,1.00f,1.00f,1.00f};
    glMaterialfv(GL_FRONT,GL_DIFFUSE, cockDiff);
    glMaterialfv(GL_FRONT,GL_SPECULAR,cockSpec);
    glMaterialf (GL_FRONT,GL_SHININESS,120);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPushMatrix(); glTranslatef(0,0.45f,0.22f); glScalef(0.13f,0.22f,0.10f); glutSolidSphere(1.0f,10,8); glPopMatrix();
    glDisable(GL_BLEND);

    // ── Wings ──
    GLfloat wingDiff[] = {0.10f,0.32f,0.88f,1};
    glMaterialfv(GL_FRONT,GL_DIFFUSE, wingDiff);
    glMaterialf (GL_FRONT,GL_SHININESS,60);
    for(int side=-1;side<=1;side+=2){
        glPushMatrix();
        glTranslatef(side*0.72f,-0.15f,0);
        glRotatef(side*-12.0f,0,0,1);
        glScalef(0.85f,0.07f,0.38f);
        glutSolidCube(1.0f);
        glPopMatrix();
        // Wing tip pod
        glPushMatrix(); glTranslatef(side*1.12f,-0.25f,0); glScalef(0.10f,0.28f,0.26f); glutSolidSphere(1.0f,8,6); glPopMatrix();
    }

    // ── Tail fin ──
    GLfloat finDiff[] = {0.05f,0.24f,0.72f,1};
    glMaterialfv(GL_FRONT,GL_DIFFUSE,finDiff);
    glPushMatrix(); glTranslatef(0,-0.68f,0.28f); glScalef(0.06f,0.36f,0.30f); glutSolidCube(1.0f); glPopMatrix();

    // ── Engine pods ──
    GLfloat engDiff[] = {0.06f,0.06f,0.14f,1};
    glMaterialfv(GL_FRONT,GL_DIFFUSE,engDiff);
    for(int side=-1;side<=1;side+=2){
        glPushMatrix(); glTranslatef(side*0.42f,-0.72f,0); glScalef(0.14f,0.30f,0.14f); glutSolidSphere(1.0f,8,6); glPopMatrix();
    }

    // ── Engine flame glow (additive) ──
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    float fl = 0.65f + 0.35f*sinf(gGlobalTime*14.0f);
    for(int side=-1;side<=1;side+=2){
        // Inner bright core
        glColor4f(0.4f,0.75f,1.0f, 0.90f*fl);
        glPushMatrix(); glTranslatef(side*0.42f,-1.02f-fl*0.08f,0); glRotatef(90,1,0,0); glutSolidCone(0.057f,0.38f*fl,8,3); glPopMatrix();
        // Outer glow corona
        glColor4f(0.10f,0.40f,1.0f, 0.38f*fl);
        glPushMatrix(); glTranslatef(side*0.42f,-1.06f,0); glRotatef(90,1,0,0); glutSolidCone(0.12f,0.58f*fl,8,3); glPopMatrix();
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);

    // ── Shield bubble (when active) ──
    if(gShieldActive){
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        float sp = 0.28f + 0.14f*sinf(gGlobalTime*4.0f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(0.0f,0.55f,1.0f, sp);
        glutSolidSphere(1.62f,18,18);
        glColor4f(0.5f,0.9f,1.0f, 0.65f);
        glLineWidth(1.5f); glutWireSphere(1.65f,18,18); glLineWidth(1.0f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }

    glEnable(GL_COLOR_MATERIAL);
    glPopMatrix();
}

// ──────────────────────────────────────────────────────────────
//  drawBox helper  (used by enemy and powerup)
// ──────────────────────────────────────────────────────────────
static void drawBox(float x, float y, float z, float w, float h, float d,
                    float r, float g, float b, float alpha=1.0f){
    glPushMatrix();
    glTranslatef(x,y,z);
    glScalef(w,h,d);

    GLfloat diff[] = {r*0.8f,g*0.8f,b*0.8f,alpha};
    GLfloat ambi[] = {r*0.3f,g*0.3f,b*0.3f,alpha};
    GLfloat spec[] = {1.0f,1.0f,1.0f,alpha};
    glMaterialfv(GL_FRONT,GL_DIFFUSE,  diff);
    glMaterialfv(GL_FRONT,GL_AMBIENT,  ambi);
    glMaterialfv(GL_FRONT,GL_SPECULAR, spec);
    glMaterialf (GL_FRONT,GL_SHININESS,60.0f);

    if(alpha<1.0f){ glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); }
    glutSolidCube(1.0f);
    // Wireframe glow
    glDisable(GL_LIGHTING);
    glColor4f(r,g,b,alpha);
    glLineWidth(2.0f);
    glutWireCube(1.02f);
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
    if(alpha<1.0f) glDisable(GL_BLEND);
    glPopMatrix();
}

// ──────────────────────────────────────────────────────────────
//  Enemy Ship  (classic geometry upgraded with premium neon rendering)
// ──────────────────────────────────────────────────────────────
static void drawEnemyShip(float x, float y, int hp){
    glPushMatrix();
    glTranslatef(x, y, 0.5f);
    
    // Original Z-spin but with a subtle dynamic axis tilt
    float tilt = 15.0f * sinf(gGlobalTime * 2.0f + x);
    glRotatef(tilt, 1, 1, 0);
    glRotatef(gGlobalTime * 150.0f, 0, 0, 1);

    // Neon colors based on HP
    float er = 1.0f;
    float eg = (hp > 1) ? 0.6f : 0.05f;
    float eb = (hp > 1) ? 0.0f : 0.15f;
    
    float pulse = 0.8f + 0.2f * sinf(gGlobalTime * 12.0f);

    glDisable(GL_COLOR_MATERIAL);

    // 1. Solid glossy core
    GLfloat coreDiff[] = {er*0.5f, eg*0.5f, eb*0.5f, 1.0f};
    GLfloat coreSpec[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat coreAmb[]  = {er*0.2f, eg*0.2f, eb*0.2f, 1.0f};
    glMaterialfv(GL_FRONT, GL_DIFFUSE,  coreDiff);
    glMaterialfv(GL_FRONT, GL_SPECULAR, coreSpec);
    glMaterialfv(GL_FRONT, GL_AMBIENT,  coreAmb);
    glMaterialf(GL_FRONT, GL_SHININESS, 120.0f);

    glutSolidCube(1.0f);

    // 2. Core neon wireframe outline (pulsing)
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(er * pulse, eg * pulse, eb * pulse, 0.8f);
    glLineWidth(3.0f);
    glutWireCube(1.05f);
    glLineWidth(1.0f);

    // 3. Four Spike Cones
    for(int i=0; i<4; i++){
        glPushMatrix();
        glRotatef(i * 90.0f, 0, 0, 1);
        glTranslatef(0.55f, 0, 0); // anchor at edge of cube
        glRotatef(90, 0, 1, 0);
        
        // Base dark glossy spike
        glEnable(GL_LIGHTING);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // standard blend for solid
        GLfloat spikeDiff[] = {0.05f, 0.02f, 0.05f, 1.0f};
        GLfloat spikeSpec[] = {0.8f, 0.8f, 0.8f, 1.0f};
        glMaterialfv(GL_FRONT, GL_DIFFUSE, spikeDiff);
        glMaterialfv(GL_FRONT, GL_SPECULAR, spikeSpec);
        glMaterialf(GL_FRONT, GL_SHININESS, 80.0f);
        glutSolidCone(0.25f, 0.5f, 12, 2);

        // Glowing neon rings winding around the spike
        glDisable(GL_LIGHTING);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(er, eg, eb, 0.8f);
        glTranslatef(0.0f, 0.0f, 0.02f);
        glutWireCone(0.28f, 0.5f, 6, 3);
        glEnable(GL_LIGHTING);

        glPopMatrix();
    }
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
    glEnable(GL_COLOR_MATERIAL);

    glPopMatrix();
}

// ──────────────────────────────────────────────────────────────
//  Powerups  (glowing rotating gem shape)
// ──────────────────────────────────────────────────────────────
static void drawPowerup(const Powerup& p){
    glPushMatrix();
    glTranslatef(p.pos.x, p.pos.y, 0.5f);
    glRotatef(gGlobalTime*150.0f, 0,1,0);
    float bob = 0.10f*sinf(gGlobalTime*3.0f + p.pos.x);
    glTranslatef(0,bob,0);

    float pr=1,pg=1,pb=1;
    if     (p.type==PT_TRIPLE){ pr=1.0f;pg=1.0f;pb=0.0f; }
    else if(p.type==PT_SHIELD){ pr=0.0f;pg=1.0f;pb=1.0f; }
    else if(p.type==PT_RAPID) { pr=1.0f;pg=0.5f;pb=0.0f; }
    else if(p.type==PT_SLOW)  { pr=1.0f;pg=0.0f;pb=1.0f; }
    else                      { pr=0.0f;pg=1.0f;pb=0.0f; }

    glDisable(GL_COLOR_MATERIAL);
    GLfloat pdiff[] = {pr,pg,pb,1};
    GLfloat pamb[]  = {pr*0.3f,pg*0.3f,pb*0.3f,1};
    GLfloat pspec[] = {1,1,1,1};
    glMaterialfv(GL_FRONT,GL_DIFFUSE, pdiff);
    glMaterialfv(GL_FRONT,GL_AMBIENT, pamb);
    glMaterialfv(GL_FRONT,GL_SPECULAR,pspec);
    glMaterialf (GL_FRONT,GL_SHININESS,85);

    // Special rendering for Extra Life (Cross)
    if (p.type == PT_HEALTH) {
        float pulse = 0.5f + 0.5f * sinf(gGlobalTime * 8.0f); // Fast heart pulse
        
        glDisable(GL_COLOR_MATERIAL);
        GLfloat crossDiff[] = {1.0f, 1.0f, 1.0f, 1.0f}; // Pure White
        GLfloat crossSpec[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glMaterialfv(GL_FRONT, GL_DIFFUSE, crossDiff);
        glMaterialfv(GL_FRONT, GL_SPECULAR, crossSpec);
        glMaterialf(GL_FRONT, GL_SHININESS, 100.0f);

        // Vertical Bar
        glPushMatrix();
        glScalef(0.3f, 0.9f, 0.3f);
        glutSolidCube(1.0f);
        glPopMatrix();

        // Horizontal Bar
        glPushMatrix();
        glScalef(0.9f, 0.3f, 0.3f);
        glutSolidCube(1.0f);
        glPopMatrix();

        // Glow halo (additive Red)
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 0.1f, 0.1f, 0.4f + pulse * 0.4f);
        glLineWidth(2.0f);
        glPushMatrix(); glScalef(1.2f, 1.2f, 1.2f); glutWireCube(0.8f); glPopMatrix();
        glLineWidth(1.0f);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
        glEnable(GL_COLOR_MATERIAL);
    } else {
        // Gem = cube + rotated cube overlay
        glutSolidCube(0.70f);
        glPushMatrix(); glRotatef(45, 0, 1, 0); glScalef(0.85f, 0.85f, 0.5f); glutSolidCube(0.60f); glPopMatrix();
        glPushMatrix(); glRotatef(45, 1, 0, 0); glScalef(0.5f, 0.85f, 0.85f); glutSolidCube(0.60f); glPopMatrix();

        // Glow halo (additive)
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        float gw = 0.38f + 0.28f * sinf(gGlobalTime * 5.5f);
        glColor4f(pr, pg, pb, gw);
        glLineWidth(1.5f); glutWireCube(1.05f); glLineWidth(1.0f);
        glColor4f(pr, pg, pb, gw * 0.45f);
        glPushMatrix(); glScalef(1.35f, 1.35f, 1.35f); glutWireSphere(0.55f, 8, 8); glPopMatrix();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
        glEnable(GL_COLOR_MATERIAL);
    }
    glPopMatrix();
}

// ──────────────────────────────────────────────────────────────
//  Bullets  (glowing elongated quads)
// ──────────────────────────────────────────────────────────────
static void drawBullets(){
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for(size_t i=0;i<gBullets.size();i++){
        if(!gBullets[i].active) continue;
        float bx = gBullets[i].pos.x;
        float by = gBullets[i].pos.y;
        float bz = gBullets[i].pos.z;
        // Bright core
        glColor4f(1.0f,1.0f,0.55f,1.0f);
        glBegin(GL_QUADS);
        glVertex3f(bx-0.05f, by-0.30f, bz);
        glVertex3f(bx+0.05f, by-0.30f, bz);
        glVertex3f(bx+0.05f, by+0.70f, bz);
        glVertex3f(bx-0.05f, by+0.70f, bz);
        glEnd();
        // Outer soft glow
        glColor4f(1.0f,0.55f,0.0f,0.38f);
        glBegin(GL_QUADS);
        glVertex3f(bx-0.12f, by-0.50f, bz);
        glVertex3f(bx+0.12f, by-0.50f, bz);
        glVertex3f(bx+0.12f, by+1.00f, bz);
        glVertex3f(bx-0.12f, by+1.00f, bz);
        glEnd();
    }
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_BLEND);
}

// ──────────────────────────────────────────────────────────────
//  Arena Grid  (pulsing neon border with glow pass)
// ──────────────────────────────────────────────────────────────
static void drawArenaGrid(){
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Subtle blue grid
    float ga = 0.07f + 0.03f*sinf(gGlobalTime*0.9f);
    glColor4f(0.10f,0.38f,0.80f, ga);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for(float gx=-ARENA_W;gx<=ARENA_W;gx+=2.0f){
        glVertex3f(gx,0,0); glVertex3f(gx,ARENA_H,0);
    }
    for(float gy=0;gy<=ARENA_H;gy+=2.0f){
        glVertex3f(-ARENA_W,gy,0); glVertex3f(ARENA_W,gy,0);
    }
    glEnd();

    // Side Borders (Interactive 3D Physical Beams)
    glEnable(GL_LIGHTING);
    for (int side = -1; side <= 1; side += 2) {
        float lx = side * ARENA_W;
        float t = gGlobalTime;
        int segs = 12;
        float sH = ARENA_H / (float)segs;

        for (int i = 0; i < segs; i++) {
            float midY = i * sH + sH * 0.5f;
            
            // Physics: Wave sway + Impact resonance
            float sway = 0.12f * sinf(t * 1.8f + midY * 0.4f);
            float vib  = gCameraShake * 0.25f * sinf(t * 65.0f);
            float curX = lx + (sway + vib) * side; // push outwards/inwards

            // 1. Hardware Bracket (Solid, metallic)
            drawBox(curX, i * sH, 0.5f, 0.6f, 0.15f, 0.4f, 0.2f, 0.3f, 0.4f);

            // 2. Main Energy Beam Segment (3D Thin Box)
            // Render as a high-gloss neon core
            drawBox(curX, midY, 0.5f, 0.12f, sH * 0.95f, 0.12f, 0.0f, 0.8f, 1.0f, 0.9f);
            
            // 3. Energy Shell (Additive semi-transparent glow)
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            float pulse = 0.6f + 0.4f * sinf(t * 5.0f + i);
            glColor4f(0.0f, 0.5f, 1.0f, 0.2f * pulse);
            glPushMatrix();
            glTranslatef(curX, midY, 0.5f);
            glScalef(0.35f, sH, 0.35f);
            glutSolidCube(1.0f);
            glPopMatrix();
            glDisable(GL_BLEND);
            glEnable(GL_LIGHTING);
        }
        
        // Final bracket at the very top
        drawBox(lx + (0.12f * sinf(t * 1.8f + ARENA_H * 0.4f)) * side, ARENA_H, 0.5f, 0.6f, 0.15f, 0.4f, 0.2f, 0.3f, 0.4f);
    }
    glDisable(GL_LIGHTING);
    glLineWidth(1.0f);
}

// ──────────────────────────────────────────────────────────────
//  2D Utility  (pixel-coordinate ortho helpers)
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

// Stroke text with glow halo (call inside 2D ortho context)
static void strokeCentered(float cx, float cy, const std::string& s, float scale,
                            float r, float g, float b, float a){
    float tw=0; for(char c:s) tw += glutStrokeWidth(GLUT_STROKE_ROMAN,c);
    float startX = cx - tw*scale*0.5f;

    glEnable(GL_BLEND);
    // Glow pass
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glColor4f(r,g,b, a*0.30f);
    glLineWidth(6.0f);
    glPushMatrix();
    glTranslatef(startX,cy,0); glScalef(scale,scale,scale);
    for(char c:s) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();
    // Crisp pass
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1.0f,1.0f,1.0f, a);
    glLineWidth(2.0f);
    glPushMatrix();
    glTranslatef(startX,cy,0); glScalef(scale,scale,scale);
    for(char c:s) glutStrokeCharacter(GLUT_STROKE_ROMAN,c);
    glPopMatrix();
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
}

// ──────────────────────────────────────────────────────────────
//  HUD  (sleek, full-horizontal top console)
// ──────────────────────────────────────────────────────────────
static void drawHUD(){
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);

    // ── Top panel background ──
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Dark top with a slight fade
    glBegin(GL_QUADS);
    glColor4f(0.0f, 0.05f, 0.15f, 0.85f);
    glVertex2f(0, WIN_H-60); glVertex2f(WIN_W, WIN_H-60);
    glVertex2f(WIN_W, WIN_H); glVertex2f(0, WIN_H);
    glEnd();
    
    // Glowing accent line
    glColor4f(0.0f, 0.80f, 1.0f, 0.7f);
    glLineWidth(2.5f);
    glBegin(GL_LINES); glVertex2f(0,WIN_H-60); glVertex2f(WIN_W,WIN_H-60); glEnd();
    glLineWidth(1.0f);
    glDisable(GL_BLEND);

    char buf[128];
    // Score (Left)
    glColor3f(0.0f,1.0f,1.0f);
    snprintf(buf,sizeof(buf),"SCORE: %05d",gScore);
    bitmapAt(30.0f, WIN_H-35.0f, buf, GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(0.45f,0.65f,0.85f);
    snprintf(buf,sizeof(buf),"BEST: %05d",gHighScore);
    bitmapAt(220.0f, WIN_H-32.0f, buf, GLUT_BITMAP_HELVETICA_18);

    // Difficulty (Center)
    glColor3f(1.0f,0.75f,0.10f);
    snprintf(buf,sizeof(buf),"LEVEL %.0f",gDifficulty);
    bitmapCentered(WIN_W*0.5f, WIN_H-38.0f, buf, GLUT_BITMAP_TIMES_ROMAN_24);

    // Lives display (Right)
    char hpStr[64]="LIVES: ";
    // Use dynamic hearts icons based on HP
    for(int i=0;i<5;i++) {
        if (i < gPlayerHP) strcat(hpStr, "[*] ");
        else if (i < 5) strcat(hpStr, "[ ] ");
    }
    glColor3f(0.18f,1.0f,0.50f);
    bitmapAt(WIN_W - bitmapWidth(hpStr,GLUT_BITMAP_TIMES_ROMAN_24) - 30.0f,
             WIN_H-35.0f, hpStr, GLUT_BITMAP_TIMES_ROMAN_24);

    // ── Active power-up strip (bottom-left) ──
    float py = 30.0f;
    if(gTripleShotTimer>0){
        snprintf(buf,sizeof(buf),"TRIPLE SHOT: %.1fs",gTripleShotTimer);
        glColor3f(1.0f,1.0f,0.0f); bitmapAt(30,py,buf,GLUT_BITMAP_HELVETICA_18); py+=24;
    }
    if(gShieldActive){
        glColor3f(0.0f,1.0f,1.0f); bitmapAt(30,py,"SHIELD: ACTIVE",GLUT_BITMAP_HELVETICA_18); py+=24;
    }
    if(gRapidFireTimer>0){
        snprintf(buf,sizeof(buf),"HYPER-DRIVE: %.1fs",gRapidFireTimer);
        glColor3f(1.0f,0.55f,0.0f); bitmapAt(30,py,buf,GLUT_BITMAP_HELVETICA_18); py+=24;
    }
    if(gTimeSlowTimer>0){
        snprintf(buf,sizeof(buf),"NEON OVERDRIVE: %.1fs",gTimeSlowTimer);
        glColor3f(1.0f,0.0f,1.0f); bitmapAt(30,py,buf,GLUT_BITMAP_HELVETICA_18);
    }

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

// ──────────────────────────────────────────────────────────────
//  Home Screen
// ──────────────────────────────────────────────────────────────
static void drawHomeScreen(){
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    
    float cx = WIN_W*0.5f;

    // Dark overlay background
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.05f, 0.85f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(WIN_W,0);
    glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H);
    glEnd();
    glDisable(GL_BLEND);

    float t = gGlobalTime;
    float pulse = 0.8f + 0.2f*sinf(t * 3.0f);

    // Title
    strokeCentered(cx, WIN_H*0.65f, "NEO SHOOTER", 0.40f, 0.0f, 0.8f, 1.0f, pulse);
    strokeCentered(cx, WIN_H*0.48f, "3D", 0.60f, 1.0f, 0.1f, 0.4f, pulse); // Magenta pinkish 3D

    // Blinking start prompt
    float blink = 0.5f + 0.5f*sinf(t * 6.0f);
    glColor3f(blink, blink, blink);
    bitmapCentered(cx, WIN_H*0.25f, "[ PRESS SPACE OR ENTER TO INITIALIZE ]", GLUT_BITMAP_TIMES_ROMAN_24);

    glColor3f(0.3f, 0.4f, 0.6f);
    bitmapCentered(cx, WIN_H*0.1f, "Developed by AL AMIN HOSSAIN", GLUT_BITMAP_HELVETICA_18);

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

// ──────────────────────────────────────────────────────────────
//  Overlay  (game over / pause / powerup notification)
// ──────────────────────────────────────────────────────────────
static void drawOverlay(){
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);

    float cx = WIN_W*0.5f;

    if(gState == STATE_GAMEOVER){
        // ── Dark animated backdrop ──
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float bgP = 0.03f + 0.02f*sinf(gGlobalTime*1.2f);
        glColor4f(bgP*0.25f,0.0f,bgP*2.0f, 0.88f);
        glBegin(GL_QUADS);
        glVertex2f(0,0);glVertex2f(WIN_W,0);glVertex2f(WIN_W,WIN_H);glVertex2f(0,WIN_H);
        glEnd();

        // Panel
        glColor4f(0.02f,0.0f,0.07f, 0.72f);
        float px1=cx-285,px2=cx+285, py1=WIN_H*0.18f, py2=WIN_H*0.84f;
        glBegin(GL_QUADS); glVertex2f(px1,py1);glVertex2f(px2,py1);glVertex2f(px2,py2);glVertex2f(px1,py2); glEnd();

        // Panel border (pulsing red)
        float la = 0.55f+0.45f*sinf(gGlobalTime*3.0f);
        glColor4f(1.0f,0.12f,0.0f,la);
        glLineWidth(2.0f);
        glBegin(GL_LINE_LOOP); glVertex2f(px1,py1);glVertex2f(px2,py1);glVertex2f(px2,py2);glVertex2f(px1,py2); glEnd();
        // Divider lines
        glBegin(GL_LINES);
        glVertex2f(px1+20,WIN_H*0.68f); glVertex2f(px2-20,WIN_H*0.68f);
        glVertex2f(px1+20,WIN_H*0.30f); glVertex2f(px2-20,WIN_H*0.30f);
        glEnd();
        glLineWidth(1.0f);
        glDisable(GL_BLEND);

        // MISSION FAILED (stroke with color pulse)
        float t = gGlobalTime;
        float titleA = 0.88f+0.12f*sinf(t*4.0f);
        strokeCentered(cx, WIN_H*0.72f, "MISSION FAILED", 0.26f, 1.0f, 0.04f*sinf(t*3.0f+1.0f), 0.04f, titleA);

        // Score / Best
        char buf[128];
        snprintf(buf,sizeof(buf),"FINAL SCORE :  %05d",gScore);
        glColor3f(1.0f,0.85f,0.0f);
        bitmapCentered(cx, WIN_H*0.55f, buf, GLUT_BITMAP_TIMES_ROMAN_24);

        snprintf(buf,sizeof(buf),"BEST SCORE  :  %05d",gHighScore);
        glColor3f(0.4f,0.80f,1.0f);
        bitmapCentered(cx, WIN_H*0.48f, buf, GLUT_BITMAP_HELVETICA_18);

        // Restart hint (blinking)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float blink = 0.5f+0.5f*sinf(t*2.5f);
        glColor4f(blink,blink,blink,1);
        bitmapCentered(cx, WIN_H*0.36f, "[ Press  R  to  Restart  System ]", GLUT_BITMAP_HELVETICA_18);

        // Developer credit bar
        glColor4f(0.04f,0.0f,0.10f,0.80f);
        glBegin(GL_QUADS);
        glVertex2f(px1,WIN_H*0.18f); glVertex2f(px2,WIN_H*0.18f);
        glVertex2f(px2,WIN_H*0.24f); glVertex2f(px1,WIN_H*0.24f);
        glEnd();
        float cr=1.0f, cg=0.3f+0.3f*sinf(t*2.0f+2.0f);
        glColor4f(cr,cg,0.0f,0.9f);
        glLineWidth(1.5f);
        glBegin(GL_LINES); glVertex2f(px1,WIN_H*0.24f); glVertex2f(px2,WIN_H*0.24f); glEnd();
        glLineWidth(1.0f);
        glDisable(GL_BLEND);
        glColor3f(cr,0.85f,0.0f);
        bitmapCentered(cx, WIN_H*0.195f, "Developed by  AL AMIN HOSSAIN", GLUT_BITMAP_HELVETICA_18);

    } else if(gPaused){
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.0f,0.0f,0.0f,0.55f);
        glBegin(GL_QUADS); glVertex2f(0,0);glVertex2f(WIN_W,0);glVertex2f(WIN_W,WIN_H);glVertex2f(0,WIN_H); glEnd();
        glDisable(GL_BLEND);

        strokeCentered(cx, WIN_H*0.53f, "PAUSED", 0.28f, 1.0f,0.92f,0.0f,1.0f);
        glColor3f(1.0f,1.0f,1.0f);
        bitmapCentered(cx, WIN_H*0.44f, "Press  P  to  Resume", GLUT_BITMAP_HELVETICA_18);
    }

    // ── Powerup notification (shown anytime outside game-over/paused) ──
    if(gState == STATE_PLAY && !gPaused && gPowerupMsgTimer>0.0f){
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float alpha = std::min(1.0f, gPowerupMsgTimer);
        float yOff  = (2.0f-gPowerupMsgTimer)*22.0f;
        glColor4f(gMsgColor[0],gMsgColor[1],gMsgColor[2],alpha);
        bitmapCentered(cx, WIN_H*0.60f+yOff, gPowerupMsg, GLUT_BITMAP_TIMES_ROMAN_24);
        glDisable(GL_BLEND);
    }

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);  glPopMatrix();
}

// ──────────────────────────────────────────────────────────────
//  Spawn helpers
// ──────────────────────────────────────────────────────────────
static void spawnEnemy(){
    Enemy e;
    e.pos    = Vec3(frand(-ARENA_W+1,ARENA_W-1), ARENA_H, 0.5f);
    e.hp     = 1 + (int)(gDifficulty*0.2f);
    e.active = true;
    gEnemies.push_back(e);
}

static void spawnPowerup(Vec3 pos){
    if(frand(0,1)>0.15f) return;
    Powerup p;
    p.pos=pos;
    float r=frand(0,5);
    if(r<1)      p.type=PT_TRIPLE;
    else if(r<2) p.type=PT_SHIELD;
    else if(r<3) p.type=PT_RAPID;
    else if(r<4) p.type=PT_SLOW;
    else         p.type=PT_HEALTH;
    p.active=true;
    gPowerups.push_back(p);
}

static void resetGame(){
    gPlayerX=0.0f;
    gBullets.clear(); gEnemies.clear(); gParticles.clear(); gPowerups.clear();

    initStars();
    initAsteroidsBg();
    loadHighScore(); 

    gFireCooldown=0; gEnemySpawnTimer=0; gDifficulty=1.0f;
    gScore=0; gPlayerHP=3; gState=STATE_PLAY;
    gCameraShake=0; gTripleShotTimer=0; gRapidFireTimer=0;
    gTimeSlowTimer=0; gShieldActive=false; gPaused=false;
#ifdef _WIN32
    mciSendString("resume bgm",NULL,0,NULL);
#endif
}

// ──────────────────────────────────────────────────────────────
//  Physics / Update  (all original logic preserved)
// ──────────────────────────────────────────────────────────────
static void update(float dt){
    gGlobalTime += dt;

    if(gPowerupMsgTimer>0) gPowerupMsgTimer-=dt;
    // Note: triple/rapid/slow timers decremented once here only
    if(gTripleShotTimer>0) gTripleShotTimer-=dt;
    if(gRapidFireTimer>0)  gRapidFireTimer-=dt;
    if(gTimeSlowTimer>0)   gTimeSlowTimer-=dt;

    float timeScale = (gTimeSlowTimer>0)?0.3f:1.0f;
    updateStars(dt, timeScale);
    updateAsteroidsBg(dt);

    if(gPaused){ return; }

    if(gState == STATE_PLAY){
        // ── Player movement (arrow keys, original) ──
        if(gKeyLeft)  gPlayerX -= 15.0f*dt;
        if(gKeyRight) gPlayerX += 15.0f*dt;
        if(gPlayerX < -ARENA_W+1) gPlayerX=-ARENA_W+1;
        if(gPlayerX >  ARENA_W-1) gPlayerX= ARENA_W-1;

        // ── Firing ──
        if(gFireCooldown>0) gFireCooldown-=dt;
        if(gKeySpace && gFireCooldown<=0){
            float fireRate = (gRapidFireTimer>0)?0.07f:0.15f;
            if(gTripleShotTimer>0){
                for(int ang=-1;ang<=1;ang++){
                    Bullet b; b.pos=Vec3(gPlayerX+ang*0.5f,gPlayerY+1.0f,0.5f); b.active=true;
                    gBullets.push_back(b);
                }
            } else {
                Bullet b; b.pos=Vec3(gPlayerX,gPlayerY+1.0f,0.5f); b.active=true;
                gBullets.push_back(b);
            }
            gFireCooldown=fireRate;
#ifdef _WIN32
            PlaySound(TEXT("shooter music/fire_new.mp3"),NULL,SND_FILENAME|SND_ASYNC);
#endif
        }

        // Engine trail
        if(fmodf(gGlobalTime,0.05f)<0.02f)
            spawnParticles(Vec3(gPlayerX,gPlayerY-0.5f,0.5f),0.0f,0.5f,1.0f,2);

        // ── Update Bullets ──
        for(size_t i=0;i<gBullets.size();i++){
            if(!gBullets[i].active) continue;
            gBullets[i].pos.y += 30.0f*dt;
            if(gBullets[i].pos.y > ARENA_H+2) gBullets[i].active=false;
        }

        // ── Update Powerups ──
        for(size_t i=0;i<gPowerups.size();i++){
            if(!gPowerups[i].active) continue;
            gPowerups[i].pos.y -= 5.0f*dt;
            float dx=fabsf(gPowerups[i].pos.x-gPlayerX);
            float dy=fabsf(gPowerups[i].pos.y-gPlayerY);
            if(dx<1.5f && dy<1.5f){
                gPowerups[i].active=false;
                if(gPowerups[i].type==PT_TRIPLE){
                    gTripleShotTimer=5.0f; strcpy(gPowerupMsg,"TRIPLE SHOT ACTIVATED!");
                    gMsgColor[0]=1;gMsgColor[1]=1;gMsgColor[2]=0;
                } else if(gPowerups[i].type==PT_SHIELD){
                    gShieldActive=true; strcpy(gPowerupMsg,"SHIELD REINFORCED!");
                    gMsgColor[0]=0;gMsgColor[1]=1;gMsgColor[2]=1;
                } else if(gPowerups[i].type==PT_RAPID){
                    gRapidFireTimer=5.0f; strcpy(gPowerupMsg,"HYPER-DRIVE ENGAGED!");
                    gMsgColor[0]=1;gMsgColor[1]=0.5f;gMsgColor[2]=0;
                } else if(gPowerups[i].type==PT_SLOW){
                    gTimeSlowTimer=5.0f; strcpy(gPowerupMsg,"NEON OVERDRIVE ACTIVE!");
                    gMsgColor[0]=1;gMsgColor[1]=0;gMsgColor[2]=1;
                } else {
                    if(gPlayerHP<5) gPlayerHP++; 
                    strcpy(gPowerupMsg,"EXTRA LIFE INITIALIZED!");
                    gMsgColor[0]=1.0f;gMsgColor[1]=0.2f;gMsgColor[2]=0.2f;
                }
                gPowerupMsgTimer=2.0f;
                gScore+=50;
                spawnParticles(gPowerups[i].pos,1,1,1,15);
#ifdef _WIN32
                PlaySound(TEXT("shooter music/pickup_new.mp3"),NULL,SND_FILENAME|SND_ASYNC);
#endif
            }
            if(gPowerups[i].pos.y<-2) gPowerups[i].active=false;
        }

        // ── Difficulty ──
        gDifficulty += dt*0.05f;

        // ── Spawn Enemies ──
        gEnemySpawnTimer -= dt;
        if(gEnemySpawnTimer<=0){
            spawnEnemy();
            gEnemySpawnTimer=frand(0.5f/gDifficulty, 1.5f/gDifficulty);
        }

        // ── Update Enemies ──
        for(size_t i=0;i<gEnemies.size();i++){
            if(!gEnemies[i].active) continue;
            float spd = (4.0f+gDifficulty*0.5f)*dt;
            if(gTimeSlowTimer>0) spd*=0.4f;
            gEnemies[i].pos.y -= spd;

            float dx=fabsf(gEnemies[i].pos.x-gPlayerX);
            float dy=fabsf(gEnemies[i].pos.y-gPlayerY);
            if(dx<1.5f && dy<1.5f){
                gEnemies[i].active=false;
                spawnParticles(gEnemies[i].pos,1.0f,0.5f,0.0f,20);
                if(gShieldActive){
                    gShieldActive=false; gCameraShake=0.5f;
#ifdef _WIN32
                    PlaySound(TEXT("shooter music/crash_new.mp3"),NULL,SND_FILENAME|SND_ASYNC);
#endif
                    if(gPlayerHP<=0){
                        gState=STATE_GAMEOVER; gCameraShake=2.0f;
                        spawnParticles(Vec3(gPlayerX,gPlayerY,0.5f),1.0f,0.2f,0.0f,40);
                    }
                }
                if(gScore>gHighScore) { gHighScore=gScore; saveHighScore(); }
            }

            if(gEnemies[i].pos.y<0.0f){
                gEnemies[i].active=false;
                gPlayerHP--; gCameraShake=0.5f;
                if(gPlayerHP<=0){ 
                    gState=STATE_GAMEOVER; 
                }
                if(gScore>gHighScore) { gHighScore=gScore; saveHighScore(); }
            }
        }

        // ── Bullet vs Enemy collisions ──
        for(size_t i=0;i<gBullets.size();i++){
            if(!gBullets[i].active) continue;
            for(size_t j=0;j<gEnemies.size();j++){
                if(!gEnemies[j].active) continue;
                float dx=fabsf(gBullets[i].pos.x-gEnemies[j].pos.x);
                float dy=fabsf(gBullets[i].pos.y-gEnemies[j].pos.y);
                if(dx<1.2f && dy<1.2f){
                    gBullets[i].active=false;
                    gEnemies[j].hp--;
                    spawnParticles(gBullets[i].pos,1.0f,1.0f,0.2f,5);
                    if(gEnemies[j].hp<=0){
                        gEnemies[j].active=false;
                        gScore+=10; gCameraShake+=0.15f;
                        spawnParticles(gEnemies[j].pos, 0.2f, 1.0f, 0.2f, 20);
                        spawnPowerup(gEnemies[j].pos);
#ifdef _WIN32
                        PlaySound(TEXT("shooter music/crash_new.mp3"),NULL,SND_FILENAME|SND_ASYNC);
#endif
                    }
                    break;
                }
            }
        }

        // Clean up
        gBullets.erase(std::remove_if(gBullets.begin(),gBullets.end(),[](const Bullet& b){return !b.active;}),gBullets.end());
        gEnemies.erase(std::remove_if(gEnemies.begin(),gEnemies.end(),[](const Enemy& e){return !e.active;}),gEnemies.end());
        gPowerups.erase(std::remove_if(gPowerups.begin(),gPowerups.end(),[](const Powerup& p){return !p.active;}),gPowerups.end());
    }

    updateParticles(dt);
    gCameraShake *= 0.9f;
}

// ──────────────────────────────────────────────────────────────
//  Idle  (smooth delta-time driven loop)
// ──────────────────────────────────────────────────────────────
static void idle(){
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = (now-gLastMs)*0.001f;
    if(dt>0.05f) dt=0.05f;
    gLastMs=now;
    update(dt);
    glutPostRedisplay();
}

// ──────────────────────────────────────────────────────────────
//  Display
// ──────────────────────────────────────────────────────────────
static void display(){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    // ── 3D scene setup ──
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(60.0,(double)WIN_W/WIN_H,0.1,100.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    float shakeX=gCameraShake*frand(-0.4f,0.4f);
    float shakeY=gCameraShake*frand(-0.4f,0.4f);
    gluLookAt(shakeX, -5.0f+shakeY, 20.0f,
              0.0f,   ARENA_H/2.0f,  0.0f,
              0.0f,   1.0f,          0.0f);

    setupLighting();

    // ── Background ──
    drawStars();
    drawArenaGrid();
    drawAsteroidsBg();

    // ── Game objects ──
    if(gState != STATE_HOME){
        if(gState == STATE_PLAY)
            drawPlayerShip(gPlayerX, gPlayerY);

        for(size_t i=0;i<gEnemies.size();i++)
            if(gEnemies[i].active)
                drawEnemyShip(gEnemies[i].pos.x, gEnemies[i].pos.y, gEnemies[i].hp);

        for(size_t i=0;i<gPowerups.size();i++)
            if(gPowerups[i].active)
                drawPowerup(gPowerups[i]);

        drawBullets();
        drawParticles();
    }

    // ── 2D overlays ──
    if(gState == STATE_HOME){
        drawHomeScreen();
    } else {
        drawHUD();
        drawOverlay();
    }

    glutSwapBuffers();
}

static void reshape(int w, int h){
    WIN_W=w; WIN_H=h?h:1;
    glViewport(0,0,WIN_W,WIN_H);
}

// ──────────────────────────────────────────────────────────────
//  Input  (identical to original)
// ──────────────────────────────────────────────────────────────
static void specialKeyDown(int key,int,int){
    if(key==GLUT_KEY_LEFT)  gKeyLeft=true;
    if(key==GLUT_KEY_RIGHT) gKeyRight=true;
}
static void specialKeyUp(int key,int,int){
    if(key==GLUT_KEY_LEFT)  gKeyLeft=false;
    if(key==GLUT_KEY_RIGHT) gKeyRight=false;
}
static void normalKeyDown(unsigned char key,int,int){
    if(key==27) exit(0);
    
    if(gState == STATE_HOME) {
        if(key==' ' || key==13) {
            resetGame();
        }
        return;
    }

    if(gState == STATE_GAMEOVER) {
        if(key=='r'||key=='R') resetGame();
        return;
    }

    // STATE_PLAY
    if(key==' ') gKeySpace=true;
    if(key=='p'||key=='P'){
        gPaused=!gPaused;
#ifdef _WIN32
        if(gPaused) mciSendString("pause bgm",NULL,0,NULL);
        else        mciSendString("resume bgm",NULL,0,NULL);
#endif
    }
}
static void normalKeyUp(unsigned char key,int,int){
    if(key==' ') gKeySpace=false;
}

// ──────────────────────────────────────────────────────────────
//  Main
// ──────────────────────────────────────────────────────────────
int main(int argc, char** argv){
    srand((unsigned)time(nullptr));

    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH|GLUT_MULTISAMPLE);
    glutInitWindowSize(WIN_W,WIN_H);
    glutCreateWindow("Neo Shooter 3D @alamin");
    glutFullScreen();

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);
    glClearColor(0.00f,0.00f,0.02f,1.0f);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT,GL_AMBIENT_AND_DIFFUSE);

    resetGame();
    gState = STATE_HOME;

#ifdef _WIN32
    mciSendString("open \"shooter music/bgm_new.mp3\" type mpegvideo alias bgm",NULL,0,NULL);
    mciSendString("setaudio bgm volume to 500",NULL,0,NULL);
    mciSendString("play bgm repeat",NULL,0,NULL);
#endif

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(normalKeyDown);
    glutKeyboardUpFunc(normalKeyUp);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);
    glutIdleFunc(idle);

    gLastMs=glutGet(GLUT_ELAPSED_TIME);
    glutMainLoop();
    return 0;
}
