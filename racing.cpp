/*
 * ================================================================
 *  NEO RACING 3D  –  OpenGL / FreeGLUT (Pro Edition)
 *  ================================================================
 *
 *  FEATURES
 *  ─────────────────────────────────────────────────────────────
 *  • 3D collision with glowing neon retro-wave style
 *  • Smooth steering interpolation & car banking (roll)
 *  • Audio System (BGM & SFX using Windows MM)
 *  • 4 Power-Up Types (Shield, Ghost Mode, Time Slow, Bonus Score)
 *  • Advanced particle explosions and camera shakes
 *  • Score and speed progression
 *
 *  CONTROLS
 *  ─────────────────────────────────────────────────────────────
 *  Arrow Left/Right   Steer / Change lanes
 *  R                  Restart game
 *  ESC                Quit
 *
 *  BUILD
 *  ─────────────────────────────────────────────────────────────
 *  Linux / macOS:
 *    g++ racing.cpp -o racing -lGL -lGLU -lglut -lm && ./racing
 *
 *  Windows (MinGW / Code::Blocks):
 *    g++ racing.cpp -o racing.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
 *
 *  AUDIO
 *  ─────────────────────────────────────────────────────────────
 *  Place the following files in the same directory to enable sound:
 *  • bgm.mp3      (Background music)
 *  • crash.wav    (Crash impact sound effect)
 *  • pickup.wav   (Power-up collection sound effect)
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

// ──────────────────────────────────────────────────────────────
//  Audio System
// ──────────────────────────────────────────────────────────────
static void playBGM() {
#ifdef _WIN32
    // mciSendString handles MP3 decoding natively on Windows
    mciSendStringA("open audio/bgm.mp3 type mpegvideo alias bgm", NULL, 0, NULL);
    mciSendStringA("play bgm repeat", NULL, 0, NULL);
#endif
}

static void stopBGM() {
#ifdef _WIN32
    mciSendStringA("stop bgm", NULL, 0, NULL);
    mciSendStringA("close bgm", NULL, 0, NULL);
#endif
}

static void playSFX(const char* file) {
#ifdef _WIN32
    // SND_NODEFAULT = Silently fail if file is missing (no Windows error ding)
    // SND_ASYNC = Play in background without freezing game
    PlaySoundA(file, NULL, SND_ASYNC | SND_FILENAME | SND_NODEFAULT);
#endif
}

// ──────────────────────────────────────────────────────────────
//  Window dimensions
// ──────────────────────────────────────────────────────────────
static int WIN_W = 800;
static int WIN_H = 800;

// ──────────────────────────────────────────────────────────────
//  Game Constants
// ──────────────────────────────────────────────────────────────
static const int NUM_LANES = 3;
static const float LANE_WIDTH = 2.5f;

static const float ROAD_LENGTH = 50.0f;
static const float ROAD_Z_START = 10.0f;
static const float ROAD_Z_END = -ROAD_LENGTH;

struct Vec3 {
    float x, y, z;
    Vec3(float x=0, float y=0, float z=0): x(x), y(y), z(z) {}
};

enum EntityType {
    ENT_ENEMY_CAR,
    ENT_POWERUP
};

enum PowerUpType {
    PU_SHIELD,      // Ice Blue - Absorbs 1 crash
    PU_GHOST,       // Purple - Pass through cars
    PU_SLOW_MO,     // Yellow - Halves speed temporarily
    PU_SCORE_BONUS, // Green - +100 Points flat
    PU_COUNT
};

struct Entity {
    EntityType type;
    int lane;
    float z;
    PowerUpType puType;
    float spin;
};

struct Particle {
    Vec3 pos, vel;
    float life;
    float r, g, b;
};

// ──────────────────────────────────────────────────────────────
//  Game State
// ──────────────────────────────────────────────────────────────
static int gTargetLane = 1;       // 0 (Left), 1 (Mid), 2 (Right)
static float gPlayerX = 0.0f;     // Actual smoothly interpolated visual X position
static float gCarRoll = 0.0f;     // Bank angle (tilt) when steering
static float gPlayerZ = 0.0f;

static std::vector<Entity> gEntities;
static std::vector<Particle> gParticles;

static float gBaseSpeed = 20.0f;
static float gCurrentSpeed = 20.0f;
static float gScrollOffset = 0.0f;

static int gScore = 0;
static int gHighScore = 0;
static float gScoreAccumulator = 0.0f;
static bool gGameOver = false;

static float gGlobalTime = 0.0f;
static float gCameraShake = 0.0f;
static float gCrashFlash = 0.0f;

static float gSpawnTimer = 0.0f;

// Power-Up States
static bool gShield = false;
static float gGhostTimer = 0.0f;
static float gSlowTimer = 0.0f;
static char gPickupMessage[128] = "";
static float gPickupMessageTimer = 0.0f;

// ──────────────────────────────────────────────────────────────
//  Helpers
// ──────────────────────────────────────────────────────────────
static float frand(float lo, float hi) {
    return lo + (hi - lo) * (float)rand() / (float)RAND_MAX;
}

static float getLaneX(int lane) {
    return (lane - 1) * LANE_WIDTH;
}

static void spawnParticles(Vec3 pos, float r, float g, float b, int count) {
    for(int i = 0; i < count; i++) {
        Particle p;
        p.pos = pos;
        p.vel = Vec3(frand(-0.5f, 0.5f), frand(-0.5f, 0.5f), frand(0.2f, 1.0f));
        p.r = r; p.g = g; p.b = b;
        p.life = 1.0f;
        gParticles.push_back(p);
    }
}

static void setPickupMessage(const char* msg) {
    snprintf(gPickupMessage, sizeof(gPickupMessage), "%s", msg);
    gPickupMessageTimer = 2.0f;
}

static void applyPowerup(PowerUpType pu) {
    playSFX("audio/pickup.wav");
    switch(pu) {
        case PU_SHIELD:
            gShield = true;
            setPickupMessage("SHIELD ACTIVATED!");
            break;
        case PU_GHOST:
            gGhostTimer = 8.0f;
            setPickupMessage("GHOST MODE! (8s)");
            break;
        case PU_SLOW_MO:
            gSlowTimer = 5.0f;
            setPickupMessage("TIME SLOWED! (5s)");
            break;
        case PU_SCORE_BONUS:
            gScore += 100;
            if (gScore > gHighScore) gHighScore = gScore;
            setPickupMessage("+100 POINT BONUS!");
            break;
        default: break;
    }
}

static void spawnEntity() {
    Entity ent;
    ent.lane = rand() % NUM_LANES;
    ent.z = ROAD_Z_END - 5.0f;
    ent.spin = 0.0f;

    if ((rand() % 100) < 15) {
        ent.type = ENT_POWERUP;
        ent.puType = (PowerUpType)(rand() % PU_COUNT);
    } else {
        ent.type = ENT_ENEMY_CAR;
    }
    gEntities.push_back(ent);
}

static void resetGame() {
    gTargetLane = 1;
    gPlayerX = getLaneX(gTargetLane);
    gCarRoll = 0.0f;

    gEntities.clear();
    gParticles.clear();

    gBaseSpeed = 15.0f;
    gCurrentSpeed = 15.0f;
    gScore = 0;
    gScoreAccumulator = 0.0f;
    gGameOver = false;

    gSpawnTimer = 0.0f;
    gCameraShake = 0.0f;
    gCrashFlash = 0.0f;

    gShield = false;
    gGhostTimer = 0.0f;
    gSlowTimer = 0.0f;
    gPickupMessageTimer = 0.0f;

    stopBGM();
    playBGM();
}

// ──────────────────────────────────────────────────────────────
//  Physics / Update
// ──────────────────────────────────────────────────────────────
static void updatePhysics(int) {
    gGlobalTime += 1.0f / 60.0f;
    float dt = 1.0f / 60.0f;

    if (!gGameOver) {
        gBaseSpeed += dt * 0.15f;

        // ── Smooth Steering & Banking ──
        float targetX = getLaneX(gTargetLane);
        float diffX = targetX - gPlayerX;
        gPlayerX += diffX * 12.0f * dt; // Lerp position

        // Tilt the car based on how far it needs to move (max 30 degrees)
        float targetRoll = fmaxf(-30.0f, fminf(30.0f, -diffX * 20.0f));
        gCarRoll += (targetRoll - gCarRoll) * 15.0f * dt; // Lerp bank angle

        // ── Power-Up Timers ──
        if (gGhostTimer > 0.0f) gGhostTimer -= dt;
        if (gSlowTimer > 0.0f) {
            gSlowTimer -= dt;
            gCurrentSpeed = gBaseSpeed * 0.5f;
        } else {
            gCurrentSpeed = gBaseSpeed;
        }

        if (gPickupMessageTimer > 0.0f) gPickupMessageTimer -= dt;

        gScrollOffset += gCurrentSpeed * dt;
        if (gScrollOffset > 4.0f) gScrollOffset -= 4.0f;

        gScoreAccumulator += gCurrentSpeed * dt * 0.1f;
        if (gScoreAccumulator >= 1.0f) {
            gScore += 1;
            gScoreAccumulator -= 1.0f;
            if (gScore > gHighScore) gHighScore = gScore;
        }

        // ── Entities Update & Collision ──
        for (size_t i = 0; i < gEntities.size(); i++) {
            gEntities[i].z += gCurrentSpeed * dt;
            gEntities[i].spin += 150.0f * dt;

            float ex = getLaneX(gEntities[i].lane);
            float distX = fabsf(ex - gPlayerX);

            // Check bounding box overlaps (Z overlap AND X overlap)
            if (gEntities[i].z > gPlayerZ - 1.2f && gEntities[i].z < gPlayerZ + 1.2f && distX < 1.4f) {
                if (gEntities[i].type == ENT_ENEMY_CAR) {
                    if (gGhostTimer > 0.0f) {
                        // Pass through (Ghost Mode)
                    } else if (gShield) {
                        // Shield Break
                        gShield = false;
                        playSFX("audio/crash.wav");
                        setPickupMessage("SHIELD BROKEN!");
                        gCameraShake = 0.6f;
                        gEntities[i].z = ROAD_Z_START + 10.0f;
                        spawnParticles(Vec3(gEntities[i].lane, 0.5f, gPlayerZ), 0.2f, 0.8f, 1.0f, 25);
                    } else {
                        // Crash !
                        gGameOver = true;
                        playSFX("audio/crash.wav");
                        stopBGM();
                        gCameraShake = 1.5f;
                        gCrashFlash = 1.0f;
                        spawnParticles(Vec3(ex, 0.5f, gPlayerZ), 1.0f, 0.2f, 0.0f, 40);
                    }
                } else if (gEntities[i].type == ENT_POWERUP) {
                    applyPowerup(gEntities[i].puType);
                    gEntities[i].z = ROAD_Z_START + 10.0f; // Consume
                    spawnParticles(Vec3(ex, 0.5f, gPlayerZ), 1.0f, 1.0f, 0.0f, 20);
                }
            }
        }

        // Remove entities behind camera
        gEntities.erase(
            std::remove_if(gEntities.begin(), gEntities.end(), [](const Entity& e){ return e.z > ROAD_Z_START + 5.0f; }),
            gEntities.end());

        // Spawn new entities
        gSpawnTimer -= dt;
        if (gSpawnTimer <= 0.0f) {
            spawnEntity();
            gSpawnTimer = frand(8.0f / gBaseSpeed, 18.0f / gBaseSpeed);
        }
    } else {
        // Upon death, car rights itself slowly
        gCarRoll += (0.0f - gCarRoll) * 5.0f * dt;
    }

    // ── Particle Update ──
    for (size_t i = 0; i < gParticles.size(); i++) {
        gParticles[i].pos.x += gParticles[i].vel.x;
        gParticles[i].pos.y += gParticles[i].vel.y;
        gParticles[i].pos.z += gParticles[i].vel.z;
        gParticles[i].vel.y -= dt * 1.5f; // Gravity

        if (gParticles[i].pos.y <= 0.0f) {
            gParticles[i].pos.y = 0.0f;
            gParticles[i].vel.y *= -0.5f; // Bounce
        }
        gParticles[i].life -= dt * 0.8f;
    }
    gParticles.erase(
        std::remove_if(gParticles.begin(), gParticles.end(), [](const Particle& p){ return p.life <= 0.0f; }),
        gParticles.end());

    gCameraShake *= 0.8f;
    if (gCrashFlash > 0.0f) gCrashFlash -= dt * 2.0f;

    glutPostRedisplay();
    glutTimerFunc(16, updatePhysics, 0);
}

// ──────────────────────────────────────────────────────────────
//  Drawing Helpers
// ──────────────────────────────────────────────────────────────
static void drawAestheticCar(float x, float y, float z, float roll, float r, float g, float b, float alpha, bool isEnemy) {
    glPushMatrix();
    glTranslatef(x, y + 0.3f, z);
    glRotatef(roll, 0.0f, 0.0f, 1.0f);

    // Scale the entire car to fit the lane naturally
    glScalef(1.2f, 1.1f, 1.2f);

    if (alpha < 1.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    GLfloat diff[] = { r*0.8f, g*0.8f, b*0.8f, alpha };
    GLfloat ambi[] = { r*0.3f, g*0.3f, b*0.3f, alpha };
    GLfloat spec[] = { 1.0f, 1.0f, 1.0f, alpha };

    // ----------- 1. MAIN BODY (Sleek aerodynamic curves) -----------
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diff);
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambi);
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    glMaterialf(GL_FRONT, GL_SHININESS, 100.0f);

    // Lower Base (Side skirts and underbelly)
    glPushMatrix();
    glTranslatef(0.0f, -0.1f, 0.0f);
    glScalef(0.9f, 0.2f, 2.5f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Front Hood (Curved wedge down)
    // A squashed sphere stretching to the front
    glPushMatrix();
    glTranslatef(0.0f, 0.05f, -0.55f);
    glScalef(0.9f, 0.25f, 1.5f);
    glutSolidSphere(0.5f, 16, 16);
    glPopMatrix();

    // Rear Trunk
    glPushMatrix();
    glTranslatef(0.0f, 0.1f, 0.8f);
    glScalef(0.85f, 0.3f, 1.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // ----------- 2. COCKPIT (Dark Glass Dome) -----------
    GLfloat diffG[] = { 0.05f, 0.05f, 0.1f, alpha*0.9f };
    GLfloat specG[] = { 1.0f, 1.0f, 1.0f, alpha };
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diffG);
    glMaterialfv(GL_FRONT, GL_SPECULAR, specG);
    glMaterialf(GL_FRONT, GL_SHININESS, 128.0f);

    glPushMatrix();
    glTranslatef(0.0f, 0.35f, 0.2f);
    glScalef(0.7f, 0.3f, 1.1f);
    // Smooth teardrop canopy
    glutSolidSphere(0.6f, 16, 16);
    glPopMatrix();

    // ----------- 3. SPOILER (Racing Wing) -----------
    glMaterialfv(GL_FRONT, GL_DIFFUSE, diff);
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);

    glPushMatrix();
    glTranslatef(0.0f, 0.4f, 1.25f);
    // Struts
    glPushMatrix(); glTranslatef( -0.3f, -0.1f, 0.0f); glScalef(0.05f, 0.3f, 0.2f); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(  0.3f, -0.1f, 0.0f); glScalef(0.05f, 0.3f, 0.2f); glutSolidCube(1.0f); glPopMatrix();
    // Wing
    glScalef(1.2f, 0.05f, 0.35f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // ----------- 4. ACTUAL SPINNING WHEELS -----------
    GLfloat diffW[] = { 0.05f, 0.05f, 0.05f, alpha }; // Rubber
    GLfloat rimColor[] = { 0.5f, 0.5f, 0.5f, alpha }; // Metallic rims

    float wx = 0.5f, wy = -0.15f, wz = 0.8f;
    float wheelSpin = gGlobalTime * gCurrentSpeed * 40.0f; // Rapid rotation based on vehicle velocity

    for(int i=0; i<4; i++) {
        glPushMatrix();
        float px = (i%2==0) ? wx : -wx;
        float pz = (i<2) ? wz : -wz - 0.2f;
        glTranslatef(px, wy, pz);

        // Spin the wheels around X-axis
        glRotatef(wheelSpin, 1.0f, 0.0f, 0.0f);

        // Face the cylinder outward to the side
        if (i%2==0) glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
        else glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);

        // Rubber Tire
        glMaterialfv(GL_FRONT, GL_DIFFUSE, diffW);
        glMaterialfv(GL_FRONT, GL_SPECULAR, diffW); // dull
        glMaterialf(GL_FRONT, GL_SHININESS, 5.0f);
        glutSolidTorus(0.08, 0.24, 12, 16);

        // Shiny Rim internals
        glMaterialfv(GL_FRONT, GL_DIFFUSE, rimColor);
        glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
        glMaterialf(GL_FRONT, GL_SHININESS, 80.0f);

        // 5 Custom Spokes
        for(int s=0; s<5; s++) {
            glPushMatrix();
            glRotatef(s * (360.0f/5.0f), 0.0f, 0.0f, 1.0f);
            glScalef(0.45f, 0.05f, 0.05f);
            glutSolidCube(1.0f);
            glPopMatrix();
        }

        glPopMatrix();
    }

    // ----------- 5. EMISSIVE LIGHTS & GLOW -----------
    glDisable(GL_LIGHTING);

    // Custom neon Under-Glow
    glColor4f(r, g, b, alpha * 0.4f);
    glPushMatrix();
    glTranslatef(0.0f, -0.2f, 0.0f);
    glScalef(1.0f, 0.01f, 2.3f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Taillights
    glColor4f(1.0f, 0.0f, 0.0f, alpha);
    glPushMatrix(); glTranslatef( 0.35f, 0.15f, 1.31f); glScalef(0.4f, 0.1f, 0.05f); glutSolidCube(1.0f); glPopMatrix();
    glPushMatrix(); glTranslatef(-0.35f, 0.15f, 1.31f); glScalef(0.4f, 0.1f, 0.05f); glutSolidCube(1.0f); glPopMatrix();

    // Front Headlights
    if (isEnemy) {
        // Red, aggressive, slim lights
        glColor4f(1.0f, 0.0f, 0.0f, alpha);
        glPushMatrix(); glTranslatef( 0.3f, 0.1f, -1.25f); glRotatef(15.0f, 0, 1, 0); glScalef(0.25f, 0.05f, 0.05f); glutSolidCube(1.0f); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.3f, 0.1f, -1.25f); glRotatef(-15.0f, 0, 1, 0); glScalef(0.25f, 0.05f, 0.05f); glutSolidCube(1.0f); glPopMatrix();
    } else {
        // Bright cyan/white
        glColor4f(0.8f, 1.0f, 1.0f, alpha);
        glPushMatrix(); glTranslatef( 0.3f, 0.1f, -1.25f); glRotatef(10.0f, 0, 1, 0); glScalef(0.35f, 0.1f, 0.05f); glutSolidCube(1.0f); glPopMatrix();
        glPushMatrix(); glTranslatef(-0.3f, 0.1f, -1.25f); glRotatef(-10.0f, 0, 1, 0); glScalef(0.35f, 0.1f, 0.05f); glutSolidCube(1.0f); glPopMatrix();
    }

    glEnable(GL_LIGHTING);
    if (alpha < 1.0f) glDisable(GL_BLEND);
    glPopMatrix();
}

static void drawText2D(float x, float y, const char* s, float r, float g, float b, void* font = GLUT_BITMAP_HELVETICA_18) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (const char* p = s; *p; p++) glutBitmapCharacter(font, *p);
}

// ──────────────────────────────────────────────────────────────
//  Display
// ──────────────────────────────────────────────────────────────
static void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // Reduced FOV from 60 to 45 to drastically diminish wide-angle stretching (rectilinear distortion) at screen edges
    gluPerspective(45.0, (double)WIN_W / WIN_H, 0.1, 150.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float shakeX = gCameraShake * frand(-0.5f, 0.5f);
    float shakeY = gCameraShake * frand(-0.5f, 0.5f);
    float shakeZ = gCameraShake * frand(-0.5f, 0.5f);

    // Camera now firmly tracks the player's X position to keep the car centered and entirely prevent perspective shear!
    gluLookAt(gPlayerX * 0.8f + shakeX, 4.5f + shakeY, 9.0f + shakeZ,
              gPlayerX * 0.9f, 0.0f, gPlayerZ - 10.0f,
              0.0f, 1.0f, 0.0f);

    GLfloat lpos[] = { 0.0f, 10.0f, 5.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE); // Fixes shape deformation artifacts caused by unnormalized scaled normals

    // ── Draw the Road Grid ──
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    float roadW = NUM_LANES * LANE_WIDTH + 2.0f;
    float hw = roadW / 2.0f;

    glEnable(GL_FOG);
    GLfloat fogColor[] = {0.01f, 0.02f, 0.05f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 10.0f);
    glFogf(GL_FOG_END, 45.0f);

    glDisable(GL_CULL_FACE);
    glColor4f(0.05f, 0.0f, 0.1f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(-hw, -0.01f, ROAD_Z_START);
    glVertex3f( hw, -0.01f, ROAD_Z_START);
    glVertex3f( hw, -0.01f, ROAD_Z_END);
    glVertex3f(-hw, -0.01f, ROAD_Z_END);
    glEnd();

    glLineWidth(2.0f);
    float gcR = (gGhostTimer > 0.0f) ? 0.8f : 1.0f;
    float gcG = (gGhostTimer > 0.0f) ? 0.0f : 0.0f;
    float gcB = (gGhostTimer > 0.0f) ? 1.0f : 0.8f;

    glColor4f(gcR, gcG, gcB, 0.5f);
    glBegin(GL_LINES);
    for (float z = ROAD_Z_START; z > ROAD_Z_END; z -= 4.0f) {
        float offsetZ = z + gScrollOffset;
        if (offsetZ > ROAD_Z_START) offsetZ -= (ROAD_Z_START - ROAD_Z_END);
        glVertex3f(-hw, 0.0f, offsetZ);
        glVertex3f( hw, 0.0f, offsetZ);
    }
    glEnd();

    glColor4f(0.0f, 0.8f, 1.0f, 0.5f);
    glBegin(GL_LINES);
    for (int i = 0; i <= NUM_LANES; i++) {
        float lx = getLaneX(i) - LANE_WIDTH/2.0f;
        glVertex3f(lx, 0.0f, ROAD_Z_START);
        glVertex3f(lx, 0.0f, ROAD_Z_END);
    }
    glEnd();

    glEnable(GL_LIGHTING);

    // ── Draw Player Car ──
    if (!gGameOver || gCrashFlash < 0.5f) { // Hide car slightly during intense crash flash
        float alpha = (gGhostTimer > 0.0f) ? 0.4f : 1.0f;
        float pcr = (gShield) ? 0.0f : 0.0f;
        float pcg = (gShield) ? 0.8f : 1.0f;
        float pcb = (gShield) ? 1.0f : 1.0f;

        drawAestheticCar(gPlayerX, 0.4f, gPlayerZ, gCarRoll, pcr, pcg, pcb, alpha, false);

        // Draw Shield Ring
        if (gShield) {
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glColor4f(0.0f, 0.8f, 1.0f, 0.6f + 0.3f * sin(gGlobalTime * 10.0f));
            glLineWidth(3.0f);
            glPushMatrix();
            glTranslatef(gPlayerX, 0.4f, gPlayerZ);
            glRotatef(gCarRoll, 0.0f, 0.0f, 1.0f);
            glRotatef(90, 1.0f, 0.0f, 0.0f);
            glutWireTorus(0.1, 1.8, 10, 20);
            glPopMatrix();
            glEnable(GL_LIGHTING);
        }

        // Engine exhaust glow
        if (!gGameOver) {
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            float exhaustFlicker = 0.8f + 0.2f * sin(gGlobalTime * 30.0f);
            glColor4f(0.0f, 0.5f, 1.0f, 0.6f * exhaustFlicker);
            glPushMatrix();
            glTranslatef(gPlayerX, 0.2f, gPlayerZ + 1.25f);
            glutSolidSphere(0.4f, 8, 8);
            glPopMatrix();
            glEnable(GL_LIGHTING);
        }
    }

    // ── Draw Entities ──
    for (size_t i = 0; i < gEntities.size(); i++) {
        float ex = getLaneX(gEntities[i].lane);
        float ez = gEntities[i].z;

        if (gEntities[i].type == ENT_ENEMY_CAR) {
            drawAestheticCar(ex, 0.5f, ez, 0.0f, 1.0f, 0.1f, 0.3f, 1.0f, true);
        } else if (gEntities[i].type == ENT_POWERUP) {
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glPushMatrix();
            glTranslatef(ex, 0.6f + 0.2f * sin(gGlobalTime * 5.0f + ez), ez);
            glRotatef(gEntities[i].spin, 0.0f, 1.0f, 0.0f);
            glScalef(0.6f, 0.6f, 0.6f);

            float r, g, b;
            if (gEntities[i].puType == PU_SHIELD) { r=0; g=0.8; b=1; }
            else if (gEntities[i].puType == PU_GHOST) { r=0.8; g=0; b=1; }
            else if (gEntities[i].puType == PU_SLOW_MO) { r=1; g=1; b=0; }
            else { r=0; g=1; b=0; }

            glColor4f(r, g, b, 0.8f);
            glutSolidOctahedron();
            glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
            glutWireOctahedron();
            glPopMatrix();
            glEnable(GL_LIGHTING);
        }
    }

    // ── Draw Particles ──
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glPointSize(6.0f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < gParticles.size(); i++) {
        glColor4f(gParticles[i].r, gParticles[i].g, gParticles[i].b, gParticles[i].life);
        glVertex3f(gParticles[i].pos.x, gParticles[i].pos.y, gParticles[i].pos.z);
    }
    glEnd();

    // Crash Flash Overlay
    if (gCrashFlash > 0.0f) {
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix(); glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix(); glLoadIdentity();

        glColor4f(1.0f, 1.0f, 1.0f, gCrashFlash * 0.8f);
        glBegin(GL_QUADS);
        glVertex2f(-1, -1); glVertex2f(1, -1); glVertex2f(1, 1); glVertex2f(-1, 1);
        glEnd();

        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glEnable(GL_DEPTH_TEST);
    }

    // ── HUD 2D Overlay ──
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FOG);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    char buf[128];
    snprintf(buf, sizeof(buf), "SCORE: %d", gScore);
    drawText2D(-0.95f, 0.90f, buf, 1.0f, 0.8f, 0.0f, GLUT_BITMAP_TIMES_ROMAN_24);

    snprintf(buf, sizeof(buf), "BEST: %d", gHighScore);
    drawText2D(-0.95f, 0.82f, buf, 0.5f, 0.5f, 0.5f, GLUT_BITMAP_HELVETICA_18);

    snprintf(buf, sizeof(buf), "SPEED: %.0f MPH", gCurrentSpeed * 5.0f);
    drawText2D(-0.95f, 0.74f, buf, 0.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_18);

    // Active Powerups HUD
    float py = -0.7f;
    if (gShield) {
        drawText2D(-0.95f, py, "[ SHIELD ACTIVE ]", 0.0f, 0.8f, 1.0f, GLUT_BITMAP_HELVETICA_18);
        py -= 0.08f;
    }
    if (gGhostTimer > 0.0f) {
        snprintf(buf, sizeof(buf), "[ GHOST MODE : %.1fs ]", gGhostTimer);
        drawText2D(-0.95f, py, buf, 0.8f, 0.0f, 1.0f, GLUT_BITMAP_HELVETICA_18);
        py -= 0.08f;
    }
    if (gSlowTimer > 0.0f) {
        snprintf(buf, sizeof(buf), "[ SLOW MOTION : %.1fs ]", gSlowTimer);
        drawText2D(-0.95f, py, buf, 1.0f, 1.0f, 0.0f, GLUT_BITMAP_HELVETICA_18);
        py -= 0.08f;
    }

    if (gPickupMessageTimer > 0.0f) {
        float alpha = fmin(1.0f, gPickupMessageTimer * 2.0f);
        glEnable(GL_BLEND);
        glColor4f(1.0f, 1.0f, 1.0f, alpha);
        glRasterPos2f(-0.2f, 0.3f);
        for (const char* p = gPickupMessage; *p; p++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *p);
    }

    if (gGameOver) {
        glEnable(GL_BLEND);
        glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
        glBegin(GL_QUADS);
        glVertex2f(-1, -1); glVertex2f(1, -1); glVertex2f(1, 1); glVertex2f(-1, 1);
        glEnd();

        drawText2D(-0.25f, 0.1f, "CRASHED!", 1.0f, 0.2f, 0.2f, GLUT_BITMAP_TIMES_ROMAN_24);
        drawText2D(-0.35f, -0.1f, "Press R to Restart", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_18);
    }

    glutSwapBuffers();
}

static void reshape(int w, int h) {
    WIN_W = w;
    WIN_H = h ? h : 1;
    glViewport(0, 0, WIN_W, WIN_H);
}

// ──────────────────────────────────────────────────────────────
//  Input
// ──────────────────────────────────────────────────────────────
static void specialKeyDown(int key, int, int) {
    if (!gGameOver) {
        if (key == GLUT_KEY_LEFT && gTargetLane > 0) {
            gTargetLane--;
        }
        if (key == GLUT_KEY_RIGHT && gTargetLane < NUM_LANES - 1) {
            gTargetLane++;
        }
    }
}

static void normalKeyboard(unsigned char key, int, int) {
    if (key == 27) exit(0);
    if (key == 'r' || key == 'R') resetGame();
}

// ──────────────────────────────────────────────────────────────
//  Main
// ──────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    srand((unsigned int)time(nullptr));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Neo Racing 3D: Pro Edition @alamin");

    glClearColor(0.01f, 0.02f, 0.05f, 1.0f);
    resetGame();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(normalKeyboard);
    glutSpecialFunc(specialKeyDown);
    glutTimerFunc(16, updatePhysics, 0);

    glutMainLoop();
    return 0;
}
