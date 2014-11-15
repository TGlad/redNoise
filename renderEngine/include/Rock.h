#pragma once
#include "basics.h"

class Cell;
// Cube based rock tree
class Rock
{
public:
  class PhysicalCell* root;
  Rock(int depth, Scale amplitude, Scale size);
  Length size;
private:
  Cell* boxFace[6];
  Cell** cells;
  int* levels; // points to array cells index for that level
  int* numEntries;
  int maxWidth;
  int m_depth;
  Scale m_amplitude;
  struct RockElement
  {
    Distance height;
    Scale shade;
    void average(RockElement& a, RockElement& b, RockElement& c, RockElement& d)
    {
      height = (a.height + b.height + c.height + d.height) * 0.25f;
      shade  = (a.shade + b.shade + c.shade + d.shade) * 0.25f;
    }
    void average(RockElement& a, RockElement& b)
    {
      height = (a.height + b.height) * 0.5f;
      shade  = (a.shade + b.shade) * 0.5f;
    }
  } *** baseArray;
  int currentFace;
  void generateBaseRecursive(int minX, int minY, int maxX, int maxY);
  Cell* generateRecursive(Cell** cellList, Cell* parent, const Vector3& parentPosWorldSpace, int minX, int minY, int maxX, int maxY, int level, Length size, Length radius);
  Position boxPos(int x, int y);
  RockElement& element(int x, int y);
  RockElement& boxElement(int x, int y, int z);
  Position getPos(int x, int y);
};