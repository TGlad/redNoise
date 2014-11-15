#include "DebugDraw.h"
#include "Cell.h"
#include "View.h"
#include "Screen.h"
#include "Image.h"

DebugDraw g_debugDraw;

DebugDraw::DebugDraw()
{
  numCircles = 0;
  numSpheres = 0;
  numLines = 0;
}
void DebugDraw::drawCircle(Cell* cell, const Position& posInCellSpace, Length radius, const Colour& colour)
{
  if (numSpheres == maxSpheres-1)
    return;

  circles[numSpheres].cell = cell;
  circles[numSpheres].position = posInCellSpace;
  circles[numSpheres].radius = radius;
  circles[numSpheres++].colour = colour;
}

void DebugDraw::drawSphere(Cell* cell, const Position& posInCellSpace, const Rotation& rotationInCellSpace, Length radius, const Colour& colour)
{
  if (numSpheres == maxSpheres-1)
    return;

  spheres[numSpheres].cell = cell;
  spheres[numSpheres].position = posInCellSpace;
  spheres[numSpheres].rotation = rotationInCellSpace;
  spheres[numSpheres].radius = radius;
  spheres[numSpheres++].colour = colour;
}

void DebugDraw::drawCell(Cell* cell, const Colour& colour)
{
  drawSphere(cell, Position(0,0,0), Rotation(RotationVector(0,0,0)), cell->radius, colour);
}
 
void DebugDraw::drawLine(Cell* cell, const Position& start, const Position& end, const Colour& colour)
{
  if (numLines == maxLines-1)
    return;

  lines[numLines].cell = cell;
  lines[numLines].start = start;
  lines[numLines].end = end;
  lines[numLines++].colour = colour;
}

void DebugDraw::drawAll(View* view)
{
  Matrix33 sceneToView = view->rotation.transposed();
  sceneToView.scale(1.0f/view->scale); // Put the scale into the matrix for speed
  drawCircles(view, sceneToView);
  drawSpheres(view, sceneToView);
  drawLines(view, sceneToView);
}

void DebugDraw::drawCircles(View* view, const Rotation& sceneToView)
{
  for (int i = 0; i<numCircles; i++)
  {
    // Here we iterate through the parents until we find the view scene cell
    Position scenePos = circles[i].position; 
    Length sceneRadius = circles[i].radius;
    Cell *cell = circles[i].cell;
    while (cell != view->parent)
    {
      if (cell == NULL)
        break;
      if (cell->type >= Cell::Orientable)
      {
        OrientableCell* oCell = (OrientableCell*)cell;
        scenePos = oCell->rotation.rotateVector(scenePos);
        scenePos *= oCell->scale;
        sceneRadius *= oCell->scale;
      }
      scenePos += cell->position;
      cell = cell->parent;
    }

    Position circleViewPos = sceneToView.rotateVector(scenePos - view->position);
    if (circleViewPos.z <= 0.0f) // circle not visible
      continue;
    Distance posX = 0.5f * view->xScale * circleViewPos.x / circleViewPos.z;
    Distance posY = -0.5f * view->yScale * circleViewPos.y / circleViewPos.z;
    Length radius = 0.5f * view->xScale * sceneRadius / circleViewPos.z;

    int numPoints = (int)(2.0f*3.1415f*radius*view->screenW);
    Angle rotAngle = 2.0f*3.1415f / (float)numPoints;
    Scale cosA = cosf(rotAngle);
    Scale sinA = sinf(rotAngle);
    Distance dirX = radius;
    Distance dirY = 0.0f;
    unsigned short bufferDepth = view->getDepth(circleViewPos.z);
    for (int j = 0; j<numPoints; j++)
    {
      int x = (int)(((0.5f+posX) + dirX)*view->screenW);
      int y = (int)(((0.5f+posY) + dirY)*view->screenW);
      if (x >= 0 && y >= 0 && x<view->screen->width && y<view->screen->height)
      {
        if (view->screen->getDistance(0, x, y) < bufferDepth)
          view->screen->image->setPixel(x, y, ScreenColour(circles[i].colour));
      }
      Distance dirX2 = dirX*cosA + dirY*sinA;
      Distance dirY2 = -dirX*sinA + dirY*cosA;
      dirX = dirX2;
      dirY = dirY2;
    }
  }
  numCircles = 0;
}

void DebugDraw::drawSpheres(View* view, const Rotation& sceneToView)
{
  for (int i = 0; i<numSpheres; i++)
  {
    // Here we iterate through the parents until we find the view scene cell
    Position scenePos = spheres[i].position; 
    Rotation sceneRot = spheres[i].rotation;
    Length sceneRadius = spheres[i].radius;
    Cell *cell = spheres[i].cell;
    while (cell != view->parent)
    {
      if (cell == NULL)
        break;
      if (cell->type >= Cell::Orientable)
      {
        OrientableCell* oCell = (OrientableCell*)cell;
        scenePos = oCell->rotation.rotateVector(scenePos);
        scenePos *= oCell->scale;
        sceneRadius *= oCell->scale;
        sceneRot *= oCell->rotation;
      }
      scenePos += cell->position;
      cell = cell->parent;
    }

    Position sphereViewPos = sceneToView.rotateVector(scenePos - view->position);
    sceneRot *= sceneToView;
    if (sphereViewPos.z <= 0.0f) // sphere not visible
      continue;

    Length radius = 0.5f * view->xScale * sceneRadius / sphereViewPos.z;
    int numPoints = (int)(0.5f*3.1415f*radius*view->screenW); 
    Angle rotAngle = 2.0f*3.1415f / (float)numPoints;
    Scale cosA = cosf(rotAngle);
    Scale sinA = sinf(rotAngle);
    Distance dirA = sceneRadius;
    Distance dirB = 0.0f;

    int axes[3][2] = {{0,1}, {1,2}, {2,0}};
    for (int k = 0; k<3; k++)
    {
      for (int j = 0; j<numPoints; j++)
      {
        Position pos = sphereViewPos + dirA*sceneRot.row[axes[k][0]] + dirB*sceneRot.row[axes[k][1]];
        if (pos.z <= 0.0f)
          continue;
        Proximity invZ = 1.0f/pos.z;
        int x = (int)((0.5f + (0.5f * view->xScale * pos.x * invZ))*view->screenW);
        int y = (int)((0.5f - (0.5f * view->yScale * pos.y * invZ))*view->screenW);
        unsigned short bufferDepth = view->getDepth(pos.z);
        if (x >= 1 && y >= 1 && x<view->screen->width-1 && y<view->screen->height-1)
        {
          if (view->screen->getDistance(0, x, y) < bufferDepth)
          {
            if (j==0)
            {
              Vector3 col(k==0 ? 1.0f : 0.0f, k==1 ? 1.0f : 0.0f, k==2 ? 1.0f : 0.0f);
              view->screen->image->setPixel(x+1, y, ScreenColour(col));
              view->screen->image->setPixel(x-1, y, ScreenColour(col));
              view->screen->image->setPixel(x, y+1, ScreenColour(col));
              view->screen->image->setPixel(x, y-1, ScreenColour(col));
              view->screen->image->setPixel(x, y, ScreenColour(col));
            }
            else
              view->screen->image->setPixel(x, y, ScreenColour(spheres[i].colour));
          }
        }
        Distance dirA2 = dirA*cosA + dirB*sinA;
        Distance dirB2 = -dirA*sinA + dirB*cosA;
        dirA = dirA2;
        dirB = dirB2;
      }
    }
  }
  numSpheres = 0;
}

void DebugDraw::drawLines(View* view, const Rotation& sceneToView)
{
  for (int i = 0; i<numLines; i++)
  {
    // Here we iterate through the parents until we find the view scene cell
    Position startPos = lines[i].start; 
    Position endPos = lines[i].end; 
    Cell *cell = lines[i].cell;
    while (cell != view->parent)
    {
      if (cell == NULL)
        break;
      if (cell->type >= Cell::Orientable)
      {
        OrientableCell* oCell = (OrientableCell*)cell;
        startPos = oCell->rotation.rotateVector(startPos);
        startPos *= oCell->scale;
        endPos = oCell->rotation.rotateVector(endPos);
        endPos *= oCell->scale;
      }
      startPos += cell->position;
      endPos += cell->position;
      cell = cell->parent;
    }

    startPos = sceneToView.rotateVector(startPos - view->position);
    endPos = sceneToView.rotateVector(endPos - view->position);
    if (startPos.z <= 0.0f && endPos.z <= 0.0f) // line not visible
      continue;

    Distance sX = 0.5f * view->xScale * startPos.x / startPos.z;
    Distance sY = 0.5f * view->yScale * startPos.y / startPos.z;
    Distance eX = 0.5f * view->xScale * endPos.x / endPos.z;
    Distance eY = 0.5f * view->yScale * endPos.y / endPos.z;
    int numPoints = (int)(0.5f*sqrtf(sqr(sX-eX) + sqr(sY-eY))*view->screenW);
    for (int j = 0; j<numPoints; j++)
    {
      Position pos = startPos + (endPos-startPos)*(float)j/(float)(numPoints-1);
      if (pos.z <= 0.0f)
        continue;
      Proximity invZ = 1.0f/pos.z;
      int x = (int)((0.5f + (0.5f * view->xScale * pos.x * invZ))*view->screenW);
      int y = (int)((0.5f - (0.5f * view->yScale * pos.y * invZ))*view->screenW);
      unsigned short bufferDepth = view->getDepth(pos.z);
      if (x >= 0 && y >= 0 && x<view->screen->width && y<view->screen->height)
      {
        if (view->screen->getDistance(0, x, y) < bufferDepth)
        {
          view->screen->image->setPixel(x, y, ScreenColour(lines[i].colour));
        }
      }
    }
  }
  numLines = 0;
}