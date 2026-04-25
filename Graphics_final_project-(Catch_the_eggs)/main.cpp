#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/glut.h>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

const int WIN_W = 800;
const int WIN_H = 600;

// ─── Constants─────────
const int   NUM_STICKS = 2;
const float STICK_Y[2] = { 540.f, 500.f }; // y positions of sticks
const float STICK_X1 = 50.f;
const float STICK_X2 = 750.f;
const float BASKET_Y = 60.f;
const float BASKET_W_DEF = 80.f;
const float BASKET_H = 30.f;
const float EGG_R = 12.f;
const float PERK_W = 22.f;
const float PERK_H = 22.f;
const int   GAME_DURATION = 120;
const float BASE_FALL_SPD = 2.8f;


enum EggType { EGG_NORMAL, EGG_GOLD, EGG_BLUE, EGG_POOP };
int eggPoints(EggType t) {
    switch(t) {
        case EGG_NORMAL: return 1;
        case EGG_GOLD:   return 10;
        case EGG_BLUE:   return 5;
        case EGG_POOP:   return -10;
        default:         return 0;
    }
}

enum PerkType { PERK_WIDE, PERK_SLOW, PERK_TIME, PERK_SHIELD, PERK_DOUBLE };

struct FallingObject {
    float x, y;
    float vx, vy;
    EggType eggType;
    bool   isPerk;
    PerkType perkType;
    bool   active;
    int    stickIdx;
};

struct Chicken {
    float x;
    float dir;   // +1 or -1
    float speed;
};

struct Particle {
    float x, y, vx, vy;
    float r, g, b;
    float life;   // 0..1
    bool  active;
};

struct Airflow {
    float strength;  // horizontal push per frame
    float timer;     // remaining frames
    bool  active;
};

Chicken chickens[NUM_STICKS];
std::vector<FallingObject> objects;
std::vector<Particle> particles;
Airflow airflow = {0, 0, false};



float basketX = WIN_W / 2.0f;
float basketW = BASKET_W_DEF;
float fallSpeed = BASE_FALL_SPD;
int   score = 0;
int   highScore = 0;
int   timeLeft = GAME_DURATION;
int   lastTimeSec = 0;

bool  shieldActive = false;
float shieldTimer = 0;
bool  doublePoints = false;
float doubleTimer = 0;
bool  wideActive = false;
float wideTimer = 0;
bool  slowActive = false;
float slowTimer = 0;

int   menuSelected    = 0; // 0=Start,1=HighScore,2=Help,3=Exit
int   pauseSelected   = 0;

float randF(float lo, float hi) {
    return lo + (hi - lo) * (rand() / (float)RAND_MAX);
}

void spawnEgg(int stickIdx) {
    FallingObject o;
    o.stickIdx = stickIdx;
    o.x  = chickens[stickIdx].x;
    o.y  = STICK_Y[stickIdx] - 10.f;
    o.vx = 0;
    o.vy = -(fallSpeed + randF(0, 1.0f));
    o.active  = true;
    o.isPerk  = false;

    float r = randF(0, 1.0f);
    if      (r < 0.05f) o.eggType = EGG_GOLD;
    else if (r < 0.20f) o.eggType = EGG_BLUE;
    else if (r < 0.30f) o.eggType = EGG_POOP;
    else                o.eggType = EGG_NORMAL;

    objects.push_back(o);
}

void spawnPerk() {
    FallingObject o;
    o.x  = randF(60, WIN_W - 60);
    o.y  = WIN_H - 20.f;
    o.vx = 0;
    o.vy = -(BASE_FALL_SPD * 0.7f);
    o.active  = true;
    o.isPerk  = true;
    o.stickIdx = 0;

    float r = randF(0, 1.0f);
    if      (r < 0.25f) o.perkType = PERK_WIDE;
    else if (r < 0.50f) o.perkType = PERK_SLOW;
    else if (r < 0.70f) o.perkType = PERK_TIME;
    else if (r < 0.85f) o.perkType = PERK_SHIELD;
    else                o.perkType = PERK_DOUBLE;

    objects.push_back(o);
}

void spawnParticles(float x, float y, float r, float g, float b, int n = 12) {
    for (int i = 0; i < n; i++) {
        Particle p;
        p.x = x; p.y = y;
        float angle = randF(0, 2 * 3.14159f);
        float speed = randF(1, 4);
        p.vx = cosf(angle) * speed;
        p.vy = sinf(angle) * speed;
        p.r = r; p.g = g; p.b = b;
        p.life = 1.0f;
        p.active = true;
        particles.push_back(p);
    }
}


void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}
void setColor(float r, float g, float b, float a = 1.0f) {
    glColor4f(r, g, b, a);
}

void drawRect(float x, float y, float w, float h) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void drawCircle(float cx, float cy, float r, int segs = 32) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segs; i++) {
        float a = i * 2 * 3.14159f / segs;
        glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    glEnd();
}

void drawEllipse(float cx, float cy, float rx, float ry, int segs = 32) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segs; i++) {
        float a = i * 2 * 3.14159f / segs;
        glVertex2f(cx + cosf(a) * rx, cy + sinf(a) * ry);
    }
    glEnd();
}

void drawText(float x, float y, const std::string& s, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (char c : s) glutBitmapCharacter(font, c);
}

void drawTextLarge(float x, float y, const std::string& s) {
    drawText(x, y, s, GLUT_BITMAP_TIMES_ROMAN_24);
}


void drawBackground() {
    // Sky gradient
    glBegin(GL_QUADS);
    setColor(0.2f, 0.3f, 1.0f);
    glVertex2f(0, WIN_H);
    glVertex2f(WIN_W, WIN_H);
    setColor(0.9f, 0.9f, .7f);
    glVertex2f(WIN_W, 120);
    glVertex2f(0, 120);
    glEnd();

    // Ground
    setColor(0.3f, 0.7f, 0.2f);
    drawRect(0, 0, WIN_W, 120);

    // Darker ground strip
    setColor(0.25f, 0.55f, 0.15f);
    drawRect(0, 0, WIN_W, 50);

    // Clouds
    setColor(.8, 1, 1);
    auto cloud = [](float cx, float cy) {
        drawEllipse(cx, cy, 40, 20);
        drawEllipse(cx + 30, cy + 5, 30, 18);
        drawEllipse(cx - 30, cy + 5, 30, 18);
        drawEllipse(cx + 10, cy + 15, 25, 15);
        };
    cloud(150, 480);
    cloud(450, 510);
    cloud(650, 460);

    // Sun
    setColor(0.9f, 0.99f, 0.3f);
    drawCircle(WIN_W - 60, WIN_H - 60, 35);
}

void drawStick(float y) {
    // Stick body
    setColor(0.3f, 0.5f, 0.2f);
    drawRect(STICK_X1, y - 4, STICK_X2 - STICK_X1, 9);

    // Bamboo nodes
    setColor(0.5f, 0.7f, 0.08f);
    for (float x = STICK_X1 + 80; x < STICK_X2; x += 80) {
        drawRect(x - 3, y - 7, 6, 14);
    }
}


void drawEgg(float cx, float cy, EggType t) {
    switch(t) {
        case EGG_GOLD:
            setColor(1.0f, 0.85f, 0.1f);
            drawEllipse(cx, cy, EGG_R, EGG_R * 1.3f);
            setColor(1.0f, 1.0f, 0.6f);
            drawEllipse(cx - 3, cy + 4, EGG_R * 0.4f, EGG_R * 0.5f);
            // Star sparkle
            setColor(1.0f, 1.0f, 1.0f);
            drawCircle(cx + 5, cy + 8, 2);
            break;
        case EGG_BLUE:
            setColor(0.2f, 0.5f, 1.0f);
            drawEllipse(cx, cy, EGG_R, EGG_R * 1.3f);
            setColor(0.6f, 0.8f, 1.0f);
            drawEllipse(cx - 3, cy + 4, EGG_R * 0.4f, EGG_R * 0.5f);
            break;
        case EGG_POOP:
            setColor(0.45f, 0.30f, 0.10f);
            // Poop swirl shape
            drawCircle(cx, cy, EGG_R * 0.9f);
            setColor(0.55f, 0.38f, 0.15f);
            drawCircle(cx, cy + 8, EGG_R * 0.7f);
            setColor(0.60f, 0.42f, 0.18f);
            drawCircle(cx, cy + 14, EGG_R * 0.5f);
            // Stink lines
            setColor(0.7f, 0.85f, 0.3f);
            glLineWidth(1.5f);
            glBegin(GL_LINES);
            glVertex2f(cx - 10, cy + 20); glVertex2f(cx - 14, cy + 30);
            glVertex2f(cx,      cy + 22); glVertex2f(cx,      cy + 32);
            glVertex2f(cx + 10, cy + 20); glVertex2f(cx + 14, cy + 30);
            glEnd();
            glLineWidth(1.0f);
            break;
        default: // EGG_NORMAL
            setColor(0.97f, 0.97f, 0.9f);
            drawEllipse(cx, cy, EGG_R, EGG_R * 1.3f);
            setColor(1.0f, 1.0f, 1.0f);
            drawEllipse(cx - 3, cy + 4, EGG_R * 0.35f, EGG_R * 0.45f);
            break;
    }
}

// ─── Draw chicken ─────────────────────────────────────────────────────────────
void drawChicken(float cx, float cy, bool facingRight) {
    float flip = facingRight ? 1.0f : -1.0f;

    // Body
    setColor(0.9f, 0.9f, 0.8f);
    drawEllipse(cx, cy, 22, 18);

    // Head
    setColor(0.95f, 0.9f, 0.9f);
    drawCircle(cx + flip * 18, cy + 12, 12);

    // Eye
    setColor(0.1f, 0.1f, 0.1f);
    drawCircle(cx + flip * 21, cy + 14, 2.5f);

    // Beak
    setColor(0.7f, 0.3f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx + flip * 28, cy + 12);
    glVertex2f(cx + flip * 34, cy + 14);
    glVertex2f(cx + flip * 28, cy + 20);
    glEnd();

    // Comb (red)
    setColor(0.7f, 0.1f, 0.1f);
    drawCircle(cx + flip * 16, cy + 23, 5);
    drawCircle(cx + flip * 20, cy + 25, 4);
    drawCircle(cx + flip * 13, cy + 22, 4);

    // Wing
    setColor(0.7f, 0.5f, 0.5f);
    drawEllipse(cx - flip * 5, cy + 2, 14, 10);


    setColor(0.9f, 0.9f, 0.9f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx - flip * 18, cy + 8);
    glVertex2f(cx - flip * 35, cy + 20);
    glVertex2f(cx - flip * 30, cy + 2);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex2f(cx - flip * 18, cy + 2);
    glVertex2f(cx - flip * 35, cy + 5);
    glVertex2f(cx - flip * 28, cy - 8);
    glEnd();

   
    setColor(1.0f, 0.6f, 0.1f);
    drawRect(cx + flip * 5 - 3, cy - 18, 5, 10);
    drawRect(cx - flip * 5 - 3, cy - 18, 5, 10);
}

void drawPerk(float cx, float cy, PerkType t) {
    float hw = PERK_W / 2, hh = PERK_H / 2;

    // Block background
    switch(t) {
        case PERK_WIDE:   setColor(0.2f, 0.9f, 0.4f); break;
        case PERK_SLOW:   setColor(0.2f, 0.7f, 1.0f); break;
        case PERK_TIME:   setColor(1.0f, 0.8f, 0.1f); break;
        case PERK_SHIELD: setColor(0.8f, 0.3f, 1.0f); break;
        case PERK_DOUBLE: setColor(1.0f, 0.4f, 0.4f); break;
    }
    drawRect(cx - hw, cy - hh, PERK_W, PERK_H);

    // Border
    setColor(1, 1, 1);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(cx - hw, cy - hh); glVertex2f(cx + hw, cy - hh);
    glVertex2f(cx + hw, cy + hh); glVertex2f(cx - hw, cy + hh);
    glEnd();
    glLineWidth(1.0f);

    // Icon letter
    setColor(1, 1, 1);
    std::string lbl;
    switch(t) {
        case PERK_WIDE:   lbl = "W"; break;
        case PERK_SLOW:   lbl = "S"; break;
        case PERK_TIME:   lbl = "+T"; break;
        case PERK_SHIELD: lbl = "SH"; break;
        case PERK_DOUBLE: lbl = "2x"; break;
    }
    drawText(cx - 7, cy - 5, lbl, GLUT_BITMAP_HELVETICA_12);
}
void drawBasket(float bx, float bw) {
    float bx1 = bx - bw / 2, bx2 = bx + bw / 2;
    float by1 = BASKET_Y, by2 = BASKET_Y + BASKET_H;

    // Wicker body
    setColor(0.6f, 0.4f, 0.15f);
    drawRect(bx1, by1, bw, BASKET_H);

    // Weave lines
    setColor(0.5f, 0.32f, 0.1f);
    glLineWidth(1.0f);
    for (float lx = bx1 + 10; lx < bx2; lx += 10) {
        glBegin(GL_LINES);
        glVertex2f(lx, by1); glVertex2f(lx, by2);
        glEnd();
    }
    for (float ly = by1 + 8; ly < by2; ly += 8) {
        glBegin(GL_LINES);
        glVertex2f(bx1, ly); glVertex2f(bx2, ly);
        glEnd();
    }

    // Rim
    setColor(0.45f, 0.28f, 0.08f);
    drawRect(bx1 - 3, by2 - 5, bw + 6, 8);

    // Handle (arc drawn as line strip)
    setColor(0.5f, 0.32f, 0.1f);
    glLineWidth(3.0f);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 20; i++) {
        float t2 = i / 20.0f;
        float ax = bx1 + t2 * bw;
        float ay = by2 + 5 + 20 * sinf(t2 * 3.14159f);
        glVertex2f(ax, ay);
    }
    glEnd();
    glLineWidth(1.0f);

    // Shield glow
    if (shieldActive) {
        setColor(0.8f, 0.3f, 1.0f, 0.4f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        drawEllipse(bx, by1 + BASKET_H / 2, bw / 2 + 10, BASKET_H / 2 + 15);
        glDisable(GL_BLEND);
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    drawBackground();

    for (int i = 0; i < NUM_STICKS; i++) {
        drawStick(STICK_Y[i]);
    }
    // Draw test chickens
    drawChicken(200, STICK_Y[0] + 10, true);
    drawChicken(600, STICK_Y[1] + 10, false);

    drawBasket(basketX, basketW);
    glutSwapBuffers();



}// display complete

int main(int argc, char** argv) {
    srand((unsigned)time(nullptr));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(100, 80);
    glutCreateWindow("Catch The Eggs - CSE 426 Final Project");

    glClearColor(0.4f, 0.7f, 1.0f, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMainLoop();


    return 0;
}
