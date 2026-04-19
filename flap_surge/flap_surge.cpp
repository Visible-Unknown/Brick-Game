/*
 * ================================================================
 *  NEO FLAP-SURGE: VOLTAGE RUN  –  OpenGL / FreeGLUT [PRO EDITION]
 * ================================================================
 *  Brutal one-button flappy obstacle game with parallax backgrounds,
 *  pulsing neon gates, Pro Edition UI, audio, and particles.
 *
 *  CONTROLS: SPACE = Flap / Start / Restart, P = Pause, ESC = Quit
 *  BUILD: g++ flap_surge.cpp -o flap_surge.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
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
static const float W = 20.0f, H = 15.0f;
enum GameState { STATE_HOME, STATE_PLAY, STATE_GAMEOVER };

struct Obstacle {
    float x, gapY, gapSize;
    bool passed;
};
struct Particle { float x, y, vx, vy, life, r, g, b; };
enum PowerupType { PWR_SHIELD, PWR_CHRONOS, PWR_SURGE, PWR_NONE };
struct Powerup { float x, y; PowerupType type; bool active; };

static GameState gState = STATE_HOME;
static float gBirdY = 0, gBirdVY = 0;
static std::vector<Obstacle> gObs;
static std::vector<Particle> gParts;
static std::vector<Powerup> gPwrs;
static int gScore = 0, gHighScore = 0;
static float gGlobalTime = 0, gCamShake = 0, gWorldScroll = 0;
static bool gPaused = false;
static float gPipeSpeed = 5.0f;
static PowerupType gActivePower = PWR_NONE;
static float gPowerTimer = 0;
static float gManualSpeedMult = 1.0f;
const char* SAVE = "highscore.dat";

static float frand(float a, float b) { return a + (b - a) * (float)rand() / (float)RAND_MAX; }
static void saveHS() { FILE* f = fopen(SAVE, "w"); if (f) { fprintf(f, "%d", gHighScore); fclose(f); } }
static void loadHS() { FILE* f = fopen(SAVE, "r"); if (f) { if (fscanf(f, "%d", &gHighScore) != 1) gHighScore = 0; fclose(f); } }
static void spawnP(float px, float py, float r, float g, float b, int n) {
    for (int i = 0; i < n; i++) gParts.push_back({ px, py, frand(-3, 3), frand(-3, 3), 1.0f, r, g, b });
}

static void spawnObs(float x) {
    Obstacle o; o.x = x; o.gapY = frand(-H / 2 + 3, H / 2 - 3); o.gapSize = 4.5f; o.passed = false;
    gObs.push_back(o);
    if (frand(0, 1) < 0.2f) {
        PowerupType t = (PowerupType)(rand() % 3);
        gPwrs.push_back({ x + 4.0f, frand(-H / 2 + 2, H / 2 - 2), t, true });
    }
}

static void resetGame() {
    gScore = 0; gState = STATE_PLAY; gPaused = false; gBirdY = 0; gBirdVY = 0;
    gObs.clear(); gParts.clear(); gPwrs.clear(); gWorldScroll = 0; gPipeSpeed = 5.0f;
    gActivePower = PWR_NONE; gPowerTimer = 0; gManualSpeedMult = 1.0f;
    for (int i = 0; i < 4; i++) spawnObs(10 + i * 8);
    playBGM("../shooter/audio/bgm.mp3", true);
}

static void update(int) {
    float dt = 1.0f / 60.0f; gGlobalTime += dt;
    if (gState == STATE_PLAY && !gPaused) {
        float timeScale = (gActivePower == PWR_CHRONOS) ? 0.6f : 1.0f;
        gBirdVY -= 25.0f * dt * timeScale; gBirdY += gBirdVY * dt * timeScale;
        gWorldScroll += gPipeSpeed * dt * timeScale;
        gPipeSpeed = (5.0f + gScore * 0.05f) * gManualSpeedMult;

        if (gPowerTimer > 0) {
            gPowerTimer -= dt;
            if (gPowerTimer <= 0) gActivePower = PWR_NONE;
        }

        if (gBirdY > H / 2 || gBirdY < -H / 2) {
            if (gActivePower == PWR_SHIELD) {
                gActivePower = PWR_NONE; gPowerTimer = 0; gBirdVY = 5; playSFX("../match3/match3 music/special.wav");
            } else {
                playSFX("../tanks/tanks music/explosion.wav"); gCamShake = 1.0f; spawnP(0, gBirdY, 1, 0, 0, 30);
                gState = STATE_GAMEOVER; playBGM(0, false); if (gScore > gHighScore) { gHighScore = gScore; saveHS(); }
            }
        }

        // Powerups
        for (int i = (int)gPwrs.size() - 1; i >= 0; i--) {
            auto& p = gPwrs[i]; p.x -= gPipeSpeed * dt * timeScale;
            if (p.x < -W / 2 - 2) { gPwrs.erase(gPwrs.begin() + i); continue; }
            if (p.active && fabs(p.x) < 0.6f && fabs(p.y - gBirdY) < 0.6f) {
                p.active = false; gActivePower = p.type;
                gPowerTimer = (p.type == PWR_SURGE) ? 8.0f : 5.0f;
                playSFX("../match3/match3 music/special.wav"); spawnP(p.x, p.y, 1, 1, 1, 15);
            }
        }

        for (int i = (int)gObs.size() - 1; i >= 0; i--) {
            auto& o = gObs[i]; o.x -= gPipeSpeed * dt * timeScale;
            if (o.x < -W / 2 - 2) { gObs.erase(gObs.begin() + i); spawnObs(gObs.back().x + 8); continue; }
            if (!o.passed && o.x < 0) {
                o.passed = true; gScore += (gActivePower == PWR_SURGE) ? 2 : 1;
                playSFX("../match3/match3 music/swap.wav");
            }

            // Collision
            float birdX = 0, birdR = 0.4f;
            if (fabs(o.x - birdX) < 1.0f) {
                if (gBirdY + birdR > o.gapY + o.gapSize / 2 || gBirdY - birdR < o.gapY - o.gapSize / 2) {
                    if (gActivePower == PWR_SHIELD) {
                        gActivePower = PWR_NONE; gPowerTimer = 0; gCamShake = 0.5f; o.passed = true; 
                        playSFX("../match3/match3 music/special.wav"); spawnP(0, gBirdY, 0, 1, 1, 20);
                    } else {
                        playSFX("../tanks/tanks music/explosion.wav"); gCamShake = 1.0f; spawnP(0, gBirdY, 1, 0, 0, 30);
                        gState = STATE_GAMEOVER; playBGM(0, false); if (gScore > gHighScore) { gHighScore = gScore; saveHS(); }
                    }
                }
            }
        }
    }
    for (int i = (int)gParts.size() - 1; i >= 0; i--) { gParts[i].x += gParts[i].vx * dt; gParts[i].y += gParts[i].vy * dt; gParts[i].life -= dt; if (gParts[i].life <= 0) gParts.erase(gParts.begin() + i); }
    gCamShake *= 0.9f; glutPostRedisplay(); glutTimerFunc(16, update, 0);
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

    // Background Parallax
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for (int i = 0; i < 3; i++) {
        float p = (i + 1) * 0.3f; float off = fmod(gWorldScroll * p, 10.0f);
        glColor4f(0.1f, 0.2f, 0.4f, 0.2f / (i + 1));
        for (float x = -20 - off; x < 20; x += 10.0f) glRectf(x, -H / 2, x + 5, -H / 2 + 4 + i * 2);
    }
    glDisable(GL_BLEND);

    // Obstacles
    for (auto& o : gObs) {
        float r = 0, g = 0.8f, b = 1.0f; if (sin(gGlobalTime * 10 + o.x) > 0) { r = 0.5f; b = 1.0f; }
        glColor3f(r * 0.4f, g * 0.4f, b * 0.4f);
        glRectf(o.x - 0.8f, -H / 2, o.x + 0.8f, o.gapY - o.gapSize / 2); // Bottom
        glRectf(o.x - 0.8f, H / 2, o.x + 0.8f, o.gapY + o.gapSize / 2); // Top
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); glColor4f(r, g, b, 0.8f); glLineWidth(3);
        glBegin(GL_LINE_LOOP); glVertex2f(o.x - 0.8f, -H / 2); glVertex2f(o.x + 0.8f, -H / 2); glVertex2f(o.x + 0.8f, o.gapY - o.gapSize / 2); glVertex2f(o.x - 0.8f, o.gapY - o.gapSize / 2); glEnd();
        glRectf(o.x - 0.8f, H / 2, o.x + 0.8f, o.gapY + o.gapSize / 2); // Top
        glDisable(GL_BLEND);
    }

    // Powerups
    for (auto& p : gPwrs) {
        if (!p.active) continue;
        float r = 0, g = 1, b = 1; if (p.type == PWR_CHRONOS) { r = 0.8; g = 0.2; b = 1; } else if (p.type == PWR_SURGE) { r = 1; g = 0.8; b = 0; }
        glPushMatrix(); glTranslatef(p.x, p.y, 0); glRotatef(gGlobalTime * 100, 0, 0, 1);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); glColor4f(r, g, b, 0.8f);
        glBegin(GL_LINE_LOOP); for (int i = 0; i < 6; i++) { float a = i * 2 * M_PI / 6; glVertex2f(cos(a) * 0.4f, sin(a) * 0.4f); } glEnd();
        glColor4f(r, g, b, 0.3f); glBegin(GL_TRIANGLE_FAN); glVertex2f(0, 0); for (int i = 0; i <= 6; i++) { float a = i * 2 * M_PI / 6; glVertex2f(cos(a) * 0.3f, sin(a) * 0.3f); } glEnd();
        glDisable(GL_BLEND); glPopMatrix();
    }

    // Bird
    if (gState != STATE_GAMEOVER) {
        glPushMatrix(); glTranslatef(0, gBirdY, 0); glRotatef(gBirdVY * 2, 0, 0, 1);
        if (gActivePower != PWR_NONE) {
            float r = 0, g = 1, b = 1; if (gActivePower == PWR_CHRONOS) { r = 0.8; g = 0.2; b = 1; } else if (gActivePower == PWR_SURGE) { r = 1; g = 0.8; b = 0; }
            glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glColor4f(r, g, b, 0.4f + 0.2f * sin(gGlobalTime * 10)); glutSolidSphere(0.6f, 15, 15);
            glDisable(GL_BLEND);
        }
        glColor3f(1, 1, 0); glBegin(GL_TRIANGLE_FAN); for (int i = 0; i < 20; i++) { float a = i * 2 * M_PI / 20; glVertex2f(cos(a) * 0.4f, sin(a) * 0.4f); } glEnd();
        glColor3f(1, 0.5f, 0); glBegin(GL_TRIANGLES); glVertex2f(0.3f, 0); glVertex2f(0.6f, 0); glVertex2f(0.3f, -0.2f); glEnd(); // Beak
        glColor3f(1, 1, 1); glPointSize(6); glBegin(GL_POINTS); glVertex2f(0.15f, 0.15f); glEnd(); // Eye
        glPopMatrix();
    }

    // Particles
    glPointSize(3); glBegin(GL_POINTS);
    for (auto& p : gParts) { glColor4f(p.r, p.g, p.b, p.life); glVertex2f(p.x, p.y); } glEnd();

    // UI
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0, WIN_W, 0, WIN_H); glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    if (gState == STATE_PLAY) {
        char buf[64]; glColor3f(0, 1, 0.5f); snprintf(buf, 64, "SCORE: %06d", gScore); glRasterPos2f(30, WIN_H - 40); for (char* c = buf; *c; c++)glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, *c);
        
        // Voltage Indicator
        int volt = (int)(gManualSpeedMult * 100);
        snprintf(buf, 64, "VOLTAGE: %d%%", volt);
        if (volt > 150) glColor3f(1, 0.2f, 0.2f); // Overdrive Red
        else if (volt < 80) glColor3f(0.2f, 0.6f, 1); // Low Voltage Blue
        else glColor3f(0, 1, 1); // Normal Cyan
        glRasterPos2f(WIN_W - 200, WIN_H - 40);
        for (char* c = buf; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

        if (gActivePower != PWR_NONE) {
            float r = 0, g = 1, b = 1; const char* pnm = "SHIELD";
            if (gActivePower == PWR_CHRONOS) { r = 0.8; g = 0.2; b = 1; pnm = "CHRONOS"; }
            else if (gActivePower == PWR_SURGE) { r = 1; g = 0.8; b = 0; pnm = "SURGE"; }
            glEnable(GL_BLEND); glColor4f(r, g, b, 0.8f); glRectf(30, WIN_H - 70, 30 + gPowerTimer * 20, WIN_H - 60);
            glRasterPos2f(30, WIN_H - 90); for (const char* c = pnm; *c; c++)glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
            glDisable(GL_BLEND);
        }
    }
    float cx = WIN_W * 0.5f;
    if (gState == STATE_HOME) {
        glEnable(GL_BLEND); glColor4f(0, 0.05f, 0.1f, 0.88f); glRecti(0, 0, WIN_W, WIN_H);
        strokeCentered(cx, WIN_H * 0.68f, "NEO FLAP-SURGE", 0.48f, 0, 0.8f, 1, 1);
        strokeCentered(cx, WIN_H * 0.58f, "VOLTAGE RUN", 0.32f, 1, 1, 0, 1);
        
        // Decorative Divider
        glLineWidth(2.0f); glColor4f(0, 0.5f, 1, 0.4f);
        glBegin(GL_LINES); glVertex2f(cx-200, WIN_H*0.53f); glVertex2f(cx+200, WIN_H*0.53f); glEnd();
        
        char hs[64]; snprintf(hs, 64, "PERSONAL BEST: %06d", gHighScore);
        strokeCentered(cx, WIN_H * 0.42f, hs, 0.18f, 0, 1, 0.5, 1);
        
        float blink = 0.5f + 0.5f * sinf(gGlobalTime * 4.0f);
        strokeCentered(cx, WIN_H * 0.25f, "PRESS SPACE TO START ENGINE", 0.20f, 0, 1, 0.8f, blink);
        
        // Developer Credit
        strokeCentered(cx, WIN_H * 0.10f, "DEVELOPED BY AL AMIN HOSSAIN", 0.12f, 0.5f, 0.5f, 0.5f, 0.7f);
        glDisable(GL_BLEND);
    } else if (gState == STATE_GAMEOVER) {
        glEnable(GL_BLEND); glColor4f(0.12f, 0, 0, 0.88f); glRecti(0, 0, WIN_W, WIN_H);
        strokeCentered(cx, WIN_H * 0.65f, "CIRCUIT BROKEN", 0.35f, 1, 0.2f, 0, 1);
        char sbuf[64], hsbuf[64];
        snprintf(sbuf, 64, "OUTPUT: %06d", gScore);
        snprintf(hsbuf, 64, "SYSTEM PEAK: %06d", gHighScore);
        strokeCentered(cx, WIN_H * 0.50f, sbuf, 0.25f, 1, 1, 1, 1);
        strokeCentered(cx, WIN_H * 0.42f, hsbuf, 0.20f, 0, 1, 1, 1);
        strokeCentered(cx, WIN_H * 0.20f, "[ PRESS SPACE TO REBOOT ]", 0.15f, 0.5, 0.5, 0.5, 1);
        glDisable(GL_BLEND);
    }
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glutSwapBuffers();
}

static void keyDown(unsigned char k, int, int) {
    if (k == 27) exit(0); if (k == 'p' || k == 'P') gPaused = !gPaused;
    if (k == '+' || k == '=') { gManualSpeedMult += 0.1f; if (gManualSpeedMult > 3.0f) gManualSpeedMult = 3.0f; playSFX("../match3/match3 music/swap.wav"); }
    if (k == '-' || k == '_') { gManualSpeedMult -= 0.1f; if (gManualSpeedMult < 0.3f) gManualSpeedMult = 0.3f; playSFX("../match3/match3 music/swap.wav"); }
    if (k == ' ') {
        if (gState != STATE_PLAY) resetGame();
        else { gBirdVY = 8.5f; playSFX("../match3/match3 music/swap.wav"); }
    }
}
int main(int argc, char** argv) {
    srand(time(0)); loadHS(); glutInit(&argc, argv); glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H); glutCreateWindow("Neo Flap-Surge"); glutFullScreen();
    glClearColor(0.01f, 0.01f, 0.08f, 1); glutDisplayFunc(display); glutKeyboardFunc(keyDown); glutTimerFunc(16, update, 0); glutMainLoop(); return 0;
}
