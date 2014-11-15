#pragma once
#include "basics.h"
#include "Cell.h"

class ProceduralCell : public Cell
{
public:
  ProceduralCell(Cell* parent, int numChildren) : Cell(parent, numChildren) { type = Cell::Procedural; }
  virtual void generateChildren() = 0;
};