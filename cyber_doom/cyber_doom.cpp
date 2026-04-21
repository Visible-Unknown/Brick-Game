/*
 * ================================================================
 *  NEO CYBER-DOOM  –  OpenGL / FreeGLUT [PRO EDITION]
 * ================================================================
 *
 *  FEATURES
 *  ─────────────────────────────────────────────────────────────
 *  • Wolfenstein-style raycasting first-person engine
 *  • TRON-style glowing wireframe neon maze
 *  • Enemy turrets with AI and projectile combat
 *  • Colored keycards, locked doors, exit portals
 *  • Power-ups: Health Pack, Speed Boost, Rapid Fire
 *  • Pro Edition UI, audio, particles, camera shake
 *
 *  CONTROLS
 *  ─────────────────────────────────────────────────────────────
 *  W/S or Up/Down     Move forward / backward
 *  A/D or Left/Right  Turn left / right
 *  SPACE / Z          Fire plasma bolt
 *  P                  Pause / Resume
 *  SPACE              Start / Restart (on menus)
 *  ESC                Quit
 *
 *  BUILD (Windows / MinGW)
 *  ─────────────────────────────────────────────────────────────
 *    g++ cyber_doom.cpp -o cyber_doom.exe -lfreeglut -lopengl32 -lglu32 -lwinmm
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

#ifndef M_PI
#define M_PI 3.14159265358979323846
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
//  Constants
// ──────────────────────────────────────────────────────────────
static int WIN_W=1280, WIN_H=720;
enum GameState { STATE_HOME, STATE_PLAY, STATE_GAMEOVER, STATE_WIN };

static const int MAP_W=16, MAP_H=16;
// Map legend: 1=wall, 2=red door, 3=blue door, 4=exit, 0=empty
// K=red key(in items), B=blue key(in items)
static int gMap[MAP_H][MAP_W] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,0,0,1,0,1,1,1,0,1,1,0,1},
    {1,0,1,0,0,0,0,0,0,0,1,0,0,0,0,1},
    {1,0,0,0,1,1,1,0,1,0,0,0,1,0,0,1},
    {1,0,1,0,0,0,0,0,1,0,1,1,1,0,1,1},
    {1,0,1,1,1,2,1,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,1,0,1,1,3,1,1,0,0,1},
    {1,1,1,0,1,0,0,0,0,0,0,0,1,0,1,1},
    {1,0,0,0,1,0,1,0,1,0,1,0,0,0,0,1},
    {1,0,1,0,0,0,1,0,1,0,1,1,1,0,0,1},
    {1,0,1,1,1,0,0,0,0,0,0,0,0,0,1,1},
    {1,0,0,0,0,0,1,0,1,0,0,1,0,0,0,1},
    {1,0,1,1,0,0,1,0,1,0,0,1,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,4,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
};

struct Bullet { float x,y,dx,dy; float life; bool isEnemy; };
struct EnemyTurret { float x,y; int hp; float fireTimer; bool alive; float angle; };
struct Item { float x,y; int type; bool alive; float pulse; }; // 0=health, 1=speed, 2=rapid, 3=red key, 4=blue key
struct Particle { float x,y,vx,vy,life,r,g,b; };

// ──────────────────────────────────────────────────────────────
//  Game State
// ──────────────────────────────────────────────────────────────
static GameState gState=STATE_HOME;
static float gPosX=1.5f, gPosY=1.5f;
static float gAngle=0;
static float gGlobalTime=0, gCameraShake=0;
static int gScore=0, gHighScore=0;
static int gHP=100;
static bool gHasRedKey=false, gHasBlueKey=false;
static bool gPaused=false;
static float gFireCooldown=0;
static float gSpeedTimer=0, gRapidTimer=0;
static float gBobPhase=0;

static bool gKeyW=false,gKeyS=false,gKeyA=false,gKeyD=false;

static std::vector<Bullet> gBullets;
static std::vector<EnemyTurret> gEnemies;
static std::vector<Item> gItems;
static std::vector<Particle> gParticles;

const char* SAVE_FILE="highscore.dat";

// ──────────────────────────────────────────────────────────────
//  Helpers
// ──────────────────────────────────────────────────────────────
static float frand(float lo,float hi){return lo+(hi-lo)*(float)rand()/(float)RAND_MAX;}
static void saveHS(){FILE*f=fopen(SAVE_FILE,"w");if(f){fprintf(f,"%d",gHighScore);fclose(f);}}
static void loadHS(){FILE*f=fopen(SAVE_FILE,"r");if(f){if(fscanf(f,"%d",&gHighScore)!=1)gHighScore=0;fclose(f);}}
static void spawnP(float px,float py,float r,float g,float b,int n){ for(int i=0;i<n;i++){Particle p;p.x=px;p.y=py;p.vx=frand(-2,2);p.vy=frand(-2,2);p.life=1;p.r=r;p.g=g;p.b=b;gParticles.push_back(p);} }

static bool isWall(int mx,int my){ if(mx<0||mx>=MAP_W||my<0||my>=MAP_H)return true; int v=gMap[my][mx]; return v==1; }
static bool isDoor(int mx,int my){ if(mx<0||mx>=MAP_W||my<0||my>=MAP_H)return false; return gMap[my][mx]==2||gMap[my][mx]==3; }
static bool isExit(int mx,int my){ if(mx<0||mx>=MAP_W||my<0||my>=MAP_H)return false; return gMap[my][mx]==4; }
static bool isSolid(int mx,int my){ if(mx<0||mx>=MAP_W||my<0||my>=MAP_H)return true; int v=gMap[my][mx]; return v!=0&&v!=4; }

static void initLevel(){
    gEnemies.clear(); gBullets.clear(); gItems.clear(); gParticles.clear();
    gPosX=1.5f; gPosY=1.5f; gAngle=0; gHP=100;
    gHasRedKey=false; gHasBlueKey=false;
    gSpeedTimer=0; gRapidTimer=0;
    
    // Place enemies
    float enemySpots[][2]={{3.5f,3.5f},{9.5f,2.5f},{5.5f,8.5f},{13.5f,5.5f},{10.5f,10.5f},{3.5f,12.5f},{7.5f,13.5f},{13.5f,13.5f}};
    for(int i=0;i<8;i++){
        EnemyTurret e; e.x=enemySpots[i][0]; e.y=enemySpots[i][1]; e.hp=3; e.fireTimer=frand(1,3); e.alive=true; e.angle=0;
        gEnemies.push_back(e);
    }
    
    // Place items: health, speed, rapid, red key, blue key
    Item h1; h1.x=7.5f;h1.y=3.5f;h1.type=0;h1.alive=true;h1.pulse=0; gItems.push_back(h1);
    Item h2; h2.x=11.5f;h2.y=9.5f;h2.type=0;h2.alive=true;h2.pulse=0; gItems.push_back(h2);
    Item sp; sp.x=4.5f;sp.y=5.5f;sp.type=1;sp.alive=true;sp.pulse=0; gItems.push_back(sp);
    Item rp; rp.x=13.5f;rp.y=1.5f;rp.type=2;rp.alive=true;rp.pulse=0; gItems.push_back(rp);
    Item rk; rk.x=1.5f;rk.y=4.5f;rk.type=3;rk.alive=true;rk.pulse=0; gItems.push_back(rk);
    Item bk; bk.x=1.5f;bk.y=11.5f;bk.type=4;bk.alive=true;bk.pulse=0; gItems.push_back(bk);
}

static void resetGame(){
    gScore=0; gState=STATE_PLAY; gPaused=false; gCameraShake=0;
    // Reset map doors
    gMap[6][5]=2; gMap[7][10]=3;
    initLevel();
    playBGM("../racing/audio/bgm.mp3",true);
}

// ──────────────────────────────────────────────────────────────
//  Raycasting
// ──────────────────────────────────────────────────────────────
struct RayHit { float dist; int side; int mapVal; float wallX; };

static RayHit castRay(float ox,float oy,float rayAngle){
    float dirX=cosf(rayAngle), dirY=sinf(rayAngle);
    int mapX=(int)ox, mapY=(int)oy;
    float ddx=fabs(1.0f/dirX), ddy=fabs(1.0f/dirY);
    int stepX,stepY; float sdx,sdy;
    if(dirX<0){stepX=-1; sdx=(ox-mapX)*ddx;} else {stepX=1; sdx=(mapX+1.0f-ox)*ddx;}
    if(dirY<0){stepY=-1; sdy=(oy-mapY)*ddy;} else {stepY=1; sdy=(mapY+1.0f-oy)*ddy;}
    
    RayHit hit; hit.dist=50; hit.side=0; hit.mapVal=1; hit.wallX=0;
    for(int i=0;i<64;i++){
        if(sdx<sdy){sdx+=ddx; mapX+=stepX; hit.side=0;} else {sdy+=ddy; mapY+=stepY; hit.side=1;}
        int v=0; if(mapX>=0&&mapX<MAP_W&&mapY>=0&&mapY<MAP_H) v=gMap[mapY][mapX];
        if(v!=0){
            hit.mapVal=v;
            if(hit.side==0) hit.dist=(mapX-ox+(1-stepX)/2)/dirX;
            else hit.dist=(mapY-oy+(1-stepY)/2)/dirY;
            float wx = (hit.side==0) ? oy+hit.dist*dirY : ox+hit.dist*dirX;
            hit.wallX=wx-floorf(wx);
            break;
        }
    }
    return hit;
}

// ──────────────────────────────────────────────────────────────
//  Update
// ──────────────────────────────────────────────────────────────
static void update(int){
    float dt=1.0f/60.0f;
    gGlobalTime+=dt;
    
    if(gState==STATE_PLAY&&!gPaused){
        if(gSpeedTimer>0) gSpeedTimer-=dt;
        if(gRapidTimer>0) gRapidTimer-=dt;
        gFireCooldown-=dt;
        
        float turnSpd=2.5f;
        float moveSpd = (gSpeedTimer>0) ? 5.0f : 3.0f;
        if(gKeyA) gAngle-=turnSpd*dt;
        if(gKeyD) gAngle+=turnSpd*dt;
        
        float dx=cosf(gAngle)*moveSpd*dt, dy=sinf(gAngle)*moveSpd*dt;
        if(gKeyW){
            if(!isSolid((int)(gPosX+dx*3),  (int)gPosY))   gPosX+=dx;
            if(!isSolid((int)gPosX,          (int)(gPosY+dy*3))) gPosY+=dy;
            gBobPhase+=dt*10;
        }
        if(gKeyS){
            if(!isSolid((int)(gPosX-dx*3),  (int)gPosY))   gPosX-=dx;
            if(!isSolid((int)gPosX,          (int)(gPosY-dy*3))) gPosY-=dy;
            gBobPhase+=dt*10;
        }
        
        // Door interaction
        int fmx=(int)(gPosX+cosf(gAngle)*0.7f), fmy=(int)(gPosY+sinf(gAngle)*0.7f);
        if(fmx>=0&&fmx<MAP_W&&fmy>=0&&fmy<MAP_H){
            if(gMap[fmy][fmx]==2&&gHasRedKey){ gMap[fmy][fmx]=0; playSFX("../match3/match3 music/special.wav"); }
            if(gMap[fmy][fmx]==3&&gHasBlueKey){ gMap[fmy][fmx]=0; playSFX("../match3/match3 music/special.wav"); }
        }
        
        // Check exit
        if(isExit((int)gPosX,(int)gPosY)){
            gState=STATE_WIN; gScore+=5000;
            playBGM(nullptr,false);
            if(gScore>gHighScore){gHighScore=gScore; saveHS();}
        }
        
        // Update bullets
        for(auto& b:gBullets){
            b.x+=b.dx*dt; b.y+=b.dy*dt; b.life-=dt;
            if(isSolid((int)b.x,(int)b.y)) b.life=0;
            
            if(!b.isEnemy){
                for(auto& e:gEnemies){
                    if(!e.alive) continue;
                    if(fabs(b.x-e.x)<0.5f&&fabs(b.y-e.y)<0.5f){
                        b.life=0; e.hp--;
                        spawnP(e.x,e.y,1,1,0.2f,5);
                        if(e.hp<=0){ e.alive=false; gScore+=500; gCameraShake=0.4f; spawnP(e.x,e.y,1,0.3f,0,20); playSFX("../match3/match3 music/explosion.wav"); }
                    }
                }
            } else {
                if(fabs(b.x-gPosX)<0.4f&&fabs(b.y-gPosY)<0.4f){
                    b.life=0; gHP-=15; gCameraShake=0.3f;
                    spawnP(gPosX,gPosY,1,0,0,10);
                    if(gHP<=0){ gState=STATE_GAMEOVER; playBGM(nullptr,false); playSFX("../tanks/tanks music/explosion.wav"); if(gScore>gHighScore){gHighScore=gScore;saveHS();} }
                }
            }
        }
        gBullets.erase(std::remove_if(gBullets.begin(),gBullets.end(),[](const Bullet& b){return b.life<=0;}),gBullets.end());
        
        // Enemy AI
        for(auto& e:gEnemies){
            if(!e.alive) continue;
            float toPlayerX=gPosX-e.x, toPlayerY=gPosY-e.y;
            float dist=sqrtf(toPlayerX*toPlayerX+toPlayerY*toPlayerY);
            e.angle=atan2f(toPlayerY,toPlayerX);
            
            if(dist<10.0f){
                e.fireTimer-=dt;
                if(e.fireTimer<=0){
                    e.fireTimer=frand(1.0f,2.5f);
                    Bullet b; b.x=e.x; b.y=e.y; b.dx=cosf(e.angle)*6; b.dy=sinf(e.angle)*6; b.life=3; b.isEnemy=true;
                    gBullets.push_back(b);
                }
            }
        }
        
        // Item collection
        for(auto& it:gItems){
            if(!it.alive) continue;
            if(fabs(gPosX-it.x)<0.6f&&fabs(gPosY-it.y)<0.6f){
                it.alive=false;
                playSFX("../tanks/tanks music/powerup.wav");
                if(it.type==0){ gHP+=30; if(gHP>100) gHP=100; gScore+=100; }
                else if(it.type==1){ gSpeedTimer=15; gScore+=200; }
                else if(it.type==2){ gRapidTimer=15; gScore+=200; }
                else if(it.type==3){ gHasRedKey=true; gScore+=300; }
                else if(it.type==4){ gHasBlueKey=true; gScore+=300; }
            }
        }
    }
    
    // Particles (minimap)
    for(auto& p:gParticles){p.x+=p.vx*dt;p.y+=p.vy*dt;p.life-=dt*2;}
    gParticles.erase(std::remove_if(gParticles.begin(),gParticles.end(),[](const Particle& p){return p.life<=0;}),gParticles.end());
    gCameraShake*=0.88f;
    glutPostRedisplay(); glutTimerFunc(16,update,0);
}

// ──────────────────────────────────────────────────────────────
//  UI
// ──────────────────────────────────────────────────────────────
static void bitmapAt(float px,float py,const char* s,void* f=GLUT_BITMAP_HELVETICA_18){ glRasterPos2f(px,py); for(const char* c=s;*c;c++) glutBitmapCharacter(f,*c); }
static int bitmapWidth(const char* s,void* f=GLUT_BITMAP_HELVETICA_18){ int w=0; for(const char* c=s;*c;c++) w+=glutBitmapWidth(f,*c); return w; }
static void bitmapCentered(float cx,float py,const char* s,void* f=GLUT_BITMAP_HELVETICA_18){ bitmapAt(cx-bitmapWidth(s,f)*0.5f,py,s,f); }
static void strokeCentered(float cx,float cy,const std::string& s,float scale,float r,float g,float b,float a){
    float tw=0; for(char c:s) tw+=glutStrokeWidth(GLUT_STROKE_ROMAN,c);
    float sx=cx-tw*scale*0.5f;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glColor4f(r,g,b,a*0.35f); glLineWidth(6); glPushMatrix(); glTranslatef(sx,cy,0); glScalef(scale,scale,scale); for(char c:s) glutStrokeCharacter(GLUT_STROKE_ROMAN,c); glPopMatrix();
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,1,a); glLineWidth(2.5f); glPushMatrix(); glTranslatef(sx,cy,0); glScalef(scale,scale,scale); for(char c:s) glutStrokeCharacter(GLUT_STROKE_ROMAN,c); glPopMatrix();
    glLineWidth(1); glDisable(GL_BLEND);
}

// ──────────────────────────────────────────────────────────────
//  Rendering
// ──────────────────────────────────────────────────────────────
static void drawFPView(){
    // 2D orthographic for raycast columns
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING);
    
    float bob = sinf(gBobPhase)*4.0f;
    float FOV = 60.0f * (float)M_PI/180.0f;
    
    // Sky & Floor
    glBegin(GL_QUADS);
    glColor3f(0.02f,0.02f,0.08f); glVertex2f(0,WIN_H/2+bob); glVertex2f(WIN_W,WIN_H/2+bob);
    glColor3f(0.08f,0.04f,0.15f); glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H);
    glEnd();
    glBegin(GL_QUADS);
    glColor3f(0.02f,0.05f,0.02f); glVertex2f(0,WIN_H/2+bob); glVertex2f(WIN_W,WIN_H/2+bob);
    glColor3f(0.01f,0.02f,0.01f); glVertex2f(WIN_W,0); glVertex2f(0,0);
    glEnd();
    
    float sx=gCameraShake*frand(-3,3), sy=gCameraShake*frand(-3,3);
    
    // Raycast walls
    for(int x=0; x<WIN_W; x++){
        float rayAngle = gAngle - FOV/2 + FOV*(float)x/(float)WIN_W;
        RayHit hit = castRay(gPosX, gPosY, rayAngle);
        float perpDist = hit.dist * cosf(rayAngle - gAngle);
        if(perpDist < 0.01f) perpDist = 0.01f;
        
        float lineH = WIN_H / perpDist;
        float drawStart = WIN_H/2 - lineH/2 + bob + sy;
        float drawEnd = WIN_H/2 + lineH/2 + bob + sy;
        
        float intensity = 1.0f / (1.0f + perpDist*0.15f);
        float r=0,g=0,b=0;
        if(hit.mapVal==1){ r=0; g=0.5f*intensity; b=0.8f*intensity; }
        else if(hit.mapVal==2){ r=0.9f*intensity; g=0.1f*intensity; b=0.1f*intensity; }
        else if(hit.mapVal==3){ r=0.1f*intensity; g=0.2f*intensity; b=0.9f*intensity; }
        else if(hit.mapVal==4){ r=0.1f*intensity; g=0.9f*intensity; b=0.3f*intensity; }
        if(hit.side==1){r*=0.7f; g*=0.7f; b*=0.7f;}
        
        glColor3f(r,g,b);
        glBegin(GL_LINES); glVertex2f(x+sx, drawStart); glVertex2f(x+sx, drawEnd); glEnd();
    }
    
    // Draw weapon view model (bottom center)
    float weaponBob = sinf(gBobPhase*0.5f)*10;
    float wcx = WIN_W/2 + weaponBob;
    float wcy = 80 + bob;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    // Gun body
    glColor4f(0.1f,0.6f,0.3f,0.8f);
    glBegin(GL_QUADS); glVertex2f(wcx-20,wcy); glVertex2f(wcx+20,wcy); glVertex2f(wcx+15,wcy+80); glVertex2f(wcx-15,wcy+80); glEnd();
    // Barrel
    glColor4f(0.2f,0.8f,0.4f,0.9f);
    glBegin(GL_QUADS); glVertex2f(wcx-8,wcy+80); glVertex2f(wcx+8,wcy+80); glVertex2f(wcx+5,wcy+130); glVertex2f(wcx-5,wcy+130); glEnd();
    // Muzzle glow
    if(gFireCooldown > 0.05f){
        float flash = gFireCooldown * 5;
        glColor4f(0,1,0.5f,flash); glBegin(GL_TRIANGLE_FAN); glVertex2f(wcx,wcy+135);
        for(int i=0;i<=12;i++){ float a=i*M_PI*2/12; glVertex2f(wcx+cosf(a)*20*flash, wcy+135+sinf(a)*20*flash); }
        glEnd();
    }
    glDisable(GL_BLEND);
    
    // Crosshair
    glColor3f(0,1,0.5f); glLineWidth(2);
    glBegin(GL_LINES);
    glVertex2f(WIN_W/2-15,WIN_H/2); glVertex2f(WIN_W/2-5,WIN_H/2);
    glVertex2f(WIN_W/2+5,WIN_H/2); glVertex2f(WIN_W/2+15,WIN_H/2);
    glVertex2f(WIN_W/2,WIN_H/2-15); glVertex2f(WIN_W/2,WIN_H/2-5);
    glVertex2f(WIN_W/2,WIN_H/2+5); glVertex2f(WIN_W/2,WIN_H/2+15);
    glEnd();
}

static void drawMinimap(){
    float ms=6; // minimap scale
    float ox=WIN_W-MAP_W*ms-15, oy=15;
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,0.6f); glBegin(GL_QUADS); glVertex2f(ox-5,oy-5); glVertex2f(ox+MAP_W*ms+5,oy-5); glVertex2f(ox+MAP_W*ms+5,oy+MAP_H*ms+5); glVertex2f(ox-5,oy+MAP_H*ms+5); glEnd();
    
    for(int y=0;y<MAP_H;y++) for(int x=0;x<MAP_W;x++){
        int v=gMap[y][x];
        if(v==0) continue;
        if(v==1) glColor4f(0,0.4f,0.7f,0.5f);
        else if(v==2) glColor4f(0.8f,0.1f,0.1f,0.7f);
        else if(v==3) glColor4f(0.1f,0.1f,0.8f,0.7f);
        else if(v==4) glColor4f(0,0.8f,0.3f,0.7f);
        glBegin(GL_QUADS); glVertex2f(ox+x*ms,oy+y*ms); glVertex2f(ox+(x+1)*ms,oy+y*ms); glVertex2f(ox+(x+1)*ms,oy+(y+1)*ms); glVertex2f(ox+x*ms,oy+(y+1)*ms); glEnd();
    }
    
    // Player dot
    glColor3f(0,1,0.5f); glPointSize(4);
    glBegin(GL_POINTS); glVertex2f(ox+gPosX*ms, oy+gPosY*ms); glEnd();
    // Direction line
    glBegin(GL_LINES); glVertex2f(ox+gPosX*ms, oy+gPosY*ms); glVertex2f(ox+(gPosX+cosf(gAngle)*1.5f)*ms, oy+(gPosY+sinf(gAngle)*1.5f)*ms); glEnd();
    
    // Enemies
    glColor3f(1,0.2f,0.1f); glPointSize(3);
    glBegin(GL_POINTS); for(auto& e:gEnemies) if(e.alive) glVertex2f(ox+e.x*ms,oy+e.y*ms); glEnd();
    
    glDisable(GL_BLEND);
}

static void drawHUD(){
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0.04f,0.12f,0.88f); glRecti(0,WIN_H-55,WIN_W,WIN_H);
    glColor4f(0,1,0.4f,0.7f); glLineWidth(2.5f); glBegin(GL_LINES); glVertex2f(0,WIN_H-55); glVertex2f(WIN_W,WIN_H-55); glEnd();
    char buf[128]; glColor3f(0,1,0.4f);
    snprintf(buf,128,"SCORE: %06d",gScore); bitmapAt(30,WIN_H-38,buf,GLUT_BITMAP_TIMES_ROMAN_24);
    
    // HP bar
    float hpPct = gHP/100.0f;
    glColor3f(1-hpPct, hpPct, 0); glBegin(GL_QUADS); glVertex2f(WIN_W/2-100,WIN_H-48); glVertex2f(WIN_W/2-100+200*hpPct,WIN_H-48); glVertex2f(WIN_W/2-100+200*hpPct,WIN_H-30); glVertex2f(WIN_W/2-100,WIN_H-30); glEnd();
    glColor3f(0.5f,0.5f,0.5f); glLineWidth(1); glBegin(GL_LINE_LOOP); glVertex2f(WIN_W/2-100,WIN_H-48); glVertex2f(WIN_W/2+100,WIN_H-48); glVertex2f(WIN_W/2+100,WIN_H-30); glVertex2f(WIN_W/2-100,WIN_H-30); glEnd();
    snprintf(buf,128,"HP: %d%%",gHP); glColor3f(1,1,1); bitmapCentered(WIN_W/2,WIN_H-38,buf,GLUT_BITMAP_HELVETICA_18);
    
    glColor3f(0,1,1); snprintf(buf,128,"PEAK: %06d",gHighScore); bitmapAt(WIN_W-220,WIN_H-38,buf,GLUT_BITMAP_TIMES_ROMAN_24);
    
    // Keys
    float keyX = 30;
    if(gHasRedKey){ glColor3f(1,0.2f,0.2f); bitmapAt(keyX, 30, "[RED KEY]", GLUT_BITMAP_HELVETICA_18); keyX+=100; }
    if(gHasBlueKey){ glColor3f(0.2f,0.2f,1); bitmapAt(keyX, 30, "[BLUE KEY]", GLUT_BITMAP_HELVETICA_18); }
    
    if(gSpeedTimer>0){glColor3f(1,0.8f,0);snprintf(buf,128,"SPEED: %.1fs",gSpeedTimer);bitmapAt(30,50,buf,GLUT_BITMAP_HELVETICA_18);}
    if(gRapidTimer>0){glColor3f(0.2f,1,0.2f);snprintf(buf,128,"RAPID: %.1fs",gRapidTimer);bitmapAt(30,70,buf,GLUT_BITMAP_HELVETICA_18);}
    
    glDisable(GL_BLEND);
}

static void drawOverlay(){
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity(); glDisable(GL_DEPTH_TEST);
    float cx=WIN_W*0.5f;
    if(gState==STATE_HOME){
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0,0,0.02f,0.92f); glRecti(0,0,WIN_W,WIN_H);
        float p=0.8f+0.2f*sinf(gGlobalTime*3);
        strokeCentered(cx,WIN_H*0.65f,"NEO CYBER-DOOM",0.4f,0,0.8f,0.3f,p);
        strokeCentered(cx,WIN_H*0.48f,"MAINFRAME BREACH",0.28f,1,0.3f,0,p);
        float blink=0.5f+0.5f*sinf(gGlobalTime*6); glColor3f(blink,blink,blink);
        bitmapCentered(cx,WIN_H*0.28f,"[ PRESS SPACE TO INFILTRATE ]",GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.5f,0.5f,0.5f);
        bitmapCentered(cx,WIN_H*0.18f,"WASD = Move/Turn  |  Space/Z = Fire  |  Walk into doors with keys",GLUT_BITMAP_HELVETICA_18);
        glDisable(GL_BLEND);
    } else if(gState==STATE_GAMEOVER){
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.15f,0,0,0.92f); glRecti(0,0,WIN_W,WIN_H);
        glBegin(GL_QUADS);glColor4f(0.04f,0,0.08f,0.75f);glVertex2f(cx-300,WIN_H*0.2f);glVertex2f(cx+300,WIN_H*0.2f);glVertex2f(cx+300,WIN_H*0.8f);glVertex2f(cx-300,WIN_H*0.8f);glEnd();
        strokeCentered(cx,WIN_H*0.7f,"SYSTEM TERMINATED",0.28f,1,0.1f,0,1);
        char buf[128]; glColor3f(1,0.8f,0); snprintf(buf,128,"FINAL SCORE: %06d",gScore); bitmapCentered(cx,WIN_H*0.55f,buf,GLUT_BITMAP_TIMES_ROMAN_24);
        float blink=0.5f+0.5f*sinf(gGlobalTime*5); glColor4f(blink,blink,blink,1); bitmapCentered(cx,WIN_H*0.35f,"[ Press SPACE to Reboot ]",GLUT_BITMAP_HELVETICA_18);
        glDisable(GL_BLEND);
    } else if(gState==STATE_WIN){
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0,0.05f,0.02f,0.92f); glRecti(0,0,WIN_W,WIN_H);
        strokeCentered(cx,WIN_H*0.65f,"MAINFRAME SECURED",0.32f,0,1,0.5f,1);
        char buf[128]; glColor3f(1,1,0); snprintf(buf,128,"FINAL SCORE: %06d",gScore); bitmapCentered(cx,WIN_H*0.5f,buf,GLUT_BITMAP_TIMES_ROMAN_24);
        float blink=0.5f+0.5f*sinf(gGlobalTime*5); glColor4f(blink,blink,blink,1); bitmapCentered(cx,WIN_H*0.35f,"[ Press SPACE to Replay ]",GLUT_BITMAP_HELVETICA_18);
        glDisable(GL_BLEND);
    } else if(gPaused){
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0,0,0,0.6f); glRecti(0,0,WIN_W,WIN_H);
        strokeCentered(cx,WIN_H*0.55f,"PAUSED",0.35f,1,1,0,1); glDisable(GL_BLEND);
    }
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

static void display(){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    if(gState==STATE_PLAY){
        drawFPView();
        drawMinimap();
        drawHUD();
        drawOverlay();
    } else {
        // For menu screens, draw a simple animated bg
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluOrtho2D(0,WIN_W,0,WIN_H);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
        // Scrolling grid
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        glColor4f(0,0.3f,0.5f,0.1f); glLineWidth(1);
        float off=fmod(gGlobalTime*30,40);
        glBegin(GL_LINES);
        for(float x=-off;x<WIN_W+40;x+=40){glVertex2f(x,0);glVertex2f(x,WIN_H);}
        for(float y=-off;y<WIN_H+40;y+=40){glVertex2f(0,y);glVertex2f(WIN_W,y);}
        glEnd(); glDisable(GL_BLEND);
        drawOverlay();
    }
    glutSwapBuffers();
}

static void reshape(int w,int h){WIN_W=w;WIN_H=h?h:1;glViewport(0,0,WIN_W,WIN_H);}

// ──────────────────────────────────────────────────────────────
//  Input
// ──────────────────────────────────────────────────────────────
static void keyDown(unsigned char k,int,int){
    if(k==27) exit(0);
    if(k=='p'||k=='P') gPaused=!gPaused;
    if(k=='w'||k=='W') gKeyW=true; if(k=='s'||k=='S') gKeyS=true;
    if(k=='a'||k=='A') gKeyA=true; if(k=='d'||k=='D') gKeyD=true;
    if((k==' '||k=='z'||k=='Z')&&gState==STATE_PLAY&&gFireCooldown<=0){
        gFireCooldown=(gRapidTimer>0)?0.1f:0.25f;
        Bullet b; b.x=gPosX; b.y=gPosY; b.dx=cosf(gAngle)*12; b.dy=sinf(gAngle)*12; b.life=2; b.isEnemy=false;
        gBullets.push_back(b); playSFX("../match3/match3 music/swap.wav");
    }
    if(k==' '&&gState!=STATE_PLAY) resetGame();
}
static void keyUp(unsigned char k,int,int){
    if(k=='w'||k=='W') gKeyW=false; if(k=='s'||k=='S') gKeyS=false;
    if(k=='a'||k=='A') gKeyA=false; if(k=='d'||k=='D') gKeyD=false;
}
static void specialDown(int k,int,int){ if(k==GLUT_KEY_UP)gKeyW=true; if(k==GLUT_KEY_DOWN)gKeyS=true; if(k==GLUT_KEY_LEFT)gKeyA=true; if(k==GLUT_KEY_RIGHT)gKeyD=true; }
static void specialUp(int k,int,int){ if(k==GLUT_KEY_UP)gKeyW=false; if(k==GLUT_KEY_DOWN)gKeyS=false; if(k==GLUT_KEY_LEFT)gKeyA=false; if(k==GLUT_KEY_RIGHT)gKeyD=false; }

// ──────────────────────────────────────────────────────────────
//  Main
// ──────────────────────────────────────────────────────────────
int main(int argc,char** argv){
    srand((unsigned)time(nullptr)); loadHS();
    glutInit(&argc,argv); glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(WIN_W,WIN_H); glutCreateWindow("Neo Cyber-Doom: Mainframe Breach");
    glutFullScreen();
    glClearColor(0.01f,0.01f,0.03f,1);
    glutDisplayFunc(display); glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown); glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialDown); glutSpecialUpFunc(specialUp);
    glutTimerFunc(16,update,0);
    glutMainLoop(); return 0;
}
