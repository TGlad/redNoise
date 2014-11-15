#include "View.h"
#include "Cell.h"
#include "Screen.h"
#include "EnvironmentBox.h"
#include "OrientableCell.h"
#include "ProceduralExample.h"
#include "DebugDraw.h"
const int View::orderings[8][4] = {{3,2,1,0}, {2,3,0,1}, {1,0,3,2}, {0,1,2,3}, 
                                    {3,1,2,0}, {2,0,3,1}, {1,3,0,2}, {0,2,1,3}};

View::View(Angle fieldOfView, int width, int height, Cell* root)
{
  parent = root;
  fieldOfViewX = fieldOfView;
  xRatio = tanf(fieldOfView*3.14159f/360.0f);
  yRatio = xRatio * (float)height / (float)width;
//  fieldOfViewY = atanf(yRatio) * 360.0f/3.14159f;
  xScale = 1.0f / sqrtf(1.0f + xRatio * xRatio);
  yScale = 1.0f / sqrtf(1.0f + yRatio * yRatio);
  setPlanes(0.2f, 200.0f); 

  box = NULL;
  boxSideIndex = 0;
  frame = 0;
  oldForwards = rotation.forwards();
  stage = 0;
  sizeRelativeToRootView = 1.0f;
}

// float View::getDistance(int val)
// {
//   return nearPlane*exp((float)val*logf(farPlane/nearPlane)/65535.0f);
// }
void View::setPlanes(Length nearPlane, Length farPlane)
{
  this->nearPlane = nearPlane;
  this->farPlane = farPlane;

  // This works, but you get artifacts right at the end, presumably when depth=0,1,2,3
  float farp = farPlane * 1.02f; // add a little extra for rendering distant pixels at their lowest res
  // TODO: this nearp should be adjusted for larger fovs
  vNearPlane = nearPlane / 1.732f; // to compensate for the fact that the nearplane is curved so will get smaller z values
  int max = 65535;
  int min = 4; // this is a safety margin, I think because the far plane can be a bit variable
  float focusPlane = vNearPlane * sqrtf(farp/vNearPlane); // a sort of mid way
  float m = 2.0f*(focusPlane-vNearPlane)/(farp-vNearPlane);
  logC = (farp*m - focusPlane) / (1.0f-m);
  logA = (max-min)/(1/(vNearPlane+logC) - 1/(farp+logC));
  logB = max - logA * 1/(vNearPlane+logC);

  // Unit test
  int shouldBeMax     = getDepth(vNearPlane);
  int shouldBeHalfWay = getDepth(focusPlane);
  int shouldBeMin     = getDepth(farp);
}

void View::generateEnvironment(int width)
{
  box = new EnvironmentBox(width, this);
  box->currentView->box = new EnvironmentBox(width, box->currentView, box->screens);
  // set the outer box to be large enough to encompass the sun
  Distance outerPlane = 1.6e11f / box->currentView->box->currentView->sizeRelativeToRootView;
  box->currentView->box->currentView->setPlanes(box->currentView->box->currentView->nearPlane, outerPlane);
}
#include "viewRender.h"

// search parents to get direction to sun.
// currently we assume sun is root of tree, since tree is solar system
Direction View::findSun()
{
  Cell* testParent = parent;
  Rotation toParentRot = rotation;
  Position toParentPos = position;
  while (testParent->parent)
  {
    if (testParent->type >= Cell::Orientable)
    {
      OrientableCell* oParent = (OrientableCell*)testParent;
      toParentRot *= oParent->rotation;
      toParentPos.rotate(oParent->rotation);
      toParentPos *= oParent->scale;
    }
    toParentPos += testParent->position;
    testParent = testParent->parent;
  }

  return toParentRot.inverseRotateVector(-toParentPos);
}


void View::recordToScreen(Screen* screen)
{
  screen->image->clear();
  recordToScreenInternal(screen);
  if (stage==0)
    g_debugDraw.drawAll(this);
  screen->image->generateTexture();

  frame++;
//   if (frame%30 == 0)
//   {
//     recordTime.print("record time");
//     recordTime.reset();
//   }
}

void View::update()
{
  OrientableCell::update(farPlane * 2.0f, stage==0);
}

void View::recordToScreenInternal(Screen* screen)
{
  this->screen = screen;
  if (stage==0 && box!=NULL)
  {
    const int boxUpdatePeriod = 5;
    if (frame % boxUpdatePeriod == 0) // Build background
    {
      RotationVector rotationPerFrame = Vector3::cross(oldForwards, rotation.forwards());
      Direction futureForwards = rotation.forwards();
      futureForwards.rotate(rotationPerFrame * (float)boxUpdatePeriod*3);
      // TODO: could also make it only do the condition if moving significantly
      for (int i = 0; i<6; i++)
      {
        int index = (boxSideIndex + i)%6;
        // TODO: this condition below is a slight cheat, it ignores up and down to give a faster update, every two tries
        if (box->faces[index].forwards().dot(futureForwards) > 0.0f)
        {
          // Here we need to generate the position from the owner view's position
          box->currentView->parent = parent;
          box->currentView->position = position; // update box view position prior to recording the scene
          box->currentView->scale = scale * box->timesLargerThanOwner;
          box->currentView->rotation = box->faces[index];
          box->currentView->update();
          box->currentView->recordToScreen(box->screens[index]);
          boxSideIndex = (index+1)%6;
          break;
        }
      }
    }
  }
  else if (stage >= 1 && box!=NULL)
  {
    // Here we need to generate the position from the owner view's position
    box->currentView->parent = parent;
    box->currentView->position = position; // update box view position prior to recording the scene
    box->currentView->scale = scale * box->timesLargerThanOwner;
    box->currentView->rotation = rotation;
    box->currentView->update();
    box->currentView->recordToScreenInternal(screen);
  }
  recordTime.start();
  screen->clear();
  oldForwards = rotation.forwards();

  Scale xSize = absf(rotation.forwards().x);
  Scale zSize = absf(rotation.forwards().z);

  int x = (int)(rotation.forwards().x*0.99f + 1.0f);
  int z = (int)(rotation.forwards().z*0.99f + 1.0f);
  int xBigger = (int)(1.0f + (xSize - zSize)*0.99f);
  orderIndex = x + 2 * z + 4 * xBigger;
  const float fogStrength = stage < 2 ? 0.002f : 0.0f; // stops sun from being fogged
  fogScale = 1.0f / (sizeRelativeToRootView * fogStrength);

  xScale = 1.0f / xRatio;
  yScale = 1.0f / yRatio;
  screenW = (float)screen->width;
  screenH = (float)screen->height;
  halfInvScreenW = 0.5f / screenW; // surprisingly, changing this doesn't change the cost much
  // rotation is view-to-scene, so we need the inverse to move cells into view space
  Matrix33 sceneToView = rotation.transposed();
  lightDirectionViewSpace = Vector3::normalise(findSun());

  renderCheckBounds(&sceneToView, 1.0f/scale, sceneToView.rotateVector(-position)/scale, parent, screen->m_depth, 1.0f);
  recordTime.stop();
  if (stage == 0)
  {
    recordTime.print("stage 0");
  }
  else if (stage == 1)
  {
    recordTime.print("stage 1");
  }
  else 
  {
    recordTime.print("stage 2");
  }
  recordTime.reset();
}


void View::renderBox()
{
  if (box == NULL)
    return;
  renderBoxTime.start();
  // Perspective mode -------------------------------------------------
  glViewport(0, 0, (GLsizei) screen->width, (GLsizei) screen->height); 
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
//  gluPerspective(fieldOfViewY, (float)screen->width / (float)screen->height, nearPlane, farPlane);
  float nearPlaneY = nearPlane * (float)screen->height / (float)screen->width; // alternative
  glFrustum(-nearPlane, nearPlane, -nearPlaneY, nearPlaneY, nearPlane, farPlane);

  glMatrixMode(GL_MODELVIEW);

  glColor3f(0.3f, 0.5f, 0.7f);   
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);


  glLoadIdentity();            
  gluLookAt(0, 0, 0, 
    rotation.row[2].x, rotation.row[2].y, rotation.row[2].z, 
    rotation.row[1].x, rotation.row[1].y, rotation.row[1].z);

  box->draw();

  glDisable(GL_TEXTURE_2D);

//   renderBoxTime.stop();
//   if (frame%30 == 0)
//   {
//     renderBoxTime.print("render box time");
//     renderBoxTime.reset();
//   }

}

void View::render()
{
  renderTime.start();
  // Ortho mode ---------------------------------------------------
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, (GLdouble) screen->width, 0.0, (GLdouble) screen->height);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glRasterPos2i(screen->width-1, screen->height-1); // 0,0 if pixel zoom is 1
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glPixelZoom(-1.0f, -1.0f); // so we don't have to rearrange in the setPixel call
 // screen->drawOcclusion(); // For testing
  screen->image->draw();
  glPixelZoom(1.0f, 1.0f);

  glDisable(GL_BLEND);
  renderTime.stop();
//   if (frame%30 == 0)
//   {
//     renderTime.print("render time");
//     renderTime.reset();
//   }
}