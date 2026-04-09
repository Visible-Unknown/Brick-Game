/*
 * ================================================================
 *  NEO SHOOTER 3D  –  OpenGL / FreeGLUT
 *  ================================================================
 *
 *  FEATURES
 *  ─────────────────────────────────────────────────────────────
 *  • 3D rendering with glowing neon aesthetic
 *  • Classic bottom-up space shooter mechanics (Space Invaders style)
 *  • Rapid fire lasers and falling enemy blocks
 *  • Particle explosion effects for destruction
 *  • Scoring system and progressive difficulty
 *  • Dynamic camera tracking
 *
 *  CONTROLS
 *  ─────────────────────────────────────────────────────────────
 *  Arrow Left/Right   Move ship
 *  SPACE              Fire laser
 *  R                  Restart game
 *  ESC                Quit
 *
 *  BUILD
 *  ─────────────────────────────────────────────────────────────
 *  Linux / macOS:
 *    g++ shooter.cpp -o shooter -lGL -lGLU -lglut -lm && ./shooter
 *
 *  Windows (MinGW / Code::Blocks):
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

// ──────────────────────────────────────────────────────────────
//  Window dimensions
// ──────────────────────────────────────────────────────────────
static int WIN_W = 700;
static int WIN_H = 800;

// ──────────────────────────────────────────────────────────────
//  Game Constants
// ──────────────────────────────────────────────────────────────
static const float ARENA_W = 15.0f;
static const float ARENA_H = 25.0f;

struct Vec3 {
    float x, y, z;
    Vec3(float x=0, float y=0, float z=0): x(x), y(y), z(z) {}
};

struct Bullet {
    Vec3 pos;
    bool active;
};

struct Enemy {
    Vec3 pos;
    int hp;
    bool active;
};

struct Particle {
    Vec3 pos, vel;
    float life;
    float r, g, b;
};

struct Star {
    Vec3 pos;
    float speed;
};

enum PowerupType { PT_TRIPLE, PT_SHIELD };
struct Powerup {
    Vec3 pos;
    PowerupType type;
    bool active;
};

// ──────────────────────────────────────────────────────────────
//  Game State
// ──────────────────────────────────────────────────────────────
static float gPlayerX = 0.0f;
static const float gPlayerY = 2.0f;

static std::vector<Bullet> gBullets;
static std::vector<Enemy> gEnemies;
static std::vector<Particle> gParticles;
static std::vector<Star> gStars;
static std::vector<Powerup> gPowerups;

static bool gKeyLeft = false;
static bool gKeyRight = false;
static bool gKeySpace = false;

static float gFireCooldown = 0.0f;
static float gEnemySpawnTimer = 0.0f;
static float gDifficulty = 1.0f;

static int gScore = 0;
static int gHighScore = 0;
static int gPlayerHP = 3;
static bool gGameOver = false;

static float gGlobalTime = 0.0f;
static float gCameraShake = 0.0f;
static float gTripleShotTimer = 0.0f;
static bool gShieldActive = false;
static bool gPaused = false;
static char gPowerupMsg[64] = "";
static float gPowerupMsgTimer = 0.0f;
static float gMsgColor[3] = {1, 1, 1};

// ──────────────────────────────────────────────────────────────
//  Helpers
// ──────────────────────────────────────────────────────────────
static float frand(float lo, float hi) {
    return lo + (hi - lo) * (float)rand() / (float)RAND_MAX;
}

static void spawnParticles(Vec3 pos, float r, float g, float b, int count) {
    for(int i = 0; i < count; i++) {
        Particle p;
        p.pos = pos;
        p.vel = Vec3(frand(-0.3f, 0.3f), frand(-0.3f, 0.3f), frand(0.2f, 0.6f));
        p.r = r; p.g = g; p.b = b;
        p.life = 1.0f;
        gParticles.push_back(p);
    }
}

static void spawnEnemy() {
    Enemy e;
    e.pos = Vec3(frand(-ARENA_W + 1.0f, ARENA_W - 1.0f), ARENA_H, 0.5f);
    e.hp = 1 + (int)(gDifficulty * 0.2f); // Occasional tougher enemies
    e.active = true;
    gEnemies.push_back(e);
}

static void resetGame() {
    gPlayerX = 0.0f;
    gBullets.clear();
    gEnemies.clear();
    gParticles.clear();
    gPowerups.clear();
    
    // Initialize stars if empty
    if (gStars.empty()) {
        for(int i=0; i<100; i++) {
            Star s;
            s.pos = Vec3(frand(-ARENA_W, ARENA_W), frand(0, ARENA_H), frand(-5, -2));
            s.speed = frand(2.0f, 5.0f);
            gStars.push_back(s);
        }
    }
    
    gFireCooldown = 0.0f;
    gEnemySpawnTimer = 0.0f;
    gDifficulty = 1.0f;
    
    gScore = 0;
    gPlayerHP = 3;
    gGameOver = false;
    gCameraShake = 0.0f;
    gTripleShotTimer = 0.0f;
    gShieldActive = false;
    gPaused = false;
#ifdef _WIN32
    mciSendString("resume bgm", NULL, 0, NULL);
#endif
}

static void spawnPowerup(Vec3 pos) {
    if (frand(0, 1) > 0.15f) return; // 15% chance
    Powerup p;
    p.pos = pos;
    p.type = (frand(0, 1) > 0.5f) ? PT_TRIPLE : PT_SHIELD;
    p.active = true;
    gPowerups.push_back(p);
}

// ──────────────────────────────────────────────────────────────
//  Physics / Update
// ──────────────────────────────────────────────────────────────
static void updatePhysics(int) {
    gGlobalTime += 1.0f / 60.0f;
    float dt = 1.0f / 60.0f;
    
    if (gPowerupMsgTimer > 0.0f) gPowerupMsgTimer -= dt;

    if (gPaused) {
        glutPostRedisplay();
        glutTimerFunc(16, updatePhysics, 0);
        return;
    }
    for (size_t i = 0; i < gStars.size(); i++) {
        gStars[i].pos.y -= gStars[i].speed * dt;
        if (gStars[i].pos.y < 0.0f) {
            gStars[i].pos.y = ARENA_H;
            gStars[i].pos.x = frand(-ARENA_W, ARENA_W);
        }
    }

    if (!gGameOver) {
        // Player Movement
        if (gKeyLeft) gPlayerX -= 15.0f * dt;
        if (gKeyRight) gPlayerX += 15.0f * dt;
        
        if (gPlayerX < -ARENA_W + 1.0f) gPlayerX = -ARENA_W + 1.0f;
        if (gPlayerX > ARENA_W - 1.0f) gPlayerX = ARENA_W - 1.0f;
        
        // Firing
        if (gFireCooldown > 0.0f) gFireCooldown -= dt;
        if (gTripleShotTimer > 0.0f) gTripleShotTimer -= dt;

        if (gKeySpace && gFireCooldown <= 0.0f) {
            if (gTripleShotTimer > 0.0f) {
                // Triple Shot
                for (int angle = -1; angle <= 1; angle++) {
                    Bullet b;
                    b.pos = Vec3(gPlayerX + angle * 0.5f, gPlayerY + 1.0f, 0.5f);
                    b.active = true;
                    // Add a tiny bit of horizontal velocity based on angle
                    // Note: Current Bullet struct doesn't have velocity, I'll just use offset for now
                    // To keep it simple, I'll just spawn 3 parallel bullets with offset
                    gBullets.push_back(b);
                }
            } else {
                Bullet b;
                b.pos = Vec3(gPlayerX, gPlayerY + 1.0f, 0.5f);
                b.active = true;
                gBullets.push_back(b);
            }
            gFireCooldown = 0.15f; 
#ifdef _WIN32
            // Realistic laser shot
            PlaySound(TEXT("shooter music/fire_new.mp3"), NULL, SND_FILENAME | SND_ASYNC);
#endif
        }
        
        // Engine Trails
        if (fmodf(gGlobalTime, 0.05f) < 0.02f) {
            spawnParticles(Vec3(gPlayerX, gPlayerY - 0.5f, 0.5f), 0.0f, 0.5f, 1.0f, 2);
        }

        // Update Bullets
        for (size_t i = 0; i < gBullets.size(); i++) {
            if (!gBullets[i].active) continue;
            gBullets[i].pos.y += 30.0f * dt;
            if (gBullets[i].pos.y > ARENA_H + 2.0f) {
                gBullets[i].active = false;
            }
        }
        
        // Update Powerups
        for (size_t i = 0; i < gPowerups.size(); i++) {
            if (!gPowerups[i].active) continue;
            gPowerups[i].pos.y -= 5.0f * dt;
            
            // Collect Powerup
            float dx = fabsf(gPowerups[i].pos.x - gPlayerX);
            float dy = fabsf(gPowerups[i].pos.y - gPlayerY);
            if (dx < 1.5f && dy < 1.5f) {
                gPowerups[i].active = false;
                if (gPowerups[i].type == PT_TRIPLE) {
                    gTripleShotTimer = 5.0f;
                    strcpy(gPowerupMsg, "TRIPLE SHOT ACTIVATED!");
                    gMsgColor[0] = 1.0f; gMsgColor[1] = 1.0f; gMsgColor[2] = 0.0f; // Yellow
                } else if (gPowerups[i].type == PT_SHIELD) {
                    gShieldActive = true;
                    strcpy(gPowerupMsg, "SHIELD REINFORCED!");
                    gMsgColor[0] = 0.0f; gMsgColor[1] = 1.0f; gMsgColor[2] = 1.0f; // Cyan
                }
                gPowerupMsgTimer = 2.0f;
                gScore += 50;
                spawnParticles(gPowerups[i].pos, 1.0f, 1.0f, 1.0f, 15);
#ifdef _WIN32
                // Professional digital chime
                PlaySound(TEXT("shooter music/pickup_new.mp3"), NULL, SND_FILENAME | SND_ASYNC);
#endif
            }
            
            if (gPowerups[i].pos.y < -2.0f) gPowerups[i].active = false;
        }

        // Difficulty increase
        gDifficulty += dt * 0.05f;
        
        // Spawn Enemies
        gEnemySpawnTimer -= dt;
        if (gEnemySpawnTimer <= 0.0f) {
            spawnEnemy();
            gEnemySpawnTimer = frand(0.5f / gDifficulty, 1.5f / gDifficulty);
        }
        
        // Update Enemies
        for (size_t i = 0; i < gEnemies.size(); i++) {
            if (!gEnemies[i].active) continue;
            
            gEnemies[i].pos.y -= (4.0f + gDifficulty * 0.5f) * dt;
            
            // Check Collision with player
            float dx = fabsf(gEnemies[i].pos.x - gPlayerX);
            float dy = fabsf(gEnemies[i].pos.y - gPlayerY);
            if (dx < 1.5f && dy < 1.5f) {
                gEnemies[i].active = false;
                spawnParticles(gEnemies[i].pos, 1.0f, 0.5f, 0.0f, 20);
                
                if (gShieldActive) {
                    gShieldActive = false;
                    gCameraShake = 0.5f;
#ifdef _WIN32
                    // Heavy realistic explosion
                    PlaySound(TEXT("shooter music/crash_new.mp3"), NULL, SND_FILENAME | SND_ASYNC);
#endif
                } else {
                    gPlayerHP--;
                    gCameraShake = 1.0f;
#ifdef _WIN32
                    // Heavy realistic explosion
                    PlaySound(TEXT("shooter music/crash_new.mp3"), NULL, SND_FILENAME | SND_ASYNC);
#endif
                    if (gPlayerHP <= 0) {
                        gGameOver = true;
                        gCameraShake = 2.0f;
                        spawnParticles(Vec3(gPlayerX, gPlayerY, 0.5f), 1.0f, 0.2f, 0.0f, 40);
                    }
                }
                if (gScore > gHighScore) gHighScore = gScore;
            }
            
            // Lost condition (reached bottom)
            if (gEnemies[i].pos.y < 0.0f) {
                gEnemies[i].active = false; // Just remove it, but maybe penalize?
                gPlayerHP--;
                gCameraShake = 0.5f;
                if (gPlayerHP <= 0) {
                    gGameOver = true;
                    if (gScore > gHighScore) gHighScore = gScore;
                }
            }
        }
        
        // Collision: Bullets vs Enemies
        for (size_t i = 0; i < gBullets.size(); i++) {
            if (!gBullets[i].active) continue;
            for (size_t j = 0; j < gEnemies.size(); j++) {
                if (!gEnemies[j].active) continue;
                
                float dx = fabsf(gBullets[i].pos.x - gEnemies[j].pos.x);
                float dy = fabsf(gBullets[i].pos.y - gEnemies[j].pos.y);
                
                if (dx < 1.2f && dy < 1.2f) { // Hit!
                    gBullets[i].active = false;
                    gEnemies[j].hp--;
                    
                    spawnParticles(gBullets[i].pos, 1.0f, 1.0f, 0.0f, 5); 
                    
                    if (gEnemies[j].hp <= 0) {
                        gEnemies[j].active = false;
                        gScore += 10;
                        gCameraShake += 0.15f;
                        spawnParticles(gEnemies[j].pos, 0.2f, 1.0f, 0.2f, 20);
                        spawnPowerup(gEnemies[j].pos);
#ifdef _WIN32
                        PlaySound(TEXT("audio/crash.wav"), NULL, SND_FILENAME | SND_ASYNC);
#endif
                    }
                    break; 
                }
            }
        }
        
        // Clean up inactive
        gBullets.erase(std::remove_if(gBullets.begin(), gBullets.end(), [](const Bullet& b){ return !b.active; }), gBullets.end());
        gEnemies.erase(std::remove_if(gEnemies.begin(), gEnemies.end(), [](const Enemy& e){ return !e.active; }), gEnemies.end());
        gPowerups.erase(std::remove_if(gPowerups.begin(), gPowerups.end(), [](const Powerup& p){ return !p.active; }), gPowerups.end());
    }
    
    // Update Particles
    for (size_t i = 0; i < gParticles.size(); i++) {
        gParticles[i].pos.x += gParticles[i].vel.x;
        gParticles[i].pos.y += gParticles[i].vel.y;
        gParticles[i].pos.z += gParticles[i].vel.z;
        gParticles[i].vel.z -= dt * 1.5f; 
        gParticles[i].life -= dt * 1.5f;
    }
    gParticles.erase(
        std::remove_if(gParticles.begin(), gParticles.end(), [](const Particle& p){ return p.life <= 0.0f; }),
        gParticles.end());
        
    gCameraShake *= 0.9f;

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

static void drawPlayerShip(float x, float y) {
    glPushMatrix();
    glTranslatef(x, y, 0.5f);
    
    // Main Body
    drawBox(0, 0, 0, 0.8f, 1.8f, 0.5f, 0.0f, 0.8f, 1.0f);
    
    // Cockpit
    drawBox(0, 0.3f, 0.3f, 0.4f, 0.6f, 0.3f, 1.0f, 1.0f, 1.0f, 0.5f);
    
    // Wings
    drawBox(-0.8f, -0.2f, 0, 1.0f, 0.5f, 0.2f, 0.0f, 0.5f, 0.8f);
    drawBox(0.8f, -0.2f, 0, 1.0f, 0.5f, 0.2f, 0.0f, 0.5f, 0.8f);
    
    // Tail / Fins
    drawBox(0, -0.6f, 0.3f, 0.1f, 0.4f, 0.5f, 0.0f, 0.4f, 0.7f);
    
    // Engines (Glow)
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.5f, 0.0f);
    glPushMatrix();
    glTranslatef(-0.3f, -0.9f, 0);
    glutSolidCone(0.15f, 0.4f, 8, 1);
    glTranslatef(0.6f, 0, 0);
    glutSolidCone(0.15f, 0.4f, 8, 1);
    glPopMatrix();
    glEnable(GL_LIGHTING);

    // Shield Bubble
    if (gShieldActive) {
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);
        glColor4f(0.0f, 0.5f, 1.0f, 0.3f);
        glutSolidSphere(1.5f, 16, 16);
        glColor4f(0.5f, 0.8f, 1.0f, 0.6f);
        glutWireSphere(1.52f, 16, 16);
        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }
    
    glPopMatrix();
}

static void drawEnemyShip(float x, float y, int hp) {
    glPushMatrix();
    glTranslatef(x, y, 0.5f);
    glRotatef(gGlobalTime * 100.0f, 0, 0, 1);
    
    float r = 1.0f;
    float g = (hp > 1) ? 0.6f : 0.0f;
    float b = 0.0f;
    
    // Core
    drawBox(0, 0, 0, 1.0f, 1.0f, 1.0f, r, g, b);
    
    // Spikes
    glDisable(GL_LIGHTING);
    glColor3f(r, g, b);
    for(int i=0; i<4; i++) {
        glPushMatrix();
        glRotatef(i * 90.0f, 0, 0, 1);
        glTranslatef(0.7f, 0, 0);
        glRotatef(90, 0, 1, 0);
        glutSolidCone(0.2f, 0.5f, 8, 1);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
    
    glPopMatrix();
}

static void drawStarfield() {
    glDisable(GL_LIGHTING);
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (const auto& s : gStars) {
        float brightness = s.speed / 5.0f;
        glColor4f(brightness, brightness, brightness, 0.8f);
        glVertex3f(s.pos.x, s.pos.y, s.pos.z);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

static void drawPowerup(const Powerup& p) {
    glPushMatrix();
    glTranslatef(p.pos.x, p.pos.y, 0.5f);
    glRotatef(gGlobalTime * 150.0f, 0, 1, 0);
    
    if (p.type == PT_TRIPLE) drawBox(0, 0, 0, 0.8f, 0.8f, 0.8f, 1.0f, 1.0f, 0.0f);
    else drawBox(0, 0, 0, 0.8f, 0.8f, 0.8f, 0.0f, 1.0f, 1.0f);
    
    // Spinning wireframe
    glDisable(GL_LIGHTING);
    glColor3f(1, 1, 1);
    glutWireCube(1.0f);
    glEnable(GL_LIGHTING);
    
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
    
    gluLookAt(0.0f + shakeX, -5.0f + shakeY, 20.0f + shakeZ, 
              0.0f, ARENA_H / 2.0f, 0.0f, 
              0.0f, 1.0f, 0.0f);

    GLfloat lpos[] = { 0.0f, ARENA_H / 2.0f, 10.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lpos);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    // Starfield
    drawStarfield();

    // Arena Floor
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glColor4f(0.05f, 0.15f, 0.3f, 0.4f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    for (float x = -ARENA_W; x <= ARENA_W; x += 2.0f) {
        glVertex3f(x, 0.0f, 0.0f); glVertex3f(x, ARENA_H, 0.0f);
    }
    for (float y = 0.0f; y <= ARENA_H; y += 2.0f) {
        glVertex3f(-ARENA_W, y, 0.0f); glVertex3f(ARENA_W, y, 0.0f);
    }
    glEnd();

    // Side Borders (Neon)
    glColor4f(0.0f, 0.8f, 1.0f, 0.8f);
    glLineWidth(4.0f);
    glBegin(GL_LINES);
    glVertex3f(-ARENA_W, 0.0f, 0.0f); glVertex3f(-ARENA_W, ARENA_H, 0.0f);
    glVertex3f(ARENA_W, 0.0f, 0.0f);  glVertex3f(ARENA_W, ARENA_H, 0.0f);
    glEnd();
    
    glEnable(GL_LIGHTING);

    // Draw Player
    if (!gGameOver) {
        drawPlayerShip(gPlayerX, gPlayerY);
    }

    // Draw Enemies
    for (size_t i = 0; i < gEnemies.size(); i++) {
        drawEnemyShip(gEnemies[i].pos.x, gEnemies[i].pos.y, gEnemies[i].hp);
    }

    // Draw Powerups
    for (size_t i = 0; i < gPowerups.size(); i++) {
        drawPowerup(gPowerups[i]);
    }

    // Draw Bullets
    glDisable(GL_LIGHTING);
    glLineWidth(4.0f);
    glBegin(GL_LINES);
    for (size_t i = 0; i < gBullets.size(); i++) {
        glColor4f(1.0f, 1.0f, 0.5f, 1.0f);
        glVertex3f(gBullets[i].pos.x, gBullets[i].pos.y - 0.5f, gBullets[i].pos.z);
        glColor4f(1.0f, 0.5f, 0.0f, 0.0f);
        glVertex3f(gBullets[i].pos.x, gBullets[i].pos.y - 2.5f, gBullets[i].pos.z);
    }
    glEnd();
    glEnable(GL_LIGHTING);

    // Draw Particles
    glDisable(GL_LIGHTING);
    glPointSize(5.0f);
    glBegin(GL_POINTS);
    for (size_t i = 0; i < gParticles.size(); i++) {
        glColor4f(gParticles[i].r, gParticles[i].g, gParticles[i].b, gParticles[i].life);
        glVertex3f(gParticles[i].pos.x, gParticles[i].pos.y, gParticles[i].pos.z);
    }
    glEnd();

    // HUD 2D Overlay
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    char buf[128];
    snprintf(buf, sizeof(buf), "SCORE: %05d", gScore);
    drawText2D(-0.95f, 0.90f, buf, 1.0f, 0.8f, 0.0f, GLUT_BITMAP_TIMES_ROMAN_24);
    
    snprintf(buf, sizeof(buf), "BEST: %05d", gHighScore);
    drawText2D(-0.95f, 0.82f, buf, 0.5f, 0.5f, 0.5f, GLUT_BITMAP_HELVETICA_18);
    
    // HP Display (Hearts)
    char hpBuf[32] = "LIVES: ";
    for(int i=0; i<3; i++) {
        if (i < gPlayerHP) strcat(hpBuf, "[O] ");
        else strcat(hpBuf, "[ ] ");
    }
    drawText2D(0.55f, 0.90f, hpBuf, 0.2f, 1.0f, 0.5f, GLUT_BITMAP_HELVETICA_18);

    // Power-up status
    if (gTripleShotTimer > 0.0f) {
        snprintf(buf, sizeof(buf), "TRIPLE SHOT: %.1fs", gTripleShotTimer);
        drawText2D(-0.95f, -0.90f, buf, 1.0f, 1.0f, 0.0f);
    }
    if (gShieldActive) {
        drawText2D(-0.95f, -0.82f, "SHIELD: ACTIVE", 0.0f, 1.0f, 1.0f);
    }

    if (gGameOver) {
        glEnable(GL_BLEND);
        glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
        glBegin(GL_QUADS);
        glVertex2f(-1, -1); glVertex2f(1, -1); glVertex2f(1, 1); glVertex2f(-1, 1);
        glEnd();
        
        drawText2D(-0.25f, 0.1f, "MISSION FAILED", 1.0f, 0.2f, 0.2f, GLUT_BITMAP_TIMES_ROMAN_24);
        drawText2D(-0.35f, -0.1f, "Press R to Restart System", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_18);
    } else if (gPaused) {
        glEnable(GL_BLEND);
        glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
        glBegin(GL_QUADS);
        glVertex2f(-1, -1); glVertex2f(1, -1); glVertex2f(1, 1); glVertex2f(-1, 1);
        glEnd();
        
        drawText2D(-0.12f, 0.05f, "PAUSED", 1.0f, 1.0f, 0.0f, GLUT_BITMAP_TIMES_ROMAN_24);
        drawText2D(-0.25f, -0.1f, "Press P to Resume", 1.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_18);
    }
    
    // Powerup Notification Message
    if (gPowerupMsgTimer > 0.0f) {
        float alpha = std::min(1.0f, gPowerupMsgTimer);
        // Add a small lift animation based on time left
        float yOffset = (2.0f - gPowerupMsgTimer) * 0.15f; 
        drawText2D(-0.3f, 0.3f + yOffset, gPowerupMsg, gMsgColor[0], gMsgColor[1], gMsgColor[2], GLUT_BITMAP_TIMES_ROMAN_24);
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
    if (key == GLUT_KEY_LEFT)  gKeyLeft = true;
    if (key == GLUT_KEY_RIGHT) gKeyRight = true;
}

static void specialKeyUp(int key, int, int) {
    if (key == GLUT_KEY_LEFT)  gKeyLeft = false;
    if (key == GLUT_KEY_RIGHT) gKeyRight = false;
}

static void normalKeyboardDown(unsigned char key, int, int) {
    if (key == 27) exit(0);
    if (key == 'r' || key == 'R') resetGame();
    if (key == ' ') gKeySpace = true;
    if (key == 'p' || key == 'P') {
        gPaused = !gPaused;
#ifdef _WIN32
        if (gPaused) mciSendString("pause bgm", NULL, 0, NULL);
        else mciSendString("resume bgm", NULL, 0, NULL);
#endif
    }
}

static void normalKeyboardUp(unsigned char key, int, int) {
    if (key == ' ') gKeySpace = false;
}

// ──────────────────────────────────────────────────────────────
//  Main
// ──────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    srand((unsigned int)time(nullptr));
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Neo Shooter 3D @alamin");

    glClearColor(0.01f, 0.02f, 0.05f, 1.0f);
    resetGame();
    
    // Start Background Music
#ifdef _WIN32
    mciSendString("open \"shooter music/bgm_new.mp3\" type mpegvideo alias bgm", NULL, 0, NULL);
    mciSendString("play bgm repeat", NULL, 0, NULL);
#endif

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(normalKeyboardDown);
    glutKeyboardUpFunc(normalKeyboardUp);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);
    glutTimerFunc(16, updatePhysics, 0);

    glutMainLoop();
    return 0;
}
