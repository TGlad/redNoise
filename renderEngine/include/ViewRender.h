#include "View.h"
#include "Cell.h"
#include "Screen.h"
#include "EnvironmentBox.h"
#include "OrientableCell.h"
#include "ProceduralExample.h"

// This does a full frustum cull
void View::renderCheckBounds(const Rotation* cellToViewRot, Scale cellToViewScale, const Position& cellToViewPos, Cell* cell, int level, Length width)
{
  Distance dist = cellToViewPos.z;
  Length cellRadius = cell->radius * cellToViewScale;
  Area distSqr = cellToViewPos.magnitudeSquared();
  if (distSqr > sqr(farPlane + cellRadius)) // Cull far objects. This is actually quite nice as it culls most of the map in one test
    return;
  if (cellToViewPos.z < nearPlane)
  {
    // Too many tests here....
    if (cellToViewPos.z + cellRadius < vNearPlane) // behind camera
      return;
    if (distSqr < sqr(nearPlane - cellRadius) && cellRadius<nearPlane)
      return;
    if (cellToViewPos.z < 0.001f)
      dist = 0.001f;//nearPlane*0.5f;
  }

  // Occlusion tests against mipmapped z-buffer
  Proximity invDotZ = 0.5f / dist;
  Length radius = cellRadius * invDotZ; // the diameter
  Length clampedRad = radius < 0.1f ? radius : 0.1f;

  Distance posX = cellToViewPos.x * xScale * invDotZ;
  Length absPosX = absf(posX);
  // Note actual extended radius is tan(A+or-B)-x where A = atan(x) and B = asin(r/sqrt(1+x^2)).
  // Possibly better would be to use a 2d lookup table... but this is reasonably close.
  Length radiusX = xScale*radius*(1.0f + clampedRad) + clampedRad*absPosX; 
  posX += 0.5f;
  Distance cellMinX = posX - radiusX;
  Distance cellMaxX = posX + radiusX;

  Length maxX = absPosX + radiusX;
  if (maxX > 0.5f)
  {
    if (maxX > 0.5f + radiusX+radiusX)
      return;
    if (cellMinX < 0.0f)
      cellMinX = 0.0f;
    if (cellMaxX >= 1.0f)
      cellMaxX = 1.0f - 1e-5f;
  }

  Distance posY = -cellToViewPos.y * yScale * invDotZ;
  Length absPosY = absf(posY);
  Length radiusY = yScale*radius*(1.0f + clampedRad) + clampedRad*absPosY;
  posY += 0.5f;
  Distance cellMinY = posY - radiusY;
  Distance cellMaxY = posY + radiusY;

  Length maxY = absPosY + radiusY;
  if (maxY > 0.5f)
  {
    if (maxY > 0.5f + radiusY+radiusY)
      return;
    if (cellMinY < 0.0f)
      cellMinY = 0.0f;
    if (cellMaxY >= 1.0f)
      cellMaxY = 1.0f - 1e-5f;
  }

  if (radiusX < halfInvScreenW || cell->children == NULL) // don't need to drill down any further, but it could still cover up several pixels
  {
//     if (cellToViewPos.z < vNearPlane) // 2% faser without this check... not sure if it is really needed
//       return; // Need to make this more efficient

    int cellMinXi = (int)(cellMinX * screenW);
    int cellMaxXi = (int)(cellMaxX * screenW);
    int cellMinYi = (int)(cellMinY * screenH);
    int cellMaxYi = (int)(cellMaxY * screenH);
    
    const Scale normScale = 1.0f/127.0f;
    Direction normal((float)cell->normal[0]*normScale, (float)cell->normal[1]*normScale, (float)cell->normal[2]*normScale);
    normal = cellToViewRot->rotateVector(normal); 
    float shade = 0.75f*normal.dot(lightDirectionViewSpace);
    if (shade < 0.0f)
      shade = 0.0f;

    unsigned short bufferDepth = getDepth(dist);

#if 0 
    ScreenColour colour(cell->colour*shade); 
#else
    // Very basic fogging system
    Colour col = cell->colour * (shade + 0.25f);
    Weight blend = fogScale*invDotZ;
    blend = blend > 1.0f ? 1.0f : blend;
    // Note, we might want to blur in the case of large pixels
    ScreenColour colour(col * blend + Vector3(0.3f, 0.5f, 0.7f)*(1.0f-blend));
#endif
#if defined(TRANSPARENCY)
    if (cell->type != Cell::Transparent)
    {
#endif
      for (int i = cellMinXi; i <= cellMaxXi; i++)
      {
        for (int j = cellMinYi; j <= cellMaxYi; j++)
        {
          unsigned short dist = screen->getDistance(0, i, j);
          if (dist < bufferDepth)
          {
            screen->image->setPixel(i, j, colour);
            screen->setDistance(0, i, j, bufferDepth);
          }
        }
      }
#if defined(TRANSPARENCY)
    }
    else
    {
      for (int i = cellMinXi; i <= cellMaxXi; i++)
      {
        for (int j = cellMinYi; j <= cellMaxYi; j++)
        {
          unsigned short dist = screen->getDistance(0, i, j);
          unsigned char& alpha = screen->image->alpha(i, j);
          if (cellToViewPos.dot(normal) < 0.0f) // only actually draw on front facing pixels
          {
            if (dist < bufferDepth && alpha==254)
            {
              Scale opacity = 0.0003f;
              Scale blend = min((float)(bufferDepth - dist)*opacity, 1.0f);
              screen->image->blendPixel(i, j, colour, blend);
            }
            screen->setDistance(0, i, j, bufferDepth);
          }
          else
          {
            alpha = 254;
            if (dist < bufferDepth)
              screen->setDistance(0, i, j, bufferDepth);
          }
        }
      }
    }
#endif
    return;
  }

#define OCCLUSION_CULLING
#if defined(OCCLUSION_CULLING)
  while (radiusX*width < 0.25f)
  {
    width *= 2.0f;
    level--;
    if (level < 0)
    {
      int h = 3;
    }
  }

  bool occluded = true;
  Scale hScale = screenH/screenW;
  int cellMinXI = (int)(cellMinX * width);
  int cellMaxXI = (int)(cellMaxX * width);
  int cellMaxYI = (int)(cellMaxY * width * hScale);
  int cellMinYI = (int)(cellMinY * width * hScale);
  unsigned short bufferDepth2 = getDepth(cellToViewPos.z - cell->radius);
  for (int i = cellMinXI; i <= cellMaxXI; i++)
  {
    for (int j = cellMinYI; j <= cellMaxYI; j++)
    {
      if (screen->getDistance(level, i, j) < bufferDepth2)
      {
        occluded = false;
        i = cellMaxXI; // We might as well end the double loop early, even though it is only a max of 4 to check
        j = cellMaxYI; // Alternatively we could do occluded ||= to avoid the if statement
      }
    }
  }
  if (occluded)
    return;
#endif
#if defined(PROCEDURAL)
  // OK so our big cell is visible and not occluded, so test children
  if (cell->type == Cell::Procedural && cell->children[0]==NULL) // Special case for procedural cells, we need to actually generate their children
  {
    ((ProceduralCell*)cell)->generateChildren();
  }
#endif
  int numChildren = cell->numChildren;
  for (int i = 0; i < numChildren; i++)
  {
    int index = numChildren==4 ? orderings[orderIndex][i] : i;
    Cell* child = cell->children[index];
    Position childToViewPos = cellToViewPos + cellToViewRot->rotateVector(child->position)*cellToViewScale;
    Scale childToViewScale = cellToViewScale;
    const Rotation* childToViewRot = cellToViewRot;
    if (child->type >= Cell::Orientable) // we have to adjust the orientation, the position is fine
    {
      Rotation mat = ((OrientableCell*) child)->rotation * *cellToViewRot;
      childToViewRot = &mat;
      childToViewScale *= ((OrientableCell*) child)->scale;
    }
    renderCheckBounds(childToViewRot, childToViewScale, childToViewPos, child, level, width);
  }

  CellList* cellDynamics = cell->dynamics;
  while (cellDynamics)
  {
    Cell* child = cellDynamics->child;
    Position childToViewPos = cellToViewPos + cellToViewRot->rotateVector(child->position)*cellToViewScale;
    Scale childToViewScale = cellToViewScale;
    const Rotation* childToViewRot = cellToViewRot;
    if (child->type >= Cell::Orientable) // we have to adjust the orientation, the position is fine
    {
      Rotation mat = ((OrientableCell*) child)->rotation * *cellToViewRot;
      childToViewRot = &mat;
      childToViewScale *= ((OrientableCell*) child)->scale;
    }
    renderCheckBounds(childToViewRot, childToViewScale, childToViewPos, child, level, width);
    cellDynamics = cellDynamics->next;
  }
}

