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

// ─── Constants ────────────────────────────────────────────────────────────────
const int   NUM_STICKS    = 2;
const float STICK_Y[2]    = {540.f, 500.f}; // y positions of sticks
const float STICK_X1      = 50.f;
const float STICK_X2      = 750.f;
const float BASKET_Y      = 60.f;
const float BASKET_W_DEF  = 80.f;
const float BASKET_H      = 30.f;
const float EGG_R         = 12.f;
const float PERK_W        = 22.f;
const float PERK_H        = 22.f;
const int   GAME_DURATION = 120;
const float BASE_FALL_SPD = 2.8f;


void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}



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
