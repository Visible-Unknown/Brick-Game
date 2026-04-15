/*
 * ================================================================
 *  NEO BOUNCE  –  OpenGL / FreeGLUT [PRO EDITION]
 * ================================================================
 *
 *  FEATURES
 *  ─────────────────────────────────────────────────────────────
 *  • Physics platformer inspired by the Nokia classic
 *  • Inflate/Deflate ball mechanics
 *  • Ring checkpoints to unlock level exit
 *  • Procedural level generation with hazards
 *  • Power-ups: Magnet, Super Bounce, Time Freeze
 *  • Pro Edition UI, audio, particles, camera shake
 *
 *  CONTROLS
 *  ─────────────────────────────────────────────────────────────
 *  Left / Right       Move ball
 *  Up / SPACE         Jump (hold for higher)
 *  Z                  Inflate (float up, break glass)
 *  X                  Deflate (shrink, squeeze through gaps)
 *  P                  Pause / Resume
 *  SPACE              Start / Restart (on menus)
 *  ESC                Quit
 *
 *  BUILD (Windows / MinGW)
 *  ─────────────────────────────────────────────────────────────
 *    g++ bounce.cpp -o bounce.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
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
//  Audio Engine
// ──────────────────────────────────────────────────────────────
static void playBGM(const char* f, bool loop) {
    char cmd[512]; snprintf(cmd,512,"close bgm"); mciSendStringA(cmd,0,0,0);
    if(!f) return;
    snprintf(cmd,512,"open \"%s\" type mpegvideo alias bgm",f);
    if(mciSendStringA(cmd,0,0,0)==0){ snprintf(cmd,512,"play bgm %s",loop?"repeat":""); mciSendStringA(cmd,0,0,0); }
}
static void playSFX(const char* f){ PlaySoundA(f,NULL,SND_ASYNC|SND_FILENAME|SND_NODEFAULT); }

// ──────────────────────────────────────────────────────────────
//  Constants & Types
// ──────────────────────────────────────────────────────────────
static int WIN_W=1280, WIN_H=720;
static const float WORLD_W=30.0f, WORLD_H=20.0f;

enum GameState { STATE_HOME, STATE_PLAY, STATE_GAMEOVER, STATE_WIN };

struct Platform {
    float x, y, w, h;
    int type; // 0=solid, 1=spike, 2=glass (breakable when inflated)
    bool alive;
    float r,g,b;
};

struct Ring {
    float x, y;
    bool collected;
    float pulse;
};

struct PowerUpItem {
    float x, y;
    int type; // 0=magnet, 1=super bounce, 2=freeze
    float pulse;
    bool alive;
};

struct Particle { float x,y,vx,vy,life,r,g,b; };

// ──────────────────────────────────────────────────────────────
//  Game State
// ──────────────────────────────────────────────────────────────
static GameState gState = STATE_HOME;
static float gBallX=0, gBallY=2.0f;
static float gBallVX=0, gBallVY=0;
static float gBallRadius = 0.5f;
static float gTargetRadius = 0.5f; // Normal size
static bool gInflated = false, gDeflated = false;
static bool gOnGround = false;

static float gGlobalTime=0, gCameraShake=0, gFlash=0;
static float gCamX=0, gCamY=0; // Camera follows ball
static int gScore=0, gHighScore=0;
static int gLives=3;
static int gLevel=1;
static int gRingsTotal=0, gRingsCollected=0;
static bool gExitOpen=false;
static bool gPaused=false;
static bool gMoveLeft=false, gMoveRight=false, gJumpHeld=false;
static bool gNewHighScore=false;

static float gFreezeTimer=0, gSuperBounceTimer=0;

static std::vector<Platform> gPlatforms;
static std::vector<Ring> gRings;
static std::vector<PowerUpItem> gPowerUps;
static std::vector<Particle> gParticles;
static float gExitX=0, gExitY=0;

const char* SAVE_FILE = "highscore.dat";

// ──────────────────────────────────────────────────────────────
//  Helpers
// ──────────────────────────────────────────────────────────────
static float frand(float lo, float hi){ return lo+(hi-lo)*(float)rand()/(float)RAND_MAX; }
static void saveHS(){
    FILE* f=fopen(SAVE_FILE,"w");
    if(f){ fprintf(f,"%d",gHighScore); fclose(f); }
}
static void loadHS(){
    FILE* f=fopen(SAVE_FILE,"r");
    if(f){ if(fscanf(f,"%d",&gHighScore)!=1) gHighScore=0; fclose(f); }
}
static void spawnParticles(float px,float py,float r,float g,float b,int n){
    for(int i=0;i<n;i++){Particle p;p.x=px;p.y=py;p.vx=frand(-3,3);p.vy=frand(-3,3);p.life=1;p.r=r;p.g=g;p.b=b;gParticles.push_back(p);}
}

static void buildLevel(int level){
    gPlatforms.clear(); gRings.clear(); gPowerUps.clear(); gParticles.clear();
    
    // Ground
    Platform ground; ground.x=0; ground.y=-8; ground.w=60; ground.h=1; ground.type=0; ground.alive=true; ground.r=0.15f; ground.g=0.4f; ground.b=0.6f;
    gPlatforms.push_back(ground);
    
    // Walls
    Platform lwall; lwall.x=-15; lwall.y=0; lwall.w=1; lwall.h=30; lwall.type=0; lwall.alive=true; lwall.r=0.1f; lwall.g=0.3f; lwall.b=0.5f;
    gPlatforms.push_back(lwall);
    Platform rwall; rwall.x=15+level*8.0f; rwall.y=0; rwall.w=1; rwall.h=30; rwall.type=0; rwall.alive=true; rwall.r=0.1f; rwall.g=0.3f; rwall.b=0.5f;
    gPlatforms.push_back(rwall);
    
    // Generate platforms procedurally
    int numPlats = 8 + level * 4;
    for(int i=0; i<numPlats; i++){
        Platform p;
        p.w = frand(2.0f, 5.0f);
        p.h = frand(0.3f, 0.6f);
        p.x = frand(-12, 12 + level*6.0f);
        p.y = frand(-6, 8);
        int roll = rand()%100;
        if(roll < 10+level*3) { p.type=1; p.r=1; p.g=0.15f; p.b=0.1f; } // spike
        else if(roll < 25+level*2) { p.type=2; p.r=0.4f; p.g=0.8f; p.b=1.0f; } // glass
        else { p.type=0; p.r=0.1f; p.g=0.5f; p.b=0.8f; }
        p.alive=true;
        gPlatforms.push_back(p);
    }
    
    // Rings
    int numRings = 3 + level;
    if(numRings > 8) numRings = 8;
    gRingsTotal = numRings; gRingsCollected = 0; gExitOpen = false;
    for(int i=0; i<numRings; i++){
        Ring r; r.x = frand(-10, 10+level*5.0f); r.y = frand(-4, 6); r.collected=false; r.pulse=0;
        gRings.push_back(r);
    }
    
    // Power-ups
    int numPU = 1 + level/2;
    for(int i=0;i<numPU;i++){
        PowerUpItem pu; pu.x=frand(-10,10+level*4.0f); pu.y=frand(-3, 5); pu.type=rand()%3; pu.pulse=0; pu.alive=true;
        gPowerUps.push_back(pu);
    }
    
    // Exit gate
    gExitX = 10 + level*6.0f; gExitY = -6;
    
    // Ball start
    gBallX = -10; gBallY = -5; gBallVX=0; gBallVY=0;
    gInflated=false; gDeflated=false; gTargetRadius=0.5f; gBallRadius=0.5f;
}

static void resetGame(){
    gScore=0; gLives=3; gLevel=1; gNewHighScore=false;
    gState=STATE_PLAY; gPaused=false; gCameraShake=0; gFlash=1.0f;
    gFreezeTimer=0; gSuperBounceTimer=0;
    buildLevel(gLevel);
    playBGM("../match3/match3 music/bgm_play.mp3", true);
}

static void die(){
    playSFX("../tanks/tanks music/explosion.wav");
    gLives--; gCameraShake=1.0f; gFlash=0.8f;
    spawnParticles(gBallX,gBallY,1,0.2f,0.1f,30);
    if(gLives<=0){
        gState=STATE_GAMEOVER; playBGM(nullptr,false);
        if(gScore>gHighScore){ gHighScore=gScore; saveHS(); }
    }
    else { gBallX=-10; gBallY=-5; gBallVX=0; gBallVY=0; gInflated=false; gDeflated=false; gTargetRadius=0.5f; }
}

// ──────────────────────────────────────────────────────────────
//  Update
// ──────────────────────────────────────────────────────────────
static void update(int){
    float dt=1.0f/60.0f;
    gGlobalTime+=dt;
    
    if(gState==STATE_PLAY && !gPaused){
        if(gFreezeTimer>0) gFreezeTimer-=dt;
        if(gSuperBounceTimer>0) gSuperBounceTimer-=dt;
        
        if(gScore > gHighScore){
            bool wasFirstRecord = (gHighScore == 0);
            gHighScore = gScore;
            saveHS(); // Save every time record is broken
            if(!gNewHighScore && !wasFirstRecord){
                gNewHighScore = true;
                spawnParticles(gBallX, gBallY, 1, 1, 0, 50);
                playSFX("../racing/audio/pickup.wav");
            }
        }
        
        float effectiveDt = (gFreezeTimer>0) ? dt*0.3f : dt;
        
        // Inflate/Deflate radius
        if(gInflated) gTargetRadius = 0.9f;
        else if(gDeflated) gTargetRadius = 0.25f;
        else gTargetRadius = 0.5f;
        gBallRadius += (gTargetRadius - gBallRadius) * 0.15f;
        
        // Horizontal movement
        float moveSpd = gDeflated ? 6.0f : (gInflated ? 4.0f : 8.0f);
        if(gMoveLeft) gBallVX = -moveSpd;
        else if(gMoveRight) gBallVX = moveSpd;
        else gBallVX *= 0.85f;
        
        // Gravity (reduced when inflated)
        float gravity = gInflated ? -5.0f : -18.0f;
        gBallVY += gravity * effectiveDt;
        if(gInflated && gBallVY < -2.0f) gBallVY = -2.0f; // Float gently
        
        // Jump
        if(gJumpHeld && gOnGround) {
            float jumpForce = (gSuperBounceTimer > 0) ? 14.0f : 9.0f;
            gBallVY = jumpForce;
            gOnGround = false;
            playSFX("../match3/match3 music/swap.wav");
        }
        
        gBallX += gBallVX * effectiveDt;
        gBallY += gBallVY * effectiveDt;
        
        // Platform collision
        gOnGround = false;
        for(auto& p : gPlatforms){
            if(!p.alive) continue;
            float left=p.x-p.w/2, right=p.x+p.w/2, top=p.y+p.h/2, bot=p.y-p.h/2;
            
            // Glass breaking
            if(p.type==2 && gInflated){
                if(gBallX+gBallRadius>left && gBallX-gBallRadius<right && gBallY-gBallRadius<top && gBallY+gBallRadius>bot){
                    p.alive=false; gScore+=50;
                    spawnParticles(p.x,p.y,0.4f,0.8f,1.0f,15);
                    playSFX("../match3/match3 music/explosion.wav");
                    continue;
                }
            }
            
            // Top collision (landing)
            if(gBallX+gBallRadius*0.8f>left && gBallX-gBallRadius*0.8f<right){
                if(gBallY-gBallRadius < top && gBallY-gBallRadius > top-0.5f && gBallVY<=0){
                    if(p.type==1) { die(); break; } // Spike!
                    gBallY = top + gBallRadius;
                    float bounce = (gSuperBounceTimer > 0) ? -gBallVY*0.5f : 0;
                    gBallVY = bounce;
                    gOnGround = true;
                }
            }
            // Bottom collision (hitting head)
            if(gBallX+gBallRadius*0.8f>left && gBallX-gBallRadius*0.8f<right){
                if(gBallY+gBallRadius > bot && gBallY+gBallRadius < bot+0.5f && gBallVY>0){
                    gBallY = bot - gBallRadius;
                    gBallVY = 0;
                }
            }
            // Side collisions
            if(gBallY+gBallRadius*0.5f>bot && gBallY-gBallRadius*0.5f<top){
                if(gBallX+gBallRadius>left && gBallX<p.x && gBallVX>0){ gBallX=left-gBallRadius; gBallVX=0; }
                if(gBallX-gBallRadius<right && gBallX>p.x && gBallVX<0){ gBallX=right+gBallRadius; gBallVX=0; }
            }
        }
        
        // Ring collection
        for(auto& r : gRings){
            if(r.collected) continue;
            float dx=gBallX-r.x, dy=gBallY-r.y;
            if(dx*dx+dy*dy < 1.5f){
                r.collected=true; gRingsCollected++; gScore+=200;
                spawnParticles(r.x,r.y,1,1,0,15);
                playSFX("../racing/audio/pickup.wav");
                if(gRingsCollected>=gRingsTotal) gExitOpen=true;
            }
        }
        
        // Power-up collection
        for(auto& pu : gPowerUps){
            if(!pu.alive) continue;
            float dx=gBallX-pu.x, dy=gBallY-pu.y;
            if(dx*dx+dy*dy < 1.5f){
                pu.alive=false; gScore+=100;
                playSFX("../tanks/tanks music/powerup.wav");
                if(pu.type==0){ spawnParticles(pu.x,pu.y,1,0.5f,1,15); } // Magnet (attract rings - visual only)
                else if(pu.type==1){ gSuperBounceTimer=12.0f; spawnParticles(pu.x,pu.y,0.2f,1,0.2f,15); }
                else { gFreezeTimer=10.0f; spawnParticles(pu.x,pu.y,0.2f,0.6f,1,15); }
            }
        }
        
        // Exit gate
        if(gExitOpen){
            float dx=gBallX-gExitX, dy=gBallY-gExitY;
            if(dx*dx+dy*dy < 2.0f){
                gScore += 1000 + gLevel*500;
                gLevel++; gCameraShake=0.5f;
                if(gLevel > 10) { gState=STATE_WIN; playBGM(nullptr,false); if(gScore>gHighScore){gHighScore=gScore; saveHS();} }
                else buildLevel(gLevel);
            }
        }
        
        // Death by falling far below
        if(gBallY < -15) die();
    }
    
    // Camera
    gCamX += (gBallX - gCamX) * 0.08f;
    gCamY += (gBallY - gCamY) * 0.08f;
    
    // Particles
    for(auto& p:gParticles){p.x+=p.vx*0.016f;p.y+=p.vy*0.016f;p.vy-=5*0.016f;p.life-=0.016f*1.5f;}
    gParticles.erase(std::remove_if(gParticles.begin(),gParticles.end(),[](const Particle& p){return p.life<=0;}),gParticles.end());
    gCameraShake*=0.85f;
    if(gFlash>0) gFlash-=dt*2.0f;
    glutPostRedisplay(); glutTimerFunc(16,update,0);
}

// ──────────────────────────────────────────────────────────────
//  UI Utilities
// ──────────────────────────────────────────────────────────────
static void bitmapAt(float px,float py,const char* s,void* f=GLUT_BITMAP_HELVETICA_18){ glRasterPos2f(px,py); for(const char* c=s;*c;c++) glutBitmapCharacter(f,*c); }
static int bitmapWidth(const char* s,void* f=GLUT_BITMAP_HELVETICA_18){ int w=0; for(const char* c=s;*c;c++) w+=glutBitmapWidth(f,*c); return w; }
static void bitmapCentered(float cx,float py,const char* s,void* f=GLUT_BITMAP_HELVETICA_18){ bitmapAt(cx-bitmapWidth(s,f)*0.5f,py,s,f); }
static void strokeCentered(float cx,float cy,const std::string& s,float scale,float r,float g,float b,float a){
    float tw=0; for(char c:s) tw+=glutStrokeWidth(GLUT_STROKE_ROMAN,c);
    float sx=cx-tw*scale*0.5f;
    glEnable(GL_BLEND);
    // Glow pass
    glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glColor4f(r,g,b,a*0.35f); glLineWidth(6.0f);
    glPushMatrix(); glTranslatef(sx,cy,0); glScalef(scale,scale,scale); for(char c:s) glutStrokeCharacter(GLUT_STROKE_ROMAN,c); glPopMatrix();
    // Sharp pass
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,1,a); glLineWidth(2.5f);
    glPushMatrix(); glTranslatef(sx,cy,0); glScalef(scale,scale,scale); for(char c:s) glutStrokeCharacter(GLUT_STROKE_ROMAN,c); glPopMatrix();
    glLineWidth(1); glDisable(GL_BLEND);
}

// ──────────────────────────────────────────────────────────────
//  Rendering
// ──────────────────────────────────────────────────────────────
static void drawScene(){
    // Atmospheric Fog
    glEnable(GL_FOG);
    GLfloat fogCol[]={0.01f,0.02f,0.05f,1};
    glFogfv(GL_FOG_COLOR,fogCol); glFogi(GL_FOG_MODE,GL_LINEAR);
    glFogf(GL_FOG_START,15.0f); glFogf(GL_FOG_END,45.0f);

    // Neural Grid (Scrolling with camera)
    glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glColor4f(0.0f,0.4f,1.0f,0.15f); glLineWidth(1);
    float gx=floorf(gCamX/2.0f)*2.0f, gy=floorf(gCamY/2.0f)*2.0f;
    glBegin(GL_LINES);
    for(float x=gx-30;x<=gx+30;x+=2){ glVertex3f(x,gy-20,-2); glVertex3f(x,gy+20,-2); }
    for(float y=gy-20;y<=gy+20;y+=2){ glVertex3f(gx-30,y,-2); glVertex3f(gx+30,y,-2); }
    glEnd(); glDisable(GL_BLEND);
    
    glEnable(GL_LIGHTING);
    
    // Platforms (Structural Rails with Neon Edges)
    for(auto& p:gPlatforms){
        if(!p.alive) continue;
        // 1. Dark Base
        GLfloat d[]={p.r*0.25f,p.g*0.25f,p.b*0.25f,1}; GLfloat a[]={0.02f,0.05f,0.1f,1};
        if(p.type==2){d[3]=0.4f; a[3]=0.4f; glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);}
        glMaterialfv(GL_FRONT,GL_DIFFUSE,d); glMaterialfv(GL_FRONT,GL_AMBIENT,a);
        glPushMatrix(); glTranslatef(p.x,p.y,0); glScalef(p.w,p.h,0.8f); glutSolidCube(1); glPopMatrix();
        if(p.type==2) glDisable(GL_BLEND);

        // 2. Neon Edge Glow
        glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        float pulse = 0.6f + 0.4f*sinf(gGlobalTime*3 + p.x);
        glColor4f(p.r,p.g,p.b,pulse*0.7f); glLineWidth(2.5f);
        glPushMatrix(); glTranslatef(p.x,p.y,0); glScalef(p.w,p.h,0.8f); glutWireCube(1.02f); glPopMatrix();
        
        // 3. Hazard indicators (Hard-Light)
        if(p.type==1){
            int ns = (int)(p.w / 0.8f);
            for(int s=0;s<ns;s++){
                float sx=p.x-p.w/2+0.4f+s*0.8f;
                float hp = 0.7f+0.3f*sinf(gGlobalTime*10+s);
                glColor4f(1,0.2f,0,hp);
                glBegin(GL_TRIANGLES); glVertex3f(sx-0.2f,p.y+p.h/2,0.4f); glVertex3f(sx+0.2f,p.y+p.h/2,0.4f); glVertex3f(sx,p.y+p.h/2+0.6f,0.4f); glEnd();
            }
        }
        glDisable(GL_BLEND); glEnable(GL_LIGHTING);
    }
    
    // Rings (Data Spheres)
    for(auto& r:gRings){
        if(r.collected) continue;
        r.pulse+=0.06f;
        glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        float p=0.4f+0.3f*sinf(r.pulse*2);
        
        // Outer Shields
        glColor4f(0,0.8f,1,p); glLineWidth(2);
        glPushMatrix(); glTranslatef(r.x,r.y,0); glRotatef(r.pulse*40,0,1,1); glutWireTorus(0.04f,0.6f,8,16); glPopMatrix();
        glPushMatrix(); glTranslatef(r.x,r.y,0); glRotatef(-r.pulse*60,1,0,1); glutWireTorus(0.04f,0.45f,8,16); glPopMatrix();
        
        // Inner Core
        glColor4f(1,1,1,0.8f); glPushMatrix(); glTranslatef(r.x,r.y,0); glutSolidSphere(0.12f,8,8); glPopMatrix();
        glDisable(GL_BLEND); glEnable(GL_LIGHTING);
    }
    
    // Exit Portal (Dimensional Vortex)
    if(gExitOpen){
        glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        float t = gGlobalTime*4;
        for(int i=0;i<3;i++){
            float s = 2.0f - i*0.5f;
            float r = (i==0?0:0), g=(i==1?1:0.5f), b=1.0f;
            glColor4f(r,g,b,0.4f/(i+1));
            glPushMatrix(); glTranslatef(gExitX,gExitY,0); glRotatef(t*(i+1),0,0,1); glRotatef(t*0.5f,1,1,0);
            glLineWidth(3); glutWireCube(s);
            glPopMatrix();
        }
        glDisable(GL_BLEND); glEnable(GL_LIGHTING);
    } else {
        glDisable(GL_LIGHTING); glColor3f(0.2f,0.2f,0.2f);
        glPushMatrix(); glTranslatef(gExitX,gExitY,0); glLineWidth(1.5f); glutWireCube(2.0f); glPopMatrix();
        glEnable(GL_LIGHTING);
    }
    
    // Power-Ups (Glow Octahedrons)
    for(auto& pu:gPowerUps){
        if(!pu.alive) continue;
        pu.pulse+=0.05f;
        float pr=1,pg=1,pb=1;
        if(pu.type==0){pr=1;pg=0.4f;pb=1;} else if(pu.type==1){pr=0.2f;pg=1;pb=0.4f;} else {pr=0.2f;pg=0.8f;pb=1;}
        
        glDisable(GL_COLOR_MATERIAL);
        GLfloat d[]={pr*0.9f,pg*0.9f,pb*0.9f,1}; GLfloat a[]={pr*0.2f,pg*0.2f,pb*0.2f,1};
        glMaterialfv(GL_FRONT,GL_DIFFUSE,d); glMaterialfv(GL_FRONT,GL_AMBIENT,a);
        float hover=0.25f*sinf(pu.pulse*1.5f);
        glPushMatrix(); glTranslatef(pu.x,pu.y+hover,0.5f); glRotatef(pu.pulse*70,0,1,0); 
        glutSolidOctahedron();
        glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        glColor4f(pr,pg,pb,0.5f); glutWireOctahedron();
        glDisable(GL_BLEND); glEnable(GL_LIGHTING);
        glPopMatrix();
    }
    
    // Ball (Reactor Core)
    if(gState==STATE_PLAY){
        float br=1,bg=0.3f,bb=0.1f;
        if(gInflated){br=1;bg=0.7f;bb=0.9f;}
        if(gDeflated){br=0.4f;bg=0.5f;bb=1.0f;}
        
        glPushMatrix(); glTranslatef(gBallX,gBallY,0.5f);
        
        // 1. Inner Energy Core
        glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        glColor4f(br,bg,bb,0.8f); glutSolidSphere(gBallRadius*0.7f,16,16);
        
        // 2. Translucent Shell
        glColor4f(br,bg,bb,0.3f); glutSolidSphere(gBallRadius,16,16);
        
        // 3. Stabilization Rings
        glLineWidth(2); glColor4f(1,1,1,0.6f);
        glPushMatrix(); glRotatef(gGlobalTime*100,0,1,0); glutWireTorus(0.02f,gBallRadius+0.1f,8,16); glPopMatrix();
        glPushMatrix(); glRotatef(gGlobalTime*80,1,0,0); glutWireTorus(0.02f,gBallRadius+0.15f,8,16); glPopMatrix();
        
        glDisable(GL_BLEND); glEnable(GL_LIGHTING);
        glPopMatrix();
    }
    
    // Particles (Glowing trail/splatter)
    glDisable(GL_LIGHTING); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE); glPointSize(4);
    glBegin(GL_POINTS); for(auto& p:gParticles){glColor4f(p.r,p.g,p.b,p.life);glVertex3f(p.x,p.y,0.5f);} glEnd();
    glDisable(GL_BLEND); glDisable(GL_FOG);
}

static void drawHUD(){
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity(); glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    
    // Console Bar
    glColor4f(0,0.02f,0.1f,0.85f); glRecti(0,WIN_H-65,WIN_W,WIN_H);
    glColor4f(0,0.6f,1.0f,0.5f); glLineWidth(2); glBegin(GL_LINES); glVertex2f(0,WIN_H-65); glVertex2f(WIN_W,WIN_H-65); glEnd();
    
    // Scanlines
    glColor4f(1,1,1,0.03f); glLineWidth(1); glBegin(GL_LINES); 
    for(int y=WIN_H-65; y<WIN_H; y+=3){ glVertex2f(0,y); glVertex2f(WIN_W,y); }
    glEnd();

    char buf[128]; glColor3f(1,1,1);
    snprintf(buf,128,"SCORE: %06d",gScore); bitmapAt(30,WIN_H-42,buf,GLUT_BITMAP_TIMES_ROMAN_24);
    
    glColor3f(0.8f,0.8f,0.8f);
    snprintf(buf,128,"LV: %d  LIVES: %d  RINGS: %d/%d",gLevel,gLives,gRingsCollected,gRingsTotal); 
    bitmapCentered(WIN_W*0.5f,WIN_H-42,buf,GLUT_BITMAP_HELVETICA_18);
    
    glColor3f(1,0.8f,0); snprintf(buf,128,"PEAK: %06d",gHighScore); 
    bitmapAt(WIN_W-220,WIN_H-42,buf,GLUT_BITMAP_TIMES_ROMAN_24);

    if(gNewHighScore){
        float p = 0.5f+0.5f*sinf(gGlobalTime*10);
        glColor4f(1,1,0,p); bitmapCentered(WIN_W*0.5f,WIN_H-85,"!! NEW RECORD !!",GLUT_BITMAP_HELVETICA_18);
    }

    if(gSuperBounceTimer>0){glColor3f(0.2f,1,0.2f);snprintf(buf,128,"SUPER BOUNCE: %.1fs",gSuperBounceTimer);bitmapCentered(WIN_W*0.5f,WIN_H-110,buf,GLUT_BITMAP_HELVETICA_18);}
    if(gFreezeTimer>0){glColor3f(0.2f,0.6f,1);snprintf(buf,128,"TIME FREEZE: %.1fs",gFreezeTimer);bitmapCentered(WIN_W*0.5f,WIN_H-130,buf,GLUT_BITMAP_HELVETICA_18);}
    if(gExitOpen){glColor3f(1,0.4f,0); bitmapCentered(WIN_W*0.5f,WIN_H-150,">> PORTAL ACTIVE <<",GLUT_BITMAP_TIMES_ROMAN_24);}
    
    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

static void drawOverlay(){
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity(); glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING);
    float cx=WIN_W*0.5f;
    if(gState==STATE_HOME){
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.01f,0.01f,0.05f,0.92f); glRecti(0,0,WIN_W,WIN_H);
        float p=0.8f+0.2f*sinf(gGlobalTime*3);
        strokeCentered(cx,WIN_H*0.70f,"NEO BOUNCE",0.55f,0,0.6f,1,p);
        strokeCentered(cx,WIN_H*0.58f,"REACTOR CORE",0.30f,0.4f,1,0.8f,p);
        
        char hb[64]; snprintf(hb,64,"PERSONAL BEST: %06d",gHighScore);
        glColor3f(1,0.8f,0); bitmapCentered(cx,WIN_H*0.42f,hb,GLUT_BITMAP_HELVETICA_18);

        float blink=0.5f+0.5f*sinf(gGlobalTime*6); glColor4f(1,1,1,blink);
        bitmapCentered(cx,WIN_H*0.28f,"[ PRESS SPACE TO INITIALIZE ]",GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.5f,0.5f,0.5f);
        bitmapCentered(cx,WIN_H*0.18f,"Arrows=Move  |  Space=Jump  |  Z/X=Inflate/Deflate",GLUT_BITMAP_HELVETICA_12);
        glDisable(GL_BLEND);
    } else if(gState==STATE_GAMEOVER){
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.08f,0,0.02f,0.95f); glRecti(0,0,WIN_W,WIN_H);
        strokeCentered(cx,WIN_H*0.7f,"CORE MELTDOWN",0.35f,1,0.2f,0,1);
        char buf[128]; glColor3f(1,0.8f,0); snprintf(buf,128,"FINAL SCORE: %06d",gScore); bitmapCentered(cx,WIN_H*0.55f,buf,GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.4f,0.7f,1); snprintf(buf,128,"SECTOR REACHED: %d",gLevel); bitmapCentered(cx,WIN_H*0.48f,buf,GLUT_BITMAP_HELVETICA_18);
        
        glColor3f(0.4f,0.4f,0.5f); bitmapCentered(cx,WIN_H*0.15f,"DEVELOPED BY AL AMIN HOSSAIN",GLUT_BITMAP_HELVETICA_12);

        float blink=0.5f+0.5f*sinf(gGlobalTime*5); glColor4f(1,1,1,blink); bitmapCentered(cx,WIN_H*0.35f,"[ Press SPACE to Rebuild Core ]",GLUT_BITMAP_HELVETICA_18);
        glDisable(GL_BLEND);
    } else if(gState==STATE_WIN){
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0,0.05f,0.02f,0.95f); glRecti(0,0,WIN_W,WIN_H);
        strokeCentered(cx,WIN_H*0.65f,"CORE STABILIZED",0.40f,0,1,0.5f,1);
        char buf[128]; glColor3f(1,1,0); snprintf(buf,128,"FINAL SCORE: %06d",gScore); bitmapCentered(cx,WIN_H*0.5f,buf,GLUT_BITMAP_TIMES_ROMAN_24);
        float blink=0.5f+0.5f*sinf(gGlobalTime*5); glColor4f(1,1,1,blink); bitmapCentered(cx,WIN_H*0.35f,"[ Press SPACE to Replay ]",GLUT_BITMAP_HELVETICA_18);
        glDisable(GL_BLEND);
    } else if(gPaused){
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0,0,0,0.7f); glRecti(0,0,WIN_W,WIN_H);
        strokeCentered(cx,WIN_H*0.55f,"PAUSED",0.40f,1,1,0,1); glDisable(GL_BLEND);
    }
    glEnable(GL_DEPTH_TEST); glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

static void display(){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(50,(double)WIN_W/WIN_H,1,100);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    float sx=gCameraShake*frand(-0.3f,0.3f), sy=gCameraShake*frand(-0.3f,0.3f);
    gluLookAt(gCamX+sx, gCamY-8+sy, 20, gCamX, gCamY, 0, 0,1,0);
    GLfloat lp[]={gCamX,gCamY+5,15,1}; glLightfv(GL_LIGHT0,GL_POSITION,lp); glEnable(GL_LIGHT0); glEnable(GL_LIGHTING); glEnable(GL_DEPTH_TEST);
    drawScene();
    if(gState==STATE_PLAY) drawHUD();
    drawOverlay();
    
    // Screen Flash
    if(gFlash>0){
        glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0,WIN_W,0,WIN_H);
        glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity(); glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        glColor4f(1,1,1,gFlash*0.5f); glRecti(0,0,WIN_W,WIN_H);
        glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST); glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    }
    
    glutSwapBuffers();
}

static void reshape(int w,int h){WIN_W=w; WIN_H=h?h:1; glViewport(0,0,WIN_W,WIN_H);}

// ──────────────────────────────────────────────────────────────
//  Input
// ──────────────────────────────────────────────────────────────
static void specialDown(int k,int,int){ if(gState!=STATE_PLAY||gPaused)return; if(k==GLUT_KEY_LEFT)gMoveLeft=true; if(k==GLUT_KEY_RIGHT)gMoveRight=true; if(k==GLUT_KEY_UP)gJumpHeld=true; }
static void specialUp(int k,int,int){ if(k==GLUT_KEY_LEFT)gMoveLeft=false; if(k==GLUT_KEY_RIGHT)gMoveRight=false; if(k==GLUT_KEY_UP)gJumpHeld=false; }
static void keyDown(unsigned char k,int,int){
    if(k==27){ 
        if(gScore > gHighScore){ gHighScore = gScore; saveHS(); }
        exit(0);
    }
    if(k=='p'||k=='P') gPaused=!gPaused;
    if(k==' '&&gState==STATE_PLAY) gJumpHeld=true;
    if(k==' '&&gState!=STATE_PLAY) resetGame();
    if((k=='z'||k=='Z')&&gState==STATE_PLAY) gInflated=true;
    if((k=='x'||k=='X')&&gState==STATE_PLAY) gDeflated=true;
}
static void keyUp(unsigned char k,int,int){
    if(k==' ') gJumpHeld=false;
    if(k=='z'||k=='Z') gInflated=false;
    if(k=='x'||k=='X') gDeflated=false;
}

// ──────────────────────────────────────────────────────────────
//  Main
// ──────────────────────────────────────────────────────────────
int main(int argc,char** argv){
    srand((unsigned)time(nullptr)); loadHS();
    glutInit(&argc,argv); glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(WIN_W,WIN_H); glutCreateWindow("Neo Bounce: Reactor Core");
    glutFullScreen();
    glClearColor(0.01f,0.01f,0.04f,1);
    buildLevel(1); // For homescreen bg
    glutDisplayFunc(display); glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown); glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialDown); glutSpecialUpFunc(specialUp);
    glutTimerFunc(16,update,0);
    glutMainLoop(); return 0;
}
