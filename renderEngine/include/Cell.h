#pragma once
#include "basics.h"

struct CellList
{
  class Cell* child;
  CellList* next;
};
class Cell 
{
public:
  Position position; // w.r.t. parent
  Cell* parent;

  Colour colour;
  char normal[3]; // quantised
  Length radius;
  class Cell **children; // ordered in x, ordered in z
  unsigned char numChildren;
  
  CellList* dynamics;
  enum CellType
  {
    Basic,
    Transparent,
    Procedural,

    // put this set at the end for easier conditions of orientable and its derived classes
    Orientable, // Has an orientation and scale, so provides its own reference frame
    Dynamic,    // Has mass and velocity, so requires an update function
    Physical,   // Has collision properties, so collides and bounces off other cells
  };
  unsigned char type; 

  Cell();
  Cell(Cell* parent, int numChildren, char *childBuffer = NULL);
  ~Cell();

  void addChild(Cell* child);
  bool removeChild(Cell* child);
};
