#pragma once
#include "basics.h"
#include "Timer.h"
class Cell;
class Terrain
{
public:
  class OrientableCell* root;
  Cell** cells;
  int* levels; // points to array cells index for that level
  int* numEntries;
  int maxWidth;
  int depth;
  Scale amplitude;
  struct TerrainElement
  {
    Position pos;
    Scale shade;
  }** baseArray;
  Terrain(int depth, Length size, Scale amplitude);
  Position volcanoPos;
private:
  Timer loadTimer;
  // diamond square algorithm
  void averageDiamond(int minX, int minY, int maxX, int maxY);
  void averageSquare(int minX, int minY, int maxX, int maxY);
  void generateDiamond(int minX, int minY, int maxX, int maxY);
  void generateSquare(int minX, int minY, int maxX, int maxY);

  Cell* generateRecursive(Cell* parent, const Position& parentPosWorldSpace, int minX, int minY, int maxX, int maxY, int level, Length size, Length radius);
  void addVolcano(Length size);
};