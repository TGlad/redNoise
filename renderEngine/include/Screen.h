#pragma once
#include "basics.h"
#include "ScreenColour.h"
#include "Image.h"

class Screen
{
public:
  int width;
  int height;
  int m_depth; // just log2 of the width
  Image *image;
  unsigned short **closestDistance; // z buffer

  Screen(int w, int h);
  ~Screen();
  void setDistance(int level, int x, int y, unsigned short bufferDepth);
  inline unsigned short getDistance(int level, int x, int y)
  {
    int shift = m_depth - level;
    return closestDistance[level][x + (y << shift)];
  }
  void drawOcclusion();
  void clear();
};