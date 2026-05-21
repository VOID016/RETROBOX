// ============================================================
//  game_mario.h  —  RETROBOX built-in game
//  Mario Platformer — 16 levels, shop, bosses
// ============================================================
#pragma once
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <math.h>

extern Adafruit_SSD1306 display;
extern Preferences       prefs;
extern int  PLAY_W, PLAY_H;
extern int  highScores[][3];
extern int  selectedGame;
extern Difficulty currentDiff;
extern bool joyLeft(), joyRight(), joyUp(), joyDown();
extern int  joyMoveStep();
extern Button btnA, btnB, btnStart, btnJoySW;
extern void sfxBeep(), sfxConfirm(), sfxBack(), sfxShoot();
extern void sfxExplode(), sfxGameOver(), sfxLevelUp(), sfxMenu(), sfxHit();
extern void saveScore(int game, int diff, int score);
extern void drawCenteredText(int y, const char* str, int size);
extern void scrollStarsV(int* sx, int* sy, int n, int speed);
extern void drawShip(int x, int y);

namespace MarioGame {

// ── CONSTANTS ────────────────────────────────────────────────
static constexpr int SW      = 128;
static constexpr int SH      = 64;
static constexpr int HUD_H   = 8;
static constexpr int PT      = HUD_H;
static constexpr int TILE    = 8;
static constexpr int TROWS   = 7;
static constexpr int TCOLS   = 16;
static constexpr int LW      = 256;
static constexpr int LEVELS  = 16;
static constexpr int FP      = 256;
static constexpr int PW      = 8;
static constexpr int PH      = 14;
static constexpr int MAX_E   = 10;
static constexpr int MAX_FB  = 2;
static constexpr int MAX_PT  = 16;

// Physics (all in FP units unless noted)
static constexpr int GRAV       =  58;
static constexpr int MAX_FALL   = 1200;
static constexpr int J_IMPULSE  = -980;
static constexpr int J_CUT      = -480;
static constexpr int WALK_SPD   =  380;
static constexpr int RUN_SPD    =  620;
static constexpr int ACC_GND    =  44;
static constexpr int ACC_AIR    =  30;
static constexpr int FRICTION   =  82;   // /100 per frame

static constexpr int SHOP_ITEMS = 8;

// ── ENUMS ────────────────────────────────────────────────────
enum TileType  : uint8_t {
    T_AIR=0,T_SOLID,T_BRICK,T_QBLOCK,T_USED,T_COIN,
    T_PIPE_T,T_PIPE_B,T_SEMI,T_HARM,T_FLAG_P,T_FLAG_T,T_HIDDEN
};
enum EType     : uint8_t { E_NONE=0,E_GOOMBA,E_KOOPA,E_PIRANHA,E_BILL,E_BOSS };
enum GMode     : uint8_t { M_PLAY=0,M_SHOP,M_ALLCLEAR };
enum ResItem   : uint8_t { RI_NONE=0,RI_MUSH,RI_FIRE,RI_STAR,RI_SHIELD };

// Difficulty helpers — map RETROBOX global currentDiff to Mario logic
static inline bool diffEasy() { return currentDiff == DIFF_EASY; }
static inline bool diffHard() { return currentDiff == DIFF_HARD; }

// ── STRUCTS ──────────────────────────────────────────────────
struct Enemy {
    int x,y,vx,vy,baseY;
    int hp;
    uint8_t type,shell,timer;
    bool alive;
};
struct Fireball { int x,y,vx,vy; bool active; };
struct Particle { int x,y,vx,vy; uint8_t life; bool active; };

// ── SHOP DEFINITION ──────────────────────────────────────────
struct ShopItem {
    const char* name;
    const char* desc;
    int         cost;
    uint8_t     icon;
};

static const ShopItem SHOP[SHOP_ITEMS] = {
    { "1UP",    "+1 Life",        50,  'L' },
    { "MUSH",   "Mushroom rsv",   40,  'M' },
    { "FIRE",   "Fire Flower",    75,  'F' },
    { "STAR",   "Star power",    100,  '*' },
    { "SHIELD", "1-hit shield",   80,  'S' },
    { "2UP",    "+2 Lives",       90,  '2' },
    { "MAP",    "Reveal hiddn",  120,  '?' },
    { "COINS+", "Convert 5pts",   30,  'C' },
};

// ── LEVEL DATA ───────────────────────────────────────────────
static uint8_t  tileMap[TROWS][LW];
static uint8_t  coinId [TROWS][LW];
static uint8_t  blockId[TROWS][LW];
static uint32_t coinsTaken[LEVELS];
static uint32_t blocksUsed[LEVELS];

// ── GAME STATE ───────────────────────────────────────────────
static Enemy    enemies  [MAX_E];
static Fireball fireballs[MAX_FB];
static Particle particles[MAX_PT];

static int  px,py,vx,vy;
static int  safeX,safeY,camX;
static int  score,lives,totalCoins;
static int  levelIndex,unlockedLevel;
static int  invulnTimer,starTimer,shieldTimer;
static int  walkFrame,shopCursor,comboCounter;
static int  endlessCycle;
static int  bossHp, bossMaxHp;
static uint8_t  reserveItem;
static bool onGround,facingRight,ducking,jumpHeld;
static int  coyoteFrames;   // frames remaining where jump is still allowed after walking off a ledge
static bool powered,fireMode,shielded;
static bool gameOver,newRecord,mapReveal;
static GMode mode;
static uint8_t nextCoinId,nextBlockId,enemyCount;
static unsigned long lastShopMove;

static unsigned long sfxEndAt = 0;

// ── FORWARD DECLARATIONS ─────────────────────────────────────
void buildLevel();
void startLevel();
void saveProgress();
void damagePlayer(bool pit);

// ── NON-BLOCKING AUDIO ───────────────────────────────────────
// Shares LEDC channel 0 with RETROBOX. Mario's sfxTick() must be
// called each frame; it cuts tones without blocking, ensuring button
// presses are never swallowed by audio waits.
void sfxTick() {
    if (sfxEndAt && millis() >= sfxEndAt) { ledcWriteTone(0,0); sfxEndAt=0; }
}
void sfxPlay(int freq, int ms) { ledcWriteTone(0,freq); sfxEndAt=millis()+ms; }
void sfxJump()    { sfxPlay(494, 55); }
void sfxCoin()    { sfxPlay(1046,40); }
void sfxBrick()   { sfxPlay(300, 30); }
void sfxStomp()   { sfxPlay(196, 65); }
void sfxDie()     { sfxPlay(196,380); }
void sfxFlag()    { sfxPlay(784,220); }
// sfxHit() and sfxMenu() are defined globally in RETROBOX; use them directly.
void sfxPowerUp() { sfxPlay(659,160); }
void sfxShoot_m() { sfxPlay(880, 35); }  // renamed to avoid conflict with global sfxShoot()
void sfxBuy()     { sfxPlay(523,200); }
void sfxBoss()    { sfxPlay(220,120); }
void sfxBossDie() { sfxPlay(880,300); }
void sfxExtraLife(){ sfxPlay(1046,250);}

// ── FIXED-POINT HELPERS ──────────────────────────────────────
static inline int fpPx(int f)   { return f/FP; }
static inline int pxFP(int p)   { return p*FP; }
static inline int tileFP(int t) { return t*TILE*FP; }
static inline int fpTile(int f) { return fpPx(f)/TILE; }

// ── BIT HELPERS ──────────────────────────────────────────────
static bool bGet(uint32_t m, uint8_t id){ return id<32 && (m>>(id))&1; }
static void bSet(uint32_t& m,uint8_t id){ if(id<32) m|=(1UL<<id); }

// ── NVS SAVE / LOAD / RESET ──────────────────────────────────
// Uses namespace "mario2" (separate from RETROBOX's "rb24" score store)
void saveProgress() {
    prefs.begin("mario2",false);
    prefs.putInt("coins",   totalCoins);
    prefs.putInt("lives",   lives);
    prefs.putInt("unlock",  unlockedLevel);
    prefs.putInt("res",     (int)reserveItem);
    prefs.putInt("map",     mapReveal?1:0);
    prefs.putInt("cycle",   endlessCycle);
    prefs.putBool("shield", shielded);
    for(int i=0;i<LEVELS;i++){
        char k[12];
        sprintf(k,"cn%d",i); prefs.putUInt(k,coinsTaken[i]);
        sprintf(k,"bk%d",i); prefs.putUInt(k,blocksUsed[i]);
    }
    prefs.end();
}

void loadProgress() {
    prefs.begin("mario2",true);
    totalCoins    = prefs.getInt("coins",  0);
    lives         = prefs.getInt("lives",  3);
    unlockedLevel = prefs.getInt("unlock", 0);
    reserveItem   = (uint8_t)prefs.getInt("res",RI_NONE);
    mapReveal     = prefs.getInt("map",0)!=0;
    endlessCycle  = prefs.getInt("cycle",0);
    shielded      = prefs.getBool("shield",false);
    for(int i=0;i<LEVELS;i++){
        char k[12];
        sprintf(k,"cn%d",i); coinsTaken[i]=prefs.getUInt(k,0);
        sprintf(k,"bk%d",i); blocksUsed[i]=prefs.getUInt(k,0);
    }
    prefs.end();
    totalCoins    = constrain(totalCoins,0,9999);
    lives         = constrain(lives,1,99);
    unlockedLevel = constrain(unlockedLevel,0,LEVELS-1);
    if(reserveItem>RI_SHIELD) reserveItem=RI_NONE;
    endlessCycle  = constrain(endlessCycle,0,9);
}

void resetProgress() {
    totalCoins=0; lives=3; unlockedLevel=0;
    reserveItem=RI_NONE; mapReveal=false;
    endlessCycle=0; shielded=false;
    memset(coinsTaken,0,sizeof(coinsTaken));
    memset(blocksUsed,0,sizeof(blocksUsed));
    saveProgress();
}

// ── TILE HELPERS ─────────────────────────────────────────────
static bool isSolid(uint8_t t){
    return t==T_SOLID||t==T_BRICK||t==T_QBLOCK||t==T_USED||
           t==T_PIPE_T||t==T_PIPE_B||t==T_HIDDEN||t==T_HARM;
}
static bool isBump(uint8_t t){ return t==T_BRICK||t==T_QBLOCK||t==T_HIDDEN; }

static uint8_t getTile(int tx,int ty){
    if(tx<0||tx>=LW||ty<0) return T_SOLID;
    if(ty>=TROWS) return T_AIR;
    return tileMap[ty][tx];
}
static void setTile(int tx,int ty,uint8_t t){
    if(tx<0||tx>=LW||ty<0||ty>=TROWS) return;
    tileMap[ty][tx]=t;
}
static void fillTiles(int x,int y,int w,int h,uint8_t t){
    for(int yy=y;yy<y+h;yy++)
        for(int xx=x;xx<x+w;xx++) setTile(xx,yy,t);
}

static bool hitSolid(int fpx,int fpy,int pw,int ph){
    int x0=fpPx(fpx),y0=fpPx(fpy),x1=x0+pw-1,y1=y0+ph-1;
    for(int ty=y0/TILE;ty<=y1/TILE;ty++)
        for(int tx=x0/TILE;tx<=x1/TILE;tx++)
            if(isSolid(getTile(tx,ty))) return true;
    return false;
}
static bool touchType(uint8_t want,int fpx,int fpy,int pw,int ph){
    int x0=fpPx(fpx),y0=fpPx(fpy),x1=x0+pw-1,y1=y0+ph-1;
    for(int ty=y0/TILE;ty<=y1/TILE;ty++)
        for(int tx=x0/TILE;tx<=x1/TILE;tx++)
            if(getTile(tx,ty)==want) return true;
    return false;
}

// ── PARTICLES ────────────────────────────────────────────────
void spawnParticles(int fpx,int fpy,int n){
    for(int i=0;i<MAX_PT&&n>0;i++){
        if(particles[i].active) continue;
        particles[i]={
            fpx+random(-2,3)*FP, fpy+random(-2,3)*FP,
            random(-220,221),    random(-420,-80),
            (uint8_t)random(6,14), true
        };
        n--;
    }
}
void updateParticles(){
    for(auto& p:particles){
        if(!p.active) continue;
        p.vy+=40; p.x+=p.vx; p.y+=p.vy;
        if(p.life) p.life--; else p.active=false;
    }
}

// ── LEVEL BUILDER HELPERS ────────────────────────────────────
void clearLevel(){
    for(int y=0;y<TROWS;y++)
        for(int x=0;x<LW;x++){
            tileMap[y][x]=T_AIR;
            coinId [y][x]=255;
            blockId[y][x]=255;
        }
    for(int x=0;x<LW;x++) tileMap[TROWS-1][x]=T_SOLID;
    for(int i=0;i<MAX_E; i++) enemies[i].alive=false;
    for(int i=0;i<MAX_FB;i++) fireballs[i].active=false;
    for(int i=0;i<MAX_PT;i++) particles[i].active=false;
    nextCoinId=nextBlockId=enemyCount=0;
}

void placeGap(int x,int w){
    w=constrain(w,2,4);
    for(int xx=x;xx<x+w;xx++){
        setTile(xx,TROWS-1,T_AIR);
        setTile(xx,TROWS-2,T_AIR);
    }
}
void placeSemi(int x,int y,int w){ w=max(w,3); fillTiles(x,y,w,1,T_SEMI); }
void placePlatform(int x,int y,int w){ fillTiles(x,y,w,1,T_SOLID); }

void placePipe(int x,int h,bool plant){
    h=constrain(h,2,3);
    int top=TROWS-h;
    setTile(x,top,T_PIPE_T); setTile(x+1,top,T_PIPE_T);
    for(int y=top+1;y<TROWS;y++){
        setTile(x,y,T_PIPE_B); setTile(x+1,y,T_PIPE_B);
    }
    if(plant&&enemyCount<MAX_E){
        Enemy& e=enemies[enemyCount++];
        e={tileFP(x),pxFP(top*TILE-10),0,0,pxFP(top*TILE-2),1,E_PIRANHA,0,0,true};
    }
}

void addCoin(int x,int y){
    if(nextCoinId>=32) return;
    uint8_t id=nextCoinId++;
    if(!bGet(coinsTaken[levelIndex],id)){
        setTile(x,y,T_COIN);
        coinId[y][x]=id;
    }
}
void coinRow(int x,int y,int n){ for(int i=0;i<n;i++) addCoin(x+i*2,y); }

void addBlock(int x,int y,uint8_t t){
    if(nextBlockId>=32) return;
    uint8_t id=nextBlockId++;
    blockId[y][x]=id;
    if(bGet(blocksUsed[levelIndex],id))
        setTile(x,y,(t==T_BRICK)?T_AIR:T_USED);
    else
        setTile(x,y,t);
}

void stairUp(int x,int steps){
    for(int i=0;i<steps;i++)
        fillTiles(x+i,TROWS-1-i,1,i+1,T_SOLID);
}

void spawnEnemy(uint8_t type,int tx){
    if(enemyCount>=MAX_E) return;
    if(diffEasy()&&enemyCount>=6) return;
    Enemy& e=enemies[enemyCount++];
    int spd=diffHard()?-125:(diffEasy()?-62:-88);
    if(type==E_KOOPA) spd=(spd*3)/4;
    if(type==E_BILL)  spd=diffHard()?-195:-145;
    int h=(type==E_KOOPA)?14:8;
    e={tileFP(tx),pxFP((TROWS-1)*TILE-h),spd,0,0,1,type,0,0,true};
}

void spawnBoss(int tx){
    if(enemyCount>=MAX_E) return;
    Enemy& e=enemies[enemyCount++];
    int hp=diffHard()?6:(diffEasy()?2:4);
    bossHp=bossMaxHp=hp;
    int spd=diffHard()?-110:-80;
    e={tileFP(tx),pxFP((TROWS-1)*TILE-16),spd,0,0,hp,E_BOSS,0,0,true};
}

void placeFlag(){
    int x=LW-6;
    setTile(x,1,T_FLAG_T);
    for(int y=2;y<TROWS;y++) setTile(x,y,T_FLAG_P);
}

// ── 16 HAND-CRAFTED LEVELS ───────────────────────────────────
void buildLevel(){
    clearLevel();
    int world=levelIndex/4;

    if(world==1) for(int x=0;x<LW;x++) setTile(x,0,T_SOLID);
    if(world==2)
        for(int x=10;x<LW-10;x+=24){
            setTile(x,  TROWS-1,T_HARM);
            setTile(x+1,TROWS-1,T_HARM);
        }
    if(world==3)
        for(int x=16;x<LW-16;x+=28)
            for(int xx=x;xx<x+8;xx++) setTile(xx,TROWS-1,T_AIR);

    switch(levelIndex){
    case 0:
        coinRow(10,4,4);
        addBlock(22,3,T_QBLOCK);
        spawnEnemy(E_GOOMBA,30);
        placeSemi(44,4,4); coinRow(46,3,3);
        placeGap(60,3);
        addBlock(72,3,T_BRICK); addBlock(74,3,T_QBLOCK); addBlock(76,3,T_BRICK);
        spawnEnemy(E_GOOMBA,86); spawnEnemy(E_GOOMBA,92);
        placePipe(104,2,false);
        coinRow(118,4,4);
        spawnEnemy(E_KOOPA,136);
        placeSemi(154,4,5); coinRow(156,3,4);
        placeGap(176,3);
        stairUp(192,4);
        break;
    case 1:
        addBlock(16,3,T_HIDDEN);
        coinRow(26,4,4);
        placeGap(44,3);
        placeSemi(56,4,4); coinRow(58,3,3);
        spawnEnemy(E_GOOMBA,72);
        placePipe(88,2,true);
        placeGap(114,3);
        addBlock(126,3,T_QBLOCK); addBlock(128,3,T_BRICK);
        coinRow(140,4,4);
        spawnEnemy(E_KOOPA,158);
        placeSemi(176,3,5);
        placeGap(198,3);
        stairUp(214,4);
        break;
    case 2:
        coinRow(12,4,3);
        placeGap(30,3);
        placeSemi(44,4,4);
        addBlock(60,3,T_QBLOCK);
        spawnEnemy(E_GOOMBA,74);
        placePipe(90,3,true);
        placeGap(114,4);
        stairUp(128,4); coinRow(140,2,4);
        spawnEnemy(E_KOOPA,152);
        placeSemi(168,3,6); coinRow(170,2,5);
        placeGap(192,3);
        addBlock(204,3,T_BRICK); addBlock(206,3,T_BRICK); addBlock(208,3,T_QBLOCK);
        stairUp(224,4);
        break;
    case 3:
        coinRow(14,4,4);
        spawnEnemy(E_GOOMBA,28);
        placeGap(44,3);
        placePipe(58,2,true);
        addBlock(76,3,T_QBLOCK);
        spawnEnemy(E_KOOPA,90);
        placeSemi(106,3,5); coinRow(108,2,4);
        placeGap(130,4);
        spawnEnemy(E_BILL,158);
        stairUp(170,5); coinRow(188,2,4);
        placeGap(208,3);
        placePlatform(216,4,6);
        spawnBoss(218);
        break;
    case 4:
        fillTiles(16,3,8,1,T_BRICK); coinRow(18,2,3);
        spawnEnemy(E_GOOMBA,36);
        placeGap(52,3);
        addBlock(66,3,T_QBLOCK); addBlock(68,3,T_BRICK); addBlock(70,3,T_BRICK);
        coinRow(82,4,4);
        placePipe(98,2,false);
        spawnEnemy(E_KOOPA,118);
        placeSemi(136,4,4);
        placeGap(158,3);
        fillTiles(172,3,6,1,T_BRICK); coinRow(174,2,2);
        spawnEnemy(E_GOOMBA,192);
        stairUp(214,4);
        break;
    case 5:
        addBlock(14,3,T_QBLOCK);
        coinRow(28,4,4);
        placePipe(46,3,true);
        placeGap(72,3);
        fillTiles(86,4,8,1,T_SEMI); coinRow(88,3,3);
        spawnEnemy(E_KOOPA,104);
        addBlock(122,3,T_BRICK); addBlock(124,3,T_BRICK);
        placeGap(144,4);
        spawnEnemy(E_GOOMBA,164); spawnEnemy(E_GOOMBA,172);
        placeSemi(186,4,5);
        stairUp(208,4);
        break;
    case 6:
        addBlock(18,3,T_HIDDEN);
        coinRow(32,4,4);
        placeGap(50,4);
        placeSemi(66,4,4);
        spawnEnemy(E_GOOMBA,80);
        placePipe(96,2,true);
        placeGap(120,3);
        addBlock(134,3,T_QBLOCK);
        coinRow(148,3,5);
        spawnEnemy(E_KOOPA,166);
        placeGap(186,4);
        stairUp(202,5); coinRow(216,2,4);
        break;
    case 7:
        coinRow(12,4,4);
        spawnEnemy(E_GOOMBA,28);
        placeGap(44,4);
        stairUp(58,4);
        addBlock(76,3,T_QBLOCK);
        spawnEnemy(E_KOOPA,90);
        placePipe(108,3,true);
        placeGap(134,4);
        spawnEnemy(E_BILL,156);
        placeSemi(168,3,6); coinRow(170,2,5);
        placeGap(196,4);
        spawnEnemy(E_BILL,216);
        placePlatform(224,4,6);
        spawnBoss(226);
        break;
    case 8:
        placeGap(20,3);
        coinRow(32,4,4);
        spawnEnemy(E_KOOPA,48);
        fillTiles(62,4,8,1,T_SOLID);
        addBlock(72,3,T_BRICK); addBlock(74,3,T_BRICK);
        placeGap(90,3);
        placeSemi(104,4,5); coinRow(106,3,4);
        spawnEnemy(E_GOOMBA,122);
        placeGap(142,4);
        spawnEnemy(E_BILL,164);
        stairUp(178,5); coinRow(192,2,4);
        break;
    case 9:
        addBlock(14,3,T_QBLOCK);
        coinRow(28,4,4);
        placeGap(46,3);
        placeSemi(60,4,4);
        spawnEnemy(E_KOOPA,76);
        placePipe(92,3,true);
        placeGap(116,4);
        fillTiles(130,4,10,1,T_SOLID); coinRow(132,3,4);
        spawnEnemy(E_GOOMBA,148);
        addBlock(162,3,T_BRICK); addBlock(164,3,T_QBLOCK);
        placeGap(180,4);
        spawnEnemy(E_BILL,202);
        stairUp(218,5);
        break;
    case 10:
        coinRow(10,4,4);
        placeGap(30,4);
        fillTiles(44,3,6,1,T_SOLID); coinRow(46,2,3);
        spawnEnemy(E_KOOPA,62);
        placePipe(78,2,true);
        placeGap(102,4);
        stairUp(116,4); coinRow(128,2,5);
        spawnEnemy(E_BILL,148);
        placeSemi(164,3,5);
        spawnEnemy(E_KOOPA,180);
        placeGap(198,4);
        addBlock(212,3,T_QBLOCK);
        stairUp(228,4);
        break;
    case 11:
        coinRow(12,4,5);
        placeGap(38,4);
        spawnEnemy(E_BILL,62);
        fillTiles(74,3,8,1,T_SOLID); coinRow(76,2,3);
        placePipe(94,3,true);
        spawnEnemy(E_KOOPA,116);
        placeGap(136,4);
        stairUp(150,5); coinRow(164,2,5);
        spawnEnemy(E_BILL,184);
        placeSemi(200,3,5);
        placeGap(222,4);
        placePlatform(230,4,6);
        spawnBoss(232);
        break;
    case 12:
        placeSemi(14,4,4); coinRow(16,3,3);
        placeSemi(30,4,3);
        spawnEnemy(E_GOOMBA,46);
        placeSemi(60,4,4); addBlock(64,3,T_QBLOCK);
        coinRow(74,3,4);
        placeSemi(88,4,4);
        spawnEnemy(E_KOOPA,102);
        placeSemi(116,4,5); coinRow(118,3,4);
        placePipe(134,2,true);
        placeSemi(150,3,5);
        spawnEnemy(E_BILL,170);
        placeSemi(186,3,6);
        stairUp(210,5);
        break;
    case 13:
        coinRow(12,4,4);
        placeSemi(28,4,4);
        spawnEnemy(E_GOOMBA,44);
        addBlock(58,3,T_HIDDEN);
        placeSemi(72,4,5); coinRow(74,3,4);
        spawnEnemy(E_KOOPA,90);
        placeSemi(106,3,5);
        addBlock(112,2,T_QBLOCK);
        coinRow(124,3,5);
        placePipe(142,2,true);
        spawnEnemy(E_BILL,164);
        placeSemi(180,3,5);
        spawnEnemy(E_KOOPA,198);
        placeSemi(214,3,5);
        stairUp(236,5);
        break;
    case 14:
        coinRow(14,4,5);
        placeSemi(32,4,4);
        spawnEnemy(E_GOOMBA,48);
        addBlock(62,3,T_QBLOCK);
        placeSemi(78,4,5);
        spawnEnemy(E_BILL,98);
        coinRow(114,3,4);
        placeSemi(130,3,5);
        spawnEnemy(E_KOOPA,146);
        placePipe(162,3,true);
        spawnEnemy(E_BILL,184);
        placeSemi(200,3,5); coinRow(202,2,5);
        stairUp(224,5);
        break;
    default:
        coinRow(10,4,5);
        spawnEnemy(E_BILL,32);
        placeSemi(50,4,4);
        spawnEnemy(E_GOOMBA,64); spawnEnemy(E_GOOMBA,70);
        addBlock(84,3,T_QBLOCK);
        placeSemi(100,3,5); coinRow(102,2,5);
        spawnEnemy(E_BILL,122);
        placeSemi(140,3,5);
        spawnEnemy(E_KOOPA,158);
        placePipe(176,3,true);
        spawnEnemy(E_BILL,198);
        coinRow(214,2,5);
        stairUp(228,5);
        placePlatform(236,4,8);
        spawnBoss(238);
        break;
    }
    placeFlag();
}

// ── GAME OBJECT LOGIC ────────────────────────────────────────
void collectCoin(int tx,int ty){
    uint8_t id=coinId[ty][tx];
    if(id<32) bSet(coinsTaken[levelIndex],id);
    setTile(tx,ty,T_AIR);
    totalCoins++; score+=10;
    if(totalCoins%10==0){ lives++; sfxExtraLife(); }
    else sfxCoin();
    saveProgress();
}

void bumpBlock(int tx,int ty){
    uint8_t t=getTile(tx,ty);
    if(!isBump(t)) return;
    uint8_t id=blockId[ty][tx];
    if(id<32) bSet(blocksUsed[levelIndex],id);
    spawnParticles(tileFP(tx),tileFP(ty),3);
    if(t==T_BRICK){
        if(powered||fireMode){ setTile(tx,ty,T_AIR); score+=5; }
        sfxBrick();
    } else {
        setTile(tx,ty,T_USED);
        totalCoins++; score+=10;
        if(levelIndex==14&&!starTimer){ starTimer=380; sfxPowerUp(); }
        else if(!powered){ powered=true; sfxPowerUp(); }
        else if(!fireMode&&levelIndex>=2){ fireMode=true; sfxPowerUp(); }
        else sfxCoin();
        saveProgress();
    }
}

void useReserve(){
    if(reserveItem==RI_NONE) return;
    switch(reserveItem){
        case RI_MUSH:   powered=true; break;
        case RI_FIRE:   powered=true; fireMode=true; break;
        case RI_STAR:   starTimer=380; break;
        case RI_SHIELD: shielded=true; shieldTimer=300; break;
        default: break;
    }
    reserveItem=RI_NONE;
    sfxPowerUp();
    saveProgress();
}

void damagePlayer(bool pit){
    if(!pit&&starTimer>0)   return;
    if(!pit&&invulnTimer>0) return;
    if(!pit&&shielded){ shielded=false; shieldTimer=0; invulnTimer=90; sfxHit(); return; }
    if(!pit&&(powered||fireMode)){
        if(fireMode) fireMode=false; else powered=false;
        invulnTimer=90; sfxHit(); return;
    }
    lives--;
    sfxDie();
    if(lives<=0){
        gameOver=true;
        saveScore(selectedGame,(int)currentDiff,score);
        // Full clean-slate: wipe all Mario saves so next play starts fresh from the beginning
        resetProgress();
        return;
    }
    px=pxFP(2*TILE); py=pxFP((TROWS-1)*TILE-PH);
    vx=vy=0; invulnTimer=110;
    buildLevel();
    saveProgress();
}

bool landOnSemi(int oldY){
    if(vy<=0) return false;
    int oldBot=oldY+PH*FP, newBot=py+PH*FP;
    int tx0=fpPx(px+FP)/TILE, tx1=fpPx(px+(PW-2)*FP)/TILE;
    int ty=fpPx(newBot)/TILE;
    if(ty<0||ty>=TROWS) return false;
    int tTopPx=ty*TILE;
    if(fpPx(oldBot)>tTopPx+FP) return false;
    for(int tx=tx0;tx<=tx1;tx++){
        if(getTile(tx,ty)==T_SEMI){
            py=pxFP(tTopPx-PH);
            vy=0; onGround=true; return true;
        }
    }
    return false;
}

void resolveX(){
    px+=vx;
    if(!hitSolid(px,py,PW,PH)) return;
    if(vx>0){ int tx=(fpPx(px)+PW-1)/TILE; px=pxFP(tx*TILE-PW); }
    else     { int tx=fpPx(px)/TILE;        px=pxFP((tx+1)*TILE); }
    vx=0;
}

void resolveY(){
    int oldY=py; py+=vy; onGround=false;
    if(landOnSemi(oldY)) return;
    if(!hitSolid(px,py,PW,PH)) return;
    if(vy>0){
        int ty=(fpPx(py)+PH-1)/TILE;
        py=pxFP(ty*TILE-PH); vy=0; onGround=true;
    } else {
        int ty=fpPx(py)/TILE;
        int tx0=fpPx(px+FP)/TILE, tx1=fpPx(px+(PW-2)*FP)/TILE;
        for(int tx=tx0;tx<=tx1;tx++) bumpBlock(tx,ty);
        py=pxFP((ty+1)*TILE); vy=0;
    }
}

void touchLevelTiles(){
    int x0=fpPx(px),y0=fpPx(py),x1=x0+PW-1,y1=y0+PH-1;
    for(int ty=y0/TILE;ty<=y1/TILE;ty++){
        for(int tx=x0/TILE;tx<=x1/TILE;tx++){
            uint8_t t=getTile(tx,ty);
            if(t==T_COIN){ collectCoin(tx,ty); continue; }
            if(t==T_HARM){ damagePlayer(false); return; }
            if(t==T_FLAG_P||t==T_FLAG_T){
                score+=500*(levelIndex+1);
                sfxFlag();
                if(levelIndex+1<LEVELS&&levelIndex+1>unlockedLevel)
                    unlockedLevel=levelIndex+1;
                if(score>highScores[selectedGame][(int)currentDiff]){
                    saveScore(selectedGame,(int)currentDiff,score);
                    newRecord=true;
                }
                mode=M_SHOP; shopCursor=0;
                lastShopMove=millis();
                saveProgress(); return;
            }
        }
    }
}

void shootFireball(){
    if(!fireMode) return;
    for(int i=0;i<MAX_FB;i++){
        if(fireballs[i].active) continue;
        fireballs[i]={px+(facingRight?PW*FP:0), py+pxFP(6),
                      facingRight?650:-650, -290, true};
        sfxShoot_m(); return;
    }
}

static bool overlap(int ax,int ay,int aw,int ah,
                    int bx,int by,int bw,int bh){
    ax=fpPx(ax);ay=fpPx(ay);bx=fpPx(bx);by=fpPx(by);
    return ax<bx+bw&&ax+aw>bx&&ay<by+bh&&ay+ah>by;
}
static int eH(const Enemy& e){ return (e.type==E_KOOPA||e.type==E_BOSS)?14:8; }

void killEnemy(int i){
    spawnParticles(enemies[i].x,enemies[i].y,5);
    enemies[i].alive=false;
    comboCounter=min(comboCounter+1,8);
    score+=100*comboCounter;
    sfxStomp();
}

void hitBoss(int i){
    Enemy& e=enemies[i];
    e.hp--;
    spawnParticles(e.x,e.y,4);
    if(e.hp<=0){
        bossHp=0;
        spawnParticles(e.x,e.y,12);
        e.alive=false;
        score+=1000;
        sfxBossDie();
    } else {
        bossHp=e.hp;
        e.vx=e.vx>0?e.vx+30:e.vx-30;
        sfxBoss();
    }
}

void updateEnemies(){
    for(int i=0;i<MAX_E;i++){
        Enemy& e=enemies[i];
        if(!e.alive) continue;

        if(e.type==E_PIRANHA){
            e.timer++;
            int ph=e.timer%120; if(ph>60) ph=120-ph;
            e.y=e.baseY-(ph/5)*FP;

        } else if(e.type==E_BILL){
            if(fpPx(e.x)>camX+SW+20) continue;
            e.x+=e.vx;
            if(fpPx(e.x)<camX-24) e.alive=false;

        } else if(e.type==E_BOSS){
            e.vy+=70; if(e.vy>900) e.vy=900;
            e.y+=e.vy;
            if(hitSolid(e.x,e.y,14,14)){
                int ty=(fpPx(e.y)+13)/TILE;
                e.y=pxFP(ty*TILE-14); e.vy=0;
            }
            int nx=e.x+e.vx; bool turn=hitSolid(nx,e.y,14,14);
            if(!turn){
                int ah=fpPx(nx+(e.vx>0?pxFP(14):-FP))/TILE;
                int ft=(fpPx(e.y)+14)/TILE;
                if(!isSolid(getTile(ah,ft))) turn=true;
            }
            if(turn) e.vx=-e.vx; else e.x=nx;
            e.timer++;
            if(e.timer%90==0&&enemyCount<MAX_E){
                Enemy& b=enemies[enemyCount++];
                int spd=(e.vx>0)?-160:160;
                b={e.x,e.y,spd,0,0,1,E_BILL,0,0,true};
            }

        } else {
            e.vy+=70; if(e.vy>900) e.vy=900;
            e.y+=e.vy;
            if(hitSolid(e.x,e.y,8,eH(e))){
                int ty=(fpPx(e.y)+eH(e)-1)/TILE;
                e.y=pxFP(ty*TILE-eH(e)); e.vy=0;
            }
            int nx=e.x+e.vx; bool turn=hitSolid(nx,e.y,8,eH(e));
            if(!turn){
                int ah=fpPx(nx+(e.vx>0?pxFP(8):-FP))/TILE;
                int ft=(fpPx(e.y)+eH(e))/TILE;
                if(!isSolid(getTile(ah,ft))&&getTile(ah,ft)!=T_SEMI) turn=true;
            }
            if(turn) e.vx=-e.vx; else e.x=nx;
        }

        if(!overlap(px,py,PW,PH,e.x,e.y,
                   (e.type==E_BOSS?14:8),eH(e))) continue;

        if(starTimer>0){
            if(e.type==E_BOSS) hitBoss(i); else killEnemy(i);
            continue;
        }
        bool stomp=(vy>80)&&(fpPx(py+PH*FP)<=fpPx(e.y)+eH(e)/2);

        if(e.type==E_BOSS){
            if(stomp){ hitBoss(i); vy=-700; }
            else      { damagePlayer(false); }
        } else if(stomp&&e.type!=E_PIRANHA&&e.type!=E_BILL){
            if(e.type==E_KOOPA){
                if(e.shell==0){ e.shell=1; e.vx=0; e.y+=pxFP(4); score+=100; sfxStomp(); }
                else          { e.vx=(facingRight?560:-560); score+=100; sfxStomp(); }
            } else killEnemy(i);
            vy=-700;
        } else {
            damagePlayer(false);
        }
    }
}

void updateFireballs(){
    for(int i=0;i<MAX_FB;i++){
        Fireball& f=fireballs[i];
        if(!f.active) continue;
        f.vy+=55; f.x+=f.vx;
        if(hitSolid(f.x,f.y,3,3)){ f.active=false; continue; }
        f.y+=f.vy;
        if(hitSolid(f.x,f.y,3,3)) f.vy=-370;
        if(fpPx(f.x)<camX-12||fpPx(f.x)>camX+SW+12){ f.active=false; continue; }
        for(int ei=0;ei<MAX_E;ei++){
            if(!enemies[ei].alive) continue;
            if(overlap(f.x,f.y,3,3,enemies[ei].x,enemies[ei].y,
                      (enemies[ei].type==E_BOSS?14:8),eH(enemies[ei]))){
                f.active=false;
                if(enemies[ei].type==E_BOSS) hitBoss(ei);
                else killEnemy(ei);
                break;
            }
        }
    }
}

void updateCamera(){
    int spx=fpPx(px)-camX;
    if(spx>62) camX=fpPx(px)-62;
    if(spx<26) camX=fpPx(px)-26;
    camX=constrain(camX,0,LW*TILE-SW);
}

// ── PLAYER UPDATE ────────────────────────────────────────────
void updatePlayer(){
    if(invulnTimer>0) invulnTimer--;
    if(starTimer>0)   starTimer--;
    if(shieldTimer>0) shieldTimer--;
    else shielded=false;

    // Button mapping:
    //   btnJoySW (Joy SW, pin 25) → use reserve item
    //   btnB                      → run / fireball
    //   btnA                      → jump
    //   Pause handled by RETROBOX STATE_PAUSED via btnStart
    if(btnJoySW.pressed) useReserve();
    if(btnB.pressed) shootFireball();

    ducking=joyDown()&&onGround;

    int walk=diffHard()?425:(diffEasy()?320:WALK_SPD);
    int run =diffHard()?670:(diffEasy()?540:RUN_SPD);
    int target=0;
    if(!ducking){
        if(joyLeft()) { target=btnB.held?-run:-walk; facingRight=false; }
        if(joyRight()){ target=btnB.held? run: walk; facingRight=true; }
    }
    int acc=onGround?ACC_GND:ACC_AIR;
    if(vx<target){ vx+=acc; if(vx>target) vx=target; }
    if(vx>target){ vx-=acc; if(vx<target) vx=target; }
    if(target==0&&onGround) vx=(vx*FRICTION)/100;
    if(abs(vx)<10) vx=0;

    vy+=GRAV; if(vy>MAX_FALL) vy=MAX_FALL;

    // Coyote time: grant a short jump window after walking off a ledge.
    // Also lets a jump press that lands 1-2 frames BEFORE touching ground fire immediately on landing.
    if(onGround)       coyoteFrames=4;  // reset to 4 frames of grace
    else if(coyoteFrames>0) coyoteFrames--;

    if(btnA.pressed&&coyoteFrames>0&&!ducking){
        vy=J_IMPULSE;
        onGround=false; jumpHeld=true; coyoteFrames=0;
        sfxJump();
    }
    if(!btnA.held) jumpHeld=false;
    if(!jumpHeld&&vy<J_CUT) vy=J_CUT;

    resolveX();
    resolveY();
    touchLevelTiles();

    if(onGround&&!touchType(T_HARM,px,py+PH*FP,PW,FP)){
        safeX=px; safeY=py;
        comboCounter=0;
    }
    if(fpPx(px)<0){ px=0; vx=0; }
    if(fpPx(py)>TROWS*TILE+20) damagePlayer(true);

    walkFrame=(millis()/130)&1;
}

// ── SHOP ─────────────────────────────────────────────────────
void shopBuy(){
    int cost=SHOP[shopCursor].cost;
    if(totalCoins<cost){ sfxHit(); return; }
    totalCoins-=cost;
    switch(shopCursor){
        case 0: lives++;                         break;
        case 1: reserveItem=RI_MUSH;             break;
        case 2: reserveItem=RI_FIRE;             break;
        case 3: reserveItem=RI_STAR;             break;
        case 4: reserveItem=RI_SHIELD;           break;
        case 5: lives+=2;                        break;
        case 6: mapReveal=true;                  break;
        case 7: score+=50; totalCoins+=5;        break;
    }
    sfxBuy();
    saveProgress();
}

void nextLevelOrEnd(){
    if(levelIndex>=LEVELS-1){
        endlessCycle++;
        levelIndex=0;
        startLevel();
        saveProgress(); return;
    }
    levelIndex++;
    if(levelIndex>unlockedLevel) unlockedLevel=levelIndex;
    startLevel();
    saveProgress();
}

void updateShop(){
    unsigned long now=millis();
    if(now-lastShopMove>155){
        if(joyLeft() &&shopCursor>0)          { shopCursor--; sfxMenu(); lastShopMove=now; }
        if(joyRight()&&shopCursor<SHOP_ITEMS-1){ shopCursor++; sfxMenu(); lastShopMove=now; }
    }
    if(btnA.pressed) shopBuy();
    if(btnB.pressed) nextLevelOrEnd();
}

// ── START LEVEL ──────────────────────────────────────────────
void startLevel(){
    buildLevel();
    px=pxFP(2*TILE); py=pxFP((TROWS-1)*TILE-PH);
    safeX=px; safeY=py;
    vx=vy=camX=0;
    onGround=facingRight=true;
    ducking=jumpHeld=false; coyoteFrames=0;
    invulnTimer=80; starTimer=0; shieldTimer=0;
    walkFrame=shopCursor=comboCounter=0;
    mode=M_PLAY;
}

// ── PUBLIC API ───────────────────────────────────────────────
// init() is called by RETROBOX's gameInit() dispatch.
// Loads persistent save and starts at the last unlocked level.
void init(){
    // LEDC channel 0 is already set up by RETROBOX's setup().
    loadProgress();
    score=0; gameOver=false; newRecord=false;
    powered=false; fireMode=false; shielded=false;
    levelIndex=unlockedLevel;
    lastShopMove=0;
    startLevel();
}

// ── UPDATE ───────────────────────────────────────────────────
void update(){
    sfxTick();
    if(gameOver) return;
    if(mode==M_SHOP) { updateShop(); return; }
    updatePlayer();
    updateEnemies();
    updateFireballs();
    updateParticles();
    updateCamera();
    // Keep best score persisted to NVS whenever it increases mid-run.
    // saveScore() guards with score>highScores so NVS writes only happen on new records.
    if(score > highScores[selectedGame][(int)currentDiff]){
        saveScore(selectedGame,(int)currentDiff,score);
        newRecord=true;
    }
}

// ── DRAWING ──────────────────────────────────────────────────
void drawTile(int sx,int sy,uint8_t t){
    if(t==T_HIDDEN&&!mapReveal) return;
    switch(t){
    case T_SOLID:
        display.fillRect(sx,sy,TILE,TILE,SSD1306_WHITE);
        display.drawLine(sx+1,sy+1,sx+TILE-2,sy+1,SSD1306_BLACK);
        display.drawLine(sx+1,sy+1,sx+1,sy+TILE-2,SSD1306_BLACK);
        break;
    case T_BRICK:
        display.fillRect(sx,sy,TILE,TILE,SSD1306_WHITE);
        display.drawFastHLine(sx,sy+3,TILE,SSD1306_BLACK);
        display.drawFastHLine(sx,sy+6,TILE,SSD1306_BLACK);
        display.drawFastVLine(sx+4,sy,  3,SSD1306_BLACK);
        display.drawFastVLine(sx+2,sy+3,3,SSD1306_BLACK);
        display.drawFastVLine(sx+6,sy+3,3,SSD1306_BLACK);
        break;
    case T_QBLOCK:
        display.fillRect(sx,sy,TILE,TILE,SSD1306_WHITE);
        display.drawRect(sx+1,sy+1,TILE-2,TILE-2,SSD1306_BLACK);
        display.drawPixel(sx+4,sy+2,SSD1306_BLACK);
        display.drawPixel(sx+3,sy+4,SSD1306_BLACK);
        display.drawPixel(sx+4,sy+4,SSD1306_BLACK);
        display.drawPixel(sx+4,sy+6,SSD1306_BLACK);
        break;
    case T_USED:
        display.drawRect(sx,sy,TILE,TILE,SSD1306_WHITE);
        display.drawPixel(sx+2,sy+3,SSD1306_WHITE);
        display.drawPixel(sx+5,sy+5,SSD1306_WHITE);
        break;
    case T_COIN:{
        bool blink=(millis()/200)&1;
        if(blink) display.drawCircle(sx+4,sy+4,3,SSD1306_WHITE);
        else      display.fillRect(sx+3,sy+1,2,6,SSD1306_WHITE);
        break;}
    case T_PIPE_T:
        display.fillRect(sx-1,sy,TILE+2,TILE,SSD1306_WHITE);
        display.fillRect(sx+1,sy+2,TILE-2,3,SSD1306_BLACK);
        break;
    case T_PIPE_B:
        display.fillRect(sx,sy,TILE,TILE,SSD1306_WHITE);
        display.drawFastVLine(sx+1,sy,TILE,SSD1306_BLACK);
        display.drawFastVLine(sx+6,sy,TILE,SSD1306_BLACK);
        break;
    case T_SEMI:
        display.fillRect(sx,sy,TILE,3,SSD1306_WHITE);
        break;
    case T_HARM:
        display.drawTriangle(sx,sy+7,sx+4,sy+1,sx+7,sy+7,SSD1306_WHITE);
        break;
    case T_FLAG_T:
    case T_FLAG_P:
        display.drawFastVLine(sx+3,sy,TILE,SSD1306_WHITE);
        if(t==T_FLAG_T) display.fillRect(sx+3,sy,5,4,SSD1306_WHITE);
        break;
    case T_HIDDEN:
        display.drawRect(sx,sy,TILE,TILE,SSD1306_WHITE);
        display.drawPixel(sx+4,sy+4,SSD1306_WHITE);
        break;
    default: break;
    }
}

void drawPlayer(int sx,int sy){
    if(invulnTimer>0&&((invulnTimer/4)&1)) return;
    int h=ducking?10:PH;
    int yo=sy+PH-h;
    if(starTimer>0&&((millis()/70)&1))
        display.drawRect(sx-1,yo-1,PW+2,h+2,SSD1306_WHITE);
    if(shielded&&((millis()/100)&1))
        display.drawCircle(sx+4,yo+h/2,h/2+2,SSD1306_WHITE);
    display.fillRect(sx+1,yo,  6,2,SSD1306_WHITE);
    display.fillRect(sx,  yo+2,8,5,SSD1306_WHITE);
    display.drawPixel(sx+(facingRight?5:2),yo+4,SSD1306_BLACK);
    display.fillRect(sx+1,yo+7,6,h-9,SSD1306_WHITE);
    if(fireMode) display.drawPixel(sx+3,yo+9,SSD1306_BLACK);
    if(!ducking){
        if(onGround&&walkFrame){
            display.fillRect(sx,  yo+h-2,3,2,SSD1306_WHITE);
            display.fillRect(sx+5,yo+h-2,3,2,SSD1306_WHITE);
        } else {
            display.fillRect(sx+1,yo+h-2,2,2,SSD1306_WHITE);
            display.fillRect(sx+5,yo+h-2,2,2,SSD1306_WHITE);
        }
    }
}

void drawEnemy(const Enemy& e){
    int sx=fpPx(e.x)-camX;
    int sy=PT+fpPx(e.y);
    if(sx<-16||sx>SW+16) return;
    switch(e.type){
    case E_GOOMBA:
        display.fillRect(sx+1,sy+2,6,5,SSD1306_WHITE);
        display.fillRect(sx,  sy+5,8,2,SSD1306_WHITE);
        display.drawPixel(sx+2,sy+3,SSD1306_BLACK);
        display.drawPixel(sx+5,sy+3,SSD1306_BLACK);
        if((millis()/200)&1) display.fillRect(sx,  sy+7,3,1,SSD1306_WHITE);
        else                 display.fillRect(sx+5,sy+7,3,1,SSD1306_WHITE);
        break;
    case E_KOOPA:
        if(e.shell){
            display.fillRect(sx,sy+6,8,8,SSD1306_WHITE);
            display.drawFastHLine(sx+1,sy+9,6,SSD1306_BLACK);
            display.drawFastVLine(sx+4,sy+6,8,SSD1306_BLACK);
        } else {
            display.fillRect(sx+1,sy,  6,6,SSD1306_WHITE);
            display.drawRect (sx,  sy+6,8,8,SSD1306_WHITE);
            display.drawPixel(sx+5,sy+2,SSD1306_BLACK);
        }
        break;
    case E_PIRANHA:
        display.fillRect(sx+1,sy,  6,8,SSD1306_WHITE);
        display.drawLine(sx+1,sy+3,sx+6,sy+3,SSD1306_BLACK);
        display.drawPixel(sx+3,sy+1,SSD1306_BLACK);
        display.drawPixel(sx+5,sy+1,SSD1306_BLACK);
        break;
    case E_BILL:
        display.fillRect(sx,  sy+2,8,4,SSD1306_WHITE);
        display.fillRect(sx+6,sy+1,2,6,SSD1306_WHITE);
        display.drawPixel(sx+2,sy+4,SSD1306_BLACK);
        break;
    case E_BOSS:{
        bool flash=(e.hp>0&&((millis()/80)&1));
        if(!flash){
            display.fillRect(sx,sy,14,14,SSD1306_WHITE);
            display.drawRect(sx+1,sy+1,12,12,SSD1306_BLACK);
            display.fillRect(sx+2,sy+3,3,3,SSD1306_WHITE);
            display.fillRect(sx+9,sy+3,3,3,SSD1306_WHITE);
            display.drawPixel(sx+3,sy+4,SSD1306_BLACK);
            display.drawPixel(sx+10,sy+4,SSD1306_BLACK);
            int barW=12*bossHp/max(1,bossMaxHp);
            display.drawRect(sx,sy-4,12,3,SSD1306_WHITE);
            if(barW>0) display.fillRect(sx+1,sy-3,barW,1,SSD1306_WHITE);
        }
        break;}
    default: break;
    }
}

void drawBackground(){
    int world=levelIndex/4;
    int par=camX/(world+2);
    for(int i=0;i<4;i++){
        int bx=((i*36+SW-par%SW))%SW;
        int by=PT+4+(i*7)%14;
        switch(world){
        case 0:
            display.drawCircle(bx+3,by,  3,SSD1306_WHITE);
            display.drawCircle(bx+7,by-2,2,SSD1306_WHITE);
            display.drawCircle(bx+10,by, 3,SSD1306_WHITE);
            break;
        case 1:
            display.drawTriangle(bx,PT,bx+3,PT+5,bx+6,PT,SSD1306_WHITE);
            break;
        case 2:
            display.drawRect(bx,by,5,3,SSD1306_WHITE);
            break;
        case 3:
            display.drawPixel(bx,  by,  SSD1306_WHITE);
            display.drawPixel(bx+4,by+4,SSD1306_WHITE);
            display.drawPixel(bx+8,by,  SSD1306_WHITE);
            if((millis()/400)&1)
                display.drawPixel(bx+2,by+2,SSD1306_WHITE);
            break;
        }
    }
}

void drawParticles(){
    for(const auto& p:particles){
        if(!p.active) continue;
        int sx=fpPx(p.x)-camX, sy=PT+fpPx(p.y);
        if(sx>=0&&sx<SW&&sy>=PT&&sy<SH)
            display.drawPixel(sx,sy,SSD1306_WHITE);
    }
}

static const char* resName(){
    switch(reserveItem){
        case RI_MUSH:   return "M";
        case RI_FIRE:   return "F";
        case RI_STAR:   return "*";
        case RI_SHIELD: return "S";
        default:        return "-";
    }
}

void drawHUD(){
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    char buf[24];
    int w=levelIndex/4+1+endlessCycle*4, l=levelIndex%4+1;
    sprintf(buf,"Mx%02d C%03d W%d-%d",lives,totalCoins,w,l);
    display.print(buf);
    display.setCursor(116,0); display.print(resName());
    if(starTimer>0){ display.setCursor(108,0); display.print("*"); }
    if(shielded)   { display.setCursor(100,0); display.print("S"); }
    display.drawFastHLine(0,HUD_H-1,SW,SSD1306_WHITE);
}

void drawShop(){
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.fillRect(0,0,SW,9,SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(30,1); display.print("  ITEM SHOP  ");
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(2,12);
    display.print("Coins: "); display.print(totalCoins);

    display.drawRect(0,21,SW,18,SSD1306_WHITE);
    display.setCursor(4,24);
    display.print(SHOP[shopCursor].name);
    display.print(" - "); display.print(SHOP[shopCursor].desc);
    display.setCursor(4,32);
    display.print("Cost: "); display.print(SHOP[shopCursor].cost); display.print("C");

    int start=max(0,shopCursor-1);
    if(start>SHOP_ITEMS-4) start=SHOP_ITEMS-4;
    for(int i=0;i<4&&start+i<SHOP_ITEMS;i++){
        int idx=start+i;
        int tx=2+i*31;
        bool sel=(idx==shopCursor);
        if(sel) display.fillRect(tx,41,28,13,SSD1306_WHITE);
        display.setTextColor(sel?SSD1306_BLACK:SSD1306_WHITE);
        display.setCursor(tx+2,43);
        display.print(SHOP[idx].icon);
        display.setCursor(tx+2,50);
        char tmp[6]; sprintf(tmp,"%d",SHOP[idx].cost);
        display.print(tmp);
        display.setTextColor(SSD1306_WHITE);
    }
    if(start>0){              display.setCursor(0,46); display.print("<"); }
    if(start+4<SHOP_ITEMS){   display.setCursor(120,46); display.print(">"); }

    display.setCursor(4,58);
    display.print("A=buy  B=next lvl");
    display.display();
}

void draw(){
    if(mode==M_SHOP) { drawShop(); return; }

    display.clearDisplay();
    drawBackground();
    drawHUD();

    int first=camX/TILE;
    for(int y=0;y<TROWS;y++){
        for(int col=0;col<=TCOLS+1;col++){
            int tx=first+col;
            if(tx<0||tx>=LW) continue;
            int sx=tx*TILE-camX;
            uint8_t t=getTile(tx,y);
            if(t!=T_AIR) drawTile(sx,PT+y*TILE,t);
        }
    }
    drawParticles();
    for(const auto& f:fireballs)
        if(f.active) display.fillCircle(fpPx(f.x)-camX,PT+fpPx(f.y),1,SSD1306_WHITE);
    for(const auto& e:enemies)
        if(e.alive) drawEnemy(e);
    drawPlayer(fpPx(px)-camX, PT+fpPx(py));
    display.display();
}

bool isOver()       { return gameOver; }
int  getScore()     { return score; }
bool isNewRecord()  { return newRecord; }

} // namespace MarioGame
