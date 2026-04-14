/*
 * ================================================================
 *  NEO ASTEROIDS: VOID DRIFT  –  OpenGL / FreeGLUT [PRO EDITION]
 * ================================================================
 *  Vector-style space survival with rock splitting, UFO enemies,
 *  screen-wrap, Pro Edition UI, audio, and particles.
 *
 *  CONTROLS: Up = Thrust, Left/Right = Rotate, SPACE = Fire
 *  BUILD: g++ asteroids.cpp -o asteroids.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
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
#include <vector>
#include <ctime>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void playBGM(const char* f, bool loop) {
    char cmd[512]; snprintf(cmd, 512, "close bgm"); mciSendStringA(cmd, 0, 0, 0);
    if (!f) return;
    snprintf(cmd, 512, "open \"%s\" type mpegvideo alias bgm", f);
    if (mciSendStringA(cmd, 0, 0, 0) == 0) {
        snprintf(cmd, 512, "play bgm %s", loop ? "repeat" : "");
        mciSendStringA(cmd, 0, 0, 0);
    }
}
static void playSFX(const char* f) { PlaySoundA(f, NULL, SND_ASYNC | SND_FILENAME | SND_NODEFAULT); }

static int WIN_W = 1280, WIN_H = 720;
static const float W = 32.0f, H = 18.0f;
enum GameState { STATE_HOME, STATE_PLAY, STATE_GAMEOVER };

struct Entity {
    float x, y, vx, vy, ang, rad;
    int type; // 0=Ship, 1=Rock, 2=Bullet, 3=UFO
    bool alive;
    float r, g, b;
    float timer;
    int size; // 2=Large, 1=Med, 0=Small
};

static GameState gState = STATE_HOME;
static Entity gShip;
static std::vector<Entity> gRocks;
static std::vector<Entity> gBullets;
static std::vector<Entity> gUFOs;
struct Particle { float x, y, vx, vy, life, r, g, b; };
static std::vector<Particle> gParts;

static int gScore = 0, gHighScore = 0, gLives = 3;
static float gGlobalTime = 0, gCamShake = 0;
static bool gPaused = false, gThrust = false, gRotL = false, gRotR = false;
static float gFireTimer = 0, gUFOSpawnTimer = 10.0f;
const char* SAVE = "highscore.dat";

static float frand(float a, float b) { return a + (b - a) * (float)rand() / (float)RAND_MAX; }
static void saveHS() { FILE* f = fopen(SAVE, "w"); if (f) { fprintf(f, "%d", gHighScore); fclose(f); } }
static void loadHS() { FILE* f = fopen(SAVE, "r"); if (f) { if (fscanf(f, "%d", &gHighScore) != 1) gHighScore = 0; fclose(f); } }
static void spawnP(float px, float py, float r, float g, float b, int n) {
    for (int i = 0; i < n; i++) gParts.push_back({ px, py, frand(-3, 3), frand(-3, 3), 1.0f, r, g, b });
}

static void spawnRock(float x, float y, int size) {
    Entity r; r.x = x; r.y = y; r.vx = frand(-3, 3); r.vy = frand(-3, 3);
    r.ang = frand(0, 360); r.rad = (size + 1) * 0.7f; r.type = 1; r.size = size; r.alive = true;
    r.r = 0.5f + 0.3f * frand(0, 1); r.g = 0.6f + 0.3f * frand(0, 1); r.b = 0.9f + 0.1f * frand(0, 1);
    gRocks.push_back(r);
}

static void resetGame() {
    gScore = 0; gLives = 3; gState = STATE_PLAY; gPaused = false;
    gShip = { 0, 0, 0, 0, 0, 0.5f, 0, true, 0, 1, 0.5f, 0, 0 };
    gRocks.clear(); gBullets.clear(); gUFOs.clear(); gParts.clear();
    for (int i = 0; i < 6; i++) {
        float rx, ry; do { rx = frand(-W / 2, W / 2); ry = frand(-H / 2, H / 2); } while (sqrt(rx * rx + ry * ry) < 5.0f);
        spawnRock(rx, ry, 2);
    }
    playBGM("../racing/audio/bgm.mp3", true);
}

static void update(int) {
    float dt = 1.0f / 60.0f; gGlobalTime += dt;
    if (gState == STATE_PLAY && !gPaused) {
        // Ship
        if (gRotL) gShip.ang += 250.0f * dt; if (gRotR) gShip.ang -= 250.0f * dt;
        if (gThrust) {
            float ar = gShip.ang * M_PI / 180.0f;
            gShip.vx += cos(ar) * 15.0f * dt; gShip.vy += sin(ar) * 15.0f * dt;
            if (frand(0, 1) > 0.5f) spawnP(gShip.x - cos(ar) * 0.4f, gShip.y - sin(ar) * 0.4f, 1, 0.5f, 0.1f, 1);
        }
        gShip.vx *= 0.99f; gShip.vy *= 0.99f;
        gShip.x += gShip.vx * dt; gShip.y += gShip.vy * dt;
        if (gShip.x < -W / 2) gShip.x = W / 2; if (gShip.x > W / 2) gShip.x = -W / 2;
        if (gShip.y < -H / 2) gShip.y = H / 2; if (gShip.y > H / 2) gShip.y = -H / 2;

        gFireTimer -= dt;

        // Rocks
        for (auto& r : gRocks) {
            r.x += r.vx * dt; r.y += r.vy * dt;
            if (r.x < -W / 2) r.x = W / 2; if (r.x > W / 2) r.x = -W / 2;
            if (r.y < -H / 2) r.y = H / 2; if (r.y > H / 2) r.y = -H / 2;
            // Rock -> Ship
            float dx = gShip.x - r.x, dy = gShip.y - r.y;
            if (dx * dx + dy * dy < (gShip.rad + r.rad) * (gShip.rad + r.rad)) {
                playSFX("../tanks/tanks music/explosion.wav"); gLives--; gCamShake = 1.0f;
                spawnP(gShip.x, gShip.y, 1, 0.2f, 0.1f, 30);
                if (gLives <= 0) { gState = STATE_GAMEOVER; playBGM(0, false); if (gScore > gHighScore) { gHighScore = gScore; saveHS(); } }
                else { gShip.x = 0; gShip.y = 0; gShip.vx = 0; gShip.vy = 0; }
                break;
            }
        }

        // Bullets
        for (int i = (int)gBullets.size() - 1; i >= 0; i--) {
            auto& b = gBullets[i]; b.x += b.vx * dt; b.y += b.vy * dt; b.timer -= dt;
            if (b.timer <= 0) { gBullets.erase(gBullets.begin() + i); continue; }
            if (b.x < -W / 2) b.x = W / 2; if (b.x > W / 2) b.x = -W / 2;
            if (b.y < -H / 2) b.y = H / 2; if (b.y > H / 2) b.y = -H / 2;
            // Bullet -> Rock
            for (auto& r : gRocks) {
                if (!r.alive) continue;
                float dx = b.x - r.x, dy = b.y - r.y;
                if (dx * dx + dy * dy < r.rad * r.rad) {
                    r.alive = false; b.timer = 0; gScore += 100; gCamShake += 0.2f;
                    spawnP(r.x, r.y, r.r, r.g, r.b, 15); playSFX("../match3/match3 music/explosion.wav");
                    if (r.size > 0) { spawnRock(r.x, r.y, r.size - 1); spawnRock(r.x, r.y, r.size - 1); }
                    break;
                }
            }
        }
        gRocks.erase(std::remove_if(gRocks.begin(), gRocks.end(), [](const Entity& r) {return !r.alive; }), gRocks.end());
        if (gRocks.empty()) { for (int i = 0; i < 6 + gScore / 5000; i++) spawnRock(frand(-W / 2, W / 2), frand(-H / 2, H / 2), 2); }

        // UFO
        gUFOSpawnTimer -= dt;
        if (gUFOSpawnTimer <= 0) {
            Entity u; u.x = -W / 2 - 1; u.y = frand(-H / 2, H / 2); u.vx = 4; u.vy = frand(-1, 1); u.rad = 0.6f; u.type = 3; u.alive = true; u.timer = 2.0f;
            gUFOs.push_back(u); gUFOSpawnTimer = frand(15, 25);
        }
        for (int i = (int)gUFOs.size() - 1; i >= 0; i--) {
            auto& u = gUFOs[i]; u.x += u.vx * dt; u.y += u.vy * dt; u.timer -= dt;
            if (u.timer <= 0) {
                u.timer = frand(1.5f, 3.0f);
                float a = atan2(gShip.y - u.y, gShip.x - u.x);
                Entity b; b.x = u.x; b.y = u.y; b.vx = cos(a) * 10; b.vy = sin(a) * 10; b.timer = 2.0f; b.alive = true; b.type = 2; b.r = 1; b.g = 0; b.b = 0;
                gBullets.push_back(b);
            }
            if (u.x > W / 2 + 1) { gUFOs.erase(gUFOs.begin() + i); continue; }
            // UFO -> Bullet
            for (auto& b : gBullets) {
                float dx = b.x - u.x, dy = b.y - u.y;
                if (dx * dx + dy * dy < u.rad * u.rad) { u.alive = false; b.timer = 0; gScore += 500; spawnP(u.x, u.y, 1, 0, 0, 20); playSFX("../match3/match3 music/explosion.wav"); break; }
            }
            if (!u.alive) { gUFOs.erase(gUFOs.begin() + i); continue; }
            // UFO -> Ship
            float dx = gShip.x - u.x, dy = gShip.y - u.y;
            if (dx * dx + dy * dy < (gShip.rad + u.rad) * (gShip.rad + u.rad)) { u.alive = false; gLives--; gCamShake = 1.0f; spawnP(gShip.x, gShip.y, 1, 1, 0, 30); }
        }
    }
    for (int i = (int)gParts.size() - 1; i >= 0; i--) { gParts[i].x += gParts[i].vx * dt; gParts[i].y += gParts[i].vy * dt; gParts[i].life -= dt; if (gParts[i].life <= 0) gParts.erase(gParts.begin() + i); }
    gCamShake *= 0.9f; glutPostRedisplay(); glutTimerFunc(16, update, 0);
}

static void drawVectorRock(const Entity& r) {
    glColor3f(r.r, r.g, r.b); glLineWidth(2);
    glPushMatrix(); glTranslatef(r.x, r.y, 0); glRotatef(r.ang, 0, 0, 1);
    glBegin(GL_LINE_LOOP); for (int i = 0; i < 8; i++) { float a = i * 2 * M_PI / 8; float d = r.rad * (0.8f + 0.4f * sin(i * 1.5f + r.timer)); glVertex2f(cos(a) * d, sin(a) * d); } glEnd();
    glPopMatrix();
}

static void strokeCentered(float cx, float cy, const std::string& s, float sc, float r, float g, float b, float a) {
    float tw = 0; for (char c : s)tw += glutStrokeWidth(GLUT_STROKE_ROMAN, c); float sx = cx - tw * sc * 0.5f;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); glColor4f(r, g, b, a * 0.35f); glLineWidth(6);
    glPushMatrix(); glTranslatef(sx, cy, 0); glScalef(sc, sc, sc); for (char c : s)glutStrokeCharacter(GLUT_STROKE_ROMAN, c); glPopMatrix();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); glColor4f(1, 1, 1, a); glLineWidth(2.5f);
    glPushMatrix(); glTranslatef(sx, cy, 0); glScalef(sc, sc, sc); for (char c : s)glutStrokeCharacter(GLUT_STROKE_ROMAN, c); glPopMatrix();
    glLineWidth(1); glDisable(GL_BLEND);
}

static void display() {
    glClear(GL_COLOR_BUFFER_BIT); glMatrixMode(GL_PROJECTION); glLoadIdentity();
    float sx = gCamShake * frand(-0.2f, 0.2f), sy = gCamShake * frand(-0.2f, 0.2f);
    gluOrtho2D(-W / 2 + sx, W / 2 + sx, -H / 2 + sy, H / 2 + sy);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    // Starfield
    glPointSize(1); glColor3f(0.5f, 0.5f, 0.8f); glBegin(GL_POINTS);
    for (int i = 0; i < 100; i++) glVertex2f(-W / 2 + frand(0, W), -H / 2 + frand(0, H)); glEnd();

    // Ship
    if (gState != STATE_GAMEOVER) {
        glPushMatrix(); glTranslatef(gShip.x, gShip.y, 0); glRotatef(gShip.ang, 0, 0, 1);
        glColor3f(0.2f, 1.0f, 0.5f); glLineWidth(2.5f);
        glBegin(GL_LINE_LOOP); glVertex2f(0.6f, 0); glVertex2f(-0.4f, 0.4f); glVertex2f(-0.2f, 0); glVertex2f(-0.4f, -0.4f); glEnd();
        if (gThrust) { glColor3f(1.0f, 0.5f, 0.1f); glBegin(GL_LINES); glVertex2f(-0.2f, 0); glVertex2f(-0.6f, 0); glEnd(); }
        glPopMatrix();
    }

    // Rocks
    for (auto& r : gRocks) drawVectorRock(r);

    // UFO
    for (auto& u : gUFOs) {
        glColor3f(1, 0, 0.5f); glLineWidth(2.5f); glPushMatrix(); glTranslatef(u.x, u.y, 0);
        glBegin(GL_LINE_LOOP); glVertex2f(-0.6f, 0); glVertex2f(0.6f, 0); glVertex2f(0.3f, 0.3f); glVertex2f(-0.3f, 0.3f); glEnd();
        glBegin(GL_LINE_LOOP); glVertex2f(-0.6f, 0); glVertex2f(-0.3f, -0.3f); glVertex2f(0.3f, -0.3f); glVertex2f(0.6f, 0); glEnd();
        glPopMatrix();
    }

    // Bullets
    glPointSize(4); glBegin(GL_POINTS);
    for (auto& b : gBullets) { glColor3f(b.r, b.g, b.b); glVertex2f(b.x, b.y); } glEnd();

    // Particles
    glPointSize(2); glBegin(GL_POINTS);
    for (auto& p : gParts) { glColor4f(p.r, p.g, p.b, p.life); glVertex2f(p.x, p.y); } glEnd();

    // UI
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIN_W, 0, WIN_H); glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    if (gState == STATE_PLAY) {
        char buf[64]; glColor3f(0, 1, 0.5f); snprintf(buf, 64, "SCORE: %06d", gScore); glRasterPos2f(30, WIN_H - 40); for (char* c = buf; *c; c++)glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
        snprintf(buf, 64, "LIVES: %d", gLives); glRasterPos2f(WIN_W / 2, WIN_H - 40); for (char* c = buf; *c; c++)glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
    }
    float cx = WIN_W * 0.5f;
    if (gState == STATE_HOME) {
        glEnable(GL_BLEND); glColor4f(0, 0, 0.1f, 0.8f); glRecti(0, 0, WIN_W, WIN_H);
        strokeCentered(cx, WIN_H * 0.65f, "NEO ASTEROIDS", 0.45f, 0, 0.8f, 1, 1);
        strokeCentered(cx, WIN_H * 0.5f, "VOID DRIFT", 0.3f, 1, 0, 0.5f, 1);
        glDisable(GL_BLEND);
    } else if (gState == STATE_GAMEOVER) {
        glEnable(GL_BLEND); glColor4f(0.1f, 0, 0, 0.85f); glRecti(0, 0, WIN_W, WIN_H);
        strokeCentered(cx, WIN_H * 0.6f, "VOID CONSUMED", 0.35f, 1, 0, 0, 1);
        glDisable(GL_BLEND);
    }
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glutSwapBuffers();
}

static void keyDown(unsigned char k, int, int) {
    if (k == 27) exit(0); if (k == 'p' || k == 'P') gPaused = !gPaused;
    if (k == ' ') {
        if (gState != STATE_PLAY) resetGame();
        else if (gFireTimer <= 0) {
            gFireTimer = 0.25f; float ar = gShip.ang * M_PI / 180.0f;
            Entity b; b.x = gShip.x + cos(ar) * 0.6f; b.y = gShip.y + sin(ar) * 0.6f; b.vx = cos(ar) * 20; b.vy = sin(ar) * 20; b.timer = 1.0f; b.alive = true; b.r = 1; b.g = 1; b.b = 0;
            gBullets.push_back(b); playSFX("../match3/match3 music/swap.wav");
        }
    }
}
static void specialDown(int k, int, int) {
    if (k == GLUT_KEY_UP) gThrust = true; if (k == GLUT_KEY_LEFT) gRotL = true; if (k == GLUT_KEY_RIGHT) gRotR = true;
}
static void specialUp(int k, int, int) {
    if (k == GLUT_KEY_UP) gThrust = false; if (k == GLUT_KEY_LEFT) gRotL = false; if (k == GLUT_KEY_RIGHT) gRotR = false;
}
int main(int argc, char** argv) {
    srand(time(0)); loadHS(); glutInit(&argc, argv); glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H); glutCreateWindow("Neo Asteroids"); glutFullScreen();
    glClearColor(0, 0, 0.02f, 1); glutDisplayFunc(display); glutKeyboardFunc(keyDown); glutSpecialFunc(specialDown); glutSpecialUpFunc(specialUp); glutTimerFunc(16, update, 0); glutMainLoop(); return 0;
}
