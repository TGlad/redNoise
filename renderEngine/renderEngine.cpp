#include <stdlib.h>
#include <glut.h>
#include <stdio.h>
#include "Core.h"
#include "Timer.h"

static Core core; // Just a singleton
static int width = 512;
static int height = 384;
static Timer frameTimer;

void init(void)
{    
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glShadeModel(GL_FLAT);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  core.init(width, height);
}

void display(void)
{
  core.render();
  glutSwapBuffers();
}

void update(void)
{
  TimePeriod timeStep = frameTimer.stop(); // in ms
  core.update(clamped(timeStep*0.001f, 1.0f/60.0f, 1.0f/15.0f)); // avoids outliers
  frameTimer.reset();
  frameTimer.start();
  glutPostRedisplay();
 // glutIdleFunc(NULL); // means you click for each update
}
void reshape(int w, int h)
{
  width = w;
  height = h;
}

void mouse(int button, int state, int x, int y) 
{
   switch (button) {
      case GLUT_LEFT_BUTTON:
         if (state == GLUT_DOWN)
            frameTimer.start();
            glutIdleFunc(update); // starts the updating
         break;
      case GLUT_MIDDLE_BUTTON:
         if (state == GLUT_DOWN)
            glutIdleFunc(NULL);
         break;
      default:
         break;
   }
}

int main(int argc, char** argv)
{
   glutInit(&argc, argv);   
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA);
   glutInitWindowSize(width, height); 
   glutInitWindowPosition(100, 100);
   glutCreateWindow(argv[0]);
   init();
   glutDisplayFunc(display); 
   glutReshapeFunc(reshape);
   glutMouseFunc(mouse);
   glutMainLoop();
   return 0;
}