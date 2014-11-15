#include "EnvironmentBox.h"
#include "View.h"
#include "Screen.h"

void EnvironmentBox::draw()
{
  for (GLint i = 0; i < 6; i++) 
  {
    glBindTexture(GL_TEXTURE_2D, screens[i]->image->textureID);
    glBegin(GL_QUADS);
    Matrix33 &mat = faces[i];
    Vector3 normal = mat.row[2];
    glNormal3fv((GLfloat*)&normal);
    Vector3 vec = normal + mat.row[0] - mat.row[1];
    glTexCoord2f(1.0, 1.0); glVertex3fv((GLfloat*)&vec);
    Vector3 vec2 = normal + mat.row[0] + mat.row[1];
    glTexCoord2f(1.0, 0.0); glVertex3fv((GLfloat*)&vec2);
    Vector3 vec3 = normal - mat.row[0] + mat.row[1];
    glTexCoord2f(0.0, 0.0); glVertex3fv((GLfloat*)&vec3);
    Vector3 vec4 = normal - mat.row[0] - mat.row[1];
    glTexCoord2f(0.0, 1.0); glVertex3fv((GLfloat*)&vec4);
    glEnd();
  }
}
EnvironmentBox::EnvironmentBox(int width, const View* parent, Screen **sharedScreens)
{
  Direction faceNormal[6];
  faceNormal[0] = Vector3(0,0,1);
  faceNormal[1] = Vector3(0,1,0);
  faceNormal[2] = Vector3(1,0,0);
  faceNormal[3] = Vector3(0,0,-1);
  faceNormal[4] = Vector3(0,-1,0);
  faceNormal[5] = Vector3(-1,0,0);
  currentView = new View(90.0f, width, width, NULL);
  currentView->stage = parent->stage + 1;
  currentView->setPlanes(parent->nearPlane, parent->farPlane);
  // scaling the cell is a better option than having huge near/far planes
  timesLargerThanOwner = parent->farPlane / parent->nearPlane; // max when looking directly at a corner. 
  currentView->sizeRelativeToRootView = parent->sizeRelativeToRootView * timesLargerThanOwner;
  for (int i = 0; i<6; i++) 
  {
    if (sharedScreens)
      screens[i] = sharedScreens[i];
    else
      screens[i] = new Screen(width, width);
    Direction up = Direction(0,1,0);
    if (faceNormal[i].y != 0.0f) // up and down directions have y pointing in the z
      up.set(0,0,1);
    faces[i].fromForwardAlignedByUp(faceNormal[i], up);
  }

  this->width = (Length)width;
}

EnvironmentBox::~EnvironmentBox()
{
  delete currentView;
}