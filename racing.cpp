/*
 * ================================================================
 *  NEO RACING 3D  –  OpenGL / FreeGLUT (Power-Up Editon)
 *  ================================================================
 *
 *  FEATURES
 *  ─────────────────────────────────────────────────────────────
 *  • 3D rendering with glowing neon retro-wave style
 *  • Highway lane mechanics with scrolling grid ground
 *  • Accelerating endless runner gameplay
 *  • 4 Power-Up Types (Shield, Ghost Mode, Time Slow, Bonus Score)
 *  • Particle explosion effects on crash or powerup pickup
 *  • Score and speed progression
 *
 *  CONTROLS
 *  ─────────────────────────────────────────────────────────────
 *  Arrow Left/Right   Change lanes
 *  R                  Restart game
 *  ESC                Quit
 *
 *  BUILD
 *  ─────────────────────────────────────────────────────────────
 *  Linux / macOS:
 *    g++ racing.cpp -o racing -lGL -lGLU -lglut -lm && ./racing
 *
 *  Windows (MinGW / Code::Blocks):
 *    g++ racing.cpp -o racing.exe -lfreeglut -lopengl32 -lglu32
 * ================================================================
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

// The ground limits
static const float ROAD_LENGTH = 50.0f;
static const float ROAD_Z_START = 10.0f; // Camera is at ~0, pushing road back
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
    float spin; // Used for rendering powerups
};

struct Particle {
    Vec3 pos, vel;
    float life;
    float r, g, b;
};

// ──────────────────────────────────────────────────────────────
//  Game State
// ──────────────────────────────────────────────────────────────
static int gPlayerLane = 1; // 0, 1, 2
static float gPlayerZ = 0.0f;

static std::vector<Entity> gEntities;
static std::vector<Particle> gParticles;

static float gBaseSpeed = 20.0f; // Target speed naturally scaling
static float gCurrentSpeed = 20.0f; // Actual speed (affected by slow-mo)
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
    return (lane - 1) * LANE_WIDTH; // -1 for left, 0 for mid, 1 for right
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
    ent.z = ROAD_Z_END - 5.0f; // Spawn beyond horizon
    ent.spin = 0.0f;
    
    // 15% chance to be a power-up instead of a car
    if ((rand() % 100) < 15) {
        ent.type = ENT_POWERUP;
        ent.puType = (PowerUpType)(rand() % PU_COUNT);
    } else {
        ent.type = ENT_ENEMY_CAR;
    }
    
    gEntities.push_back(ent);
}

static void resetGame() {
    gPlayerLane = 1;
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
}

// ──────────────────────────────────────────────────────────────
//  Physics / Update
// ──────────────────────────────────────────────────────────────
static void updatePhysics(int) {
    gGlobalTime += 1.0f / 60.0f;
    float dt = 1.0f / 60.0f;
    
    if (!gGameOver) {
        gBaseSpeed += dt * 0.15f; // Gradually increase natural speed
        
        // Power-Up Timers
        if (gGhostTimer > 0.0f) gGhostTimer -= dt;
        if (gSlowTimer > 0.0f) {
            gSlowTimer -= dt;
            gCurrentSpeed = gBaseSpeed * 0.5f; // Half speed in slow-mo
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

        // Move entities towards player (positive Z direction)
        for (size_t i = 0; i < gEntities.size(); i++) {
            gEntities[i].z += gCurrentSpeed * dt;
            gEntities[i].spin += 150.0f * dt;
            
            // Collision Check
            if (gEntities[i].lane == gPlayerLane) {
                if (gEntities[i].z > gPlayerZ - 1.2f && gEntities[i].z < gPlayerZ + 1.2f) {
                    float ex = getLaneX(gPlayerLane);
                    if (gEntities[i].type == ENT_ENEMY_CAR) {
                        if (gGhostTimer > 0.0f) {
                            // Pass through safely (Ghost Mode)
                        } else if (gShield) {
                            // Shield break
                            gShield = false;
                            setPickupMessage("SHIELD BROKEN!");
                            gCameraShake = 0.6f;
                            gEntities[i].z = ROAD_Z_START + 10.0f; // Throw past player
                            spawnParticles(Vec3(ex, 0.5f, gPlayerZ), 0.2f, 0.8f, 1.0f, 25);
                        } else {
                            // Crash
                            gGameOver = true;
                            gCameraShake = 1.5f;
                            gCrashFlash = 1.0f;
                            spawnParticles(Vec3(ex, 0.5f, gPlayerZ), 1.0f, 0.2f, 0.0f, 40);
                        }
                    } else if (gEntities[i].type == ENT_POWERUP) {
                        // Pickup Powerup
                        applyPowerup(gEntities[i].puType);
                        gEntities[i].z = ROAD_Z_START + 10.0f; // Consume
                        spawnParticles(Vec3(ex, 0.5f, gPlayerZ), 1.0f, 1.0f, 0.0f, 20);
                    }
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
            // Spawn rate gets faster as you speed up (use base speed to prevent slow-mo clusters)
            gSpawnTimer = frand(8.0f / gBaseSpeed, 18.0f / gBaseSpeed); 
        }
    }
    
    // Update Particles
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
static void drawBox(float x, float y, float z, float w, float h, float d, float r, float g, float b, float alpha=1.0f) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(w, h, d);
    
    GLfloat diff[] = { r*0.8f, g*0.8f, b*0.8f, alpha };
    GLfloat ambi[] = { r*0.3f, g*0.3f, b*0.3f, alpha };
    GLfloat spec[] = { 1.0f, 1.0f, 1.0f, alpha };
    glMaterialfv(GL_FRONT, GL_DIFFUSE,  diff);
    glMaterialfv(GL_FRONT, GL_AMBIENT,  ambi);
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    glMaterialf (GL_FRONT, GL_SHININESS, 60.0f);
    
    if (alpha < 1.0f) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    
    glutSolidCube(1.0f);
    
    // Wireframe glow
    glDisable(GL_LIGHTING);
    glColor4f(r, g, b, alpha);
    glLineWidth(2.0f);
    glutWireCube(1.02f);
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
    gluPerspective(60.0, (double)WIN_W / WIN_H, 0.1, 100.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    float shakeX = gCameraShake * frand(-0.5f, 0.5f);
    float shakeY = gCameraShake * frand(-0.5f, 0.5f);
    float shakeZ = gCameraShake * frand(-0.5f, 0.5f);
    
    // Camera is behind and above the player
    gluLookAt(0.0f + shakeX, 3.5f + shakeY, 6.0f + shakeZ,  // Eye
              0.0f, 0.0f, gPlayerZ - 10.0f,                 // Center (looking down road)
              0.0f, 1.0f, 0.0f);                            // Up

    GLfloat lpos[] = { 0.0f, 10.0f, 5.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    // Draw the Road Grid (Synthwave style)
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    float roadW = NUM_LANES * LANE_WIDTH + 2.0f;
    float hw = roadW / 2.0f;
    
    // Far horizon fade using fog
    glEnable(GL_FOG);
    GLfloat fogColor[] = {0.01f, 0.02f, 0.05f, 1.0f};
    glFogfv(GL_FOG_COLOR, fogColor);
    glFogi(GL_FOG_MODE, GL_LINEAR);
    glFogf(GL_FOG_START, 10.0f);
    glFogf(GL_FOG_END, 45.0f);
    
    // Draw ground base
    glDisable(GL_CULL_FACE);
    glColor4f(0.05f, 0.0f, 0.1f, 1.0f);
    glBegin(GL_QUADS);
    glVertex3f(-hw, -0.01f, ROAD_Z_START);
    glVertex3f( hw, -0.01f, ROAD_Z_START);
    glVertex3f( hw, -0.01f, ROAD_Z_END);
    glVertex3f(-hw, -0.01f, ROAD_Z_END);
    glEnd();

    // Draw grid lines
    glLineWidth(2.0f);
    
    // Fast moving horizontal lines
    // If in ghost mode, turn grid slightly purple
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
    
    // Lane separator lines
    glColor4f(0.0f, 0.8f, 1.0f, 0.5f);
    glBegin(GL_LINES);
    for (int i = 0; i <= NUM_LANES; i++) {
        float lx = getLaneX(i) - LANE_WIDTH/2.0f;
        glVertex3f(lx, 0.0f, ROAD_Z_START);
        glVertex3f(lx, 0.0f, ROAD_Z_END);
    }
    glEnd();

    glEnable(GL_LIGHTING);

    // Draw Player Car
    if (!gGameOver) {
        float px = getLaneX(gPlayerLane);
        float alpha = (gGhostTimer > 0.0f) ? 0.4f : 1.0f;
        float pcr = (gShield) ? 0.0f : 0.0f;
        float pcg = (gShield) ? 0.8f : 1.0f;
        float pcb = (gShield) ? 1.0f : 1.0f;
        
        drawBox(px, 0.4f, gPlayerZ, 
                1.4f, 0.8f, 2.5f, 
                pcr, pcg, pcb, alpha);
        
        // Draw Shield Ring
        if (gShield) {
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glColor4f(0.0f, 0.8f, 1.0f, 0.6f + 0.3f * sin(gGlobalTime * 10.0f));
            glLineWidth(3.0f);
            glPushMatrix();
            glTranslatef(px, 0.4f, gPlayerZ);
            glRotatef(90, 1.0f, 0.0f, 0.0f);
            glutWireTorus(0.1, 1.8, 10, 20);
            glPopMatrix();
            glEnable(GL_LIGHTING);
        }

        // Engine exhaust glow
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        float exhaustFlicker = 0.8f + 0.2f * sin(gGlobalTime * 30.0f);
        glColor4f(0.0f, 0.5f, 1.0f, 0.6f * exhaustFlicker);
        glPushMatrix();
        glTranslatef(px, 0.2f, gPlayerZ + 1.25f);
        glutSolidSphere(0.4f, 8, 8);
        glPopMatrix();
        glEnable(GL_LIGHTING);
    }

    // Draw Entities
    for (size_t i = 0; i < gEntities.size(); i++) {
        float ex = getLaneX(gEntities[i].lane);
        float ez = gEntities[i].z;
        
        if (gEntities[i].type == ENT_ENEMY_CAR) {
            drawBox(ex, 0.5f, ez, 
                    1.4f, 1.0f, 2.0f, 
                    1.0f, 0.1f, 0.3f);
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
            else { r=0; g=1; b=0; } // Bonus
            
            glColor4f(r, g, b, 0.8f);
            glutSolidOctahedron();
            glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
            glutWireOctahedron();
            glPopMatrix();
            glEnable(GL_LIGHTING);
        }
    }

    // Draw Particles
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glPointSize(6.0f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < gParticles.size(); i++) {
        glColor4f(gParticles[i].r, gParticles[i].g, gParticles[i].b, gParticles[i].life);
        glVertex3f(gParticles[i].pos.x, gParticles[i].pos.y, gParticles[i].pos.z);
    }
    glEnd();
    
    // Crash Flash Overlay (Full screen)
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

    // HUD 2D Overlay
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

    // Pickup Message Center HUD
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
        if (key == GLUT_KEY_LEFT && gPlayerLane > 0) {
            gPlayerLane--;
        }
        if (key == GLUT_KEY_RIGHT && gPlayerLane < NUM_LANES - 1) {
            gPlayerLane++;
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
    glutCreateWindow("Neo Racing 3D: Power-Up Edition @alamin");

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
