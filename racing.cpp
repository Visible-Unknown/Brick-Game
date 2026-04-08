/*
 * ================================================================
 *  NEO RACING 3D  –  OpenGL / FreeGLUT
 *  ================================================================
 *
 *  FEATURES
 *  ─────────────────────────────────────────────────────────────
 *  • 3D rendering with glowing neon retro-wave style
 *  • Highway lane mechanics with scrolling grid ground
 *  • Accelerating endless runner gameplay
 *  • High speed motion blur/trail effects
 *  • Particle explosion effects on crash
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

struct Obstacle {
    int lane;
    float z;
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

static std::vector<Obstacle> gObstacles;
static std::vector<Particle> gParticles;

static float gSpeed = 20.0f; // Units per second
static float gScrollOffset = 0.0f;

static int gScore = 0;
static int gHighScore = 0;
static float gScoreAccumulator = 0.0f;
static bool gGameOver = false;

static float gGlobalTime = 0.0f;
static float gCameraShake = 0.0f;
static float gCrashFlash = 0.0f;

static float gSpawnTimer = 0.0f;

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

static void spawnObstacle() {
    Obstacle obs;
    obs.lane = rand() % NUM_LANES;
    obs.z = ROAD_Z_END - 5.0f; // Spawn beyond the far horizon
    gObstacles.push_back(obs);
}

static void resetGame() {
    gPlayerLane = 1;
    gObstacles.clear();
    gParticles.clear();
    
    gSpeed = 15.0f;
    gScore = 0;
    gScoreAccumulator = 0.0f;
    gGameOver = false;
    
    gSpawnTimer = 0.0f;
    gCameraShake = 0.0f;
    gCrashFlash = 0.0f;
}

// ──────────────────────────────────────────────────────────────
//  Physics / Update
// ──────────────────────────────────────────────────────────────
static void updatePhysics(int) {
    gGlobalTime += 1.0f / 60.0f;
    float dt = 1.0f / 60.0f;
    
    if (!gGameOver) {
        gSpeed += dt * 0.15f; // Gradually increase speed over time
        
        gScrollOffset += gSpeed * dt;
        if (gScrollOffset > 4.0f) gScrollOffset -= 4.0f;
        
        gScoreAccumulator += gSpeed * dt * 0.1f;
        if (gScoreAccumulator >= 1.0f) {
            gScore += 1;
            gScoreAccumulator -= 1.0f;
            if (gScore > gHighScore) gHighScore = gScore;
        }

        // Move obstacles towards player (positive Z direction)
        for (size_t i = 0; i < gObstacles.size(); i++) {
            gObstacles[i].z += gSpeed * dt;
            
            // Collision Check
            if (gObstacles[i].lane == gPlayerLane) {
                if (gObstacles[i].z > gPlayerZ - 1.0f && gObstacles[i].z < gPlayerZ + 1.0f) {
                    gGameOver = true;
                    gCameraShake = 1.5f;
                    gCrashFlash = 1.0f;
                    spawnParticles(Vec3(getLaneX(gPlayerLane), 0.5f, gPlayerZ), 1.0f, 0.2f, 0.0f, 40);
                }
            }
        }
        
        // Remove obstacles behind camera
        gObstacles.erase(
            std::remove_if(gObstacles.begin(), gObstacles.end(), [](const Obstacle& o){ return o.z > ROAD_Z_START + 5.0f; }),
            gObstacles.end());

        // Spawn new obstacles
        gSpawnTimer -= dt;
        if (gSpawnTimer <= 0.0f) {
            spawnObstacle();
            // Spawn rate gets faster as you speed up
            gSpawnTimer = frand(8.0f / gSpeed, 18.0f / gSpeed); 
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
              0.0f, 0.0f, gPlayerZ - 10.0f,                 // Center (looking far down the road)
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
    glColor4f(1.0f, 0.0f, 0.8f, 0.5f); 
    glBegin(GL_LINES);
    for (float z = ROAD_Z_START; z > ROAD_Z_END; z -= 4.0f) {
        float offsetZ = z + gScrollOffset; // Move towards camera
        if (offsetZ > ROAD_Z_START) offsetZ -= (ROAD_Z_START - ROAD_Z_END); // Wrapping
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
        // Player is a sleek futuristic box
        drawBox(px, 0.4f, gPlayerZ, 
                1.4f, 0.8f, 2.5f, 
                0.0f, 1.0f, 1.0f);
                
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

    // Draw Obstacles (Enemy cars/blocks)
    for (size_t i = 0; i < gObstacles.size(); i++) {
        float ox = getLaneX(gObstacles[i].lane);
        float oz = gObstacles[i].z;
        drawBox(ox, 0.5f, oz, 
                1.4f, 1.0f, 2.0f, 
                1.0f, 0.1f, 0.3f);
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
    
    snprintf(buf, sizeof(buf), "SPEED: %.0f MPH", gSpeed * 5.0f);
    drawText2D(-0.95f, 0.74f, buf, 0.0f, 1.0f, 1.0f, GLUT_BITMAP_HELVETICA_18);

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
    glutCreateWindow("Neo Racing 3D @alamin");

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
