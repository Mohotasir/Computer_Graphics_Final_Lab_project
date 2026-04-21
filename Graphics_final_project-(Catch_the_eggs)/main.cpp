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

// ─── Draw chicken ─────────────────────────────────────────────────────────────
void drawChicken(float cx, float cy, bool facingRight) {
    float flip = facingRight ? 1.0f : -1.0f;

    // Body
    setColor(0.9f, 0.7f, 0.3f);
    drawEllipse(cx, cy, 22, 18);

    // Head
    setColor(0.95f, 0.8f, 0.4f);
    drawCircle(cx + flip * 18, cy + 12, 12);

    // Eye
    setColor(0.1f, 0.1f, 0.1f);
    drawCircle(cx + flip * 21, cy + 14, 2.5f);

    // Beak
    setColor(1.0f, 0.6f, 0.1f);
    glBegin(GL_TRIANGLES);
    glVertex2f(cx + flip * 28, cy + 12);
    glVertex2f(cx + flip * 34, cy + 14);
    glVertex2f(cx + flip * 28, cy + 10);
    glEnd();

    // Comb (red)
    setColor(0.9f, 0.1f, 0.1f);
    drawCircle(cx + flip * 16, cy + 23, 5);
    drawCircle(cx + flip * 20, cy + 25, 4);
    drawCircle(cx + flip * 13, cy + 22, 4);

    // Wing
    setColor(0.8f, 0.6f, 0.2f);
    drawEllipse(cx - flip * 5, cy + 2, 14, 10);

    // Tail feathers
    setColor(0.7f, 0.5f, 0.15f);
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

    // Feet
    setColor(1.0f, 0.6f, 0.1f);
    drawRect(cx + flip * 5 - 3, cy - 18, 5, 10);
    drawRect(cx - flip * 5 - 3, cy - 18, 5, 10);
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
