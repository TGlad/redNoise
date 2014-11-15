#pragma once
#include "basics.h"
#include "Cell.h"

class ProceduralExample : public Cell
{
public:
  ProceduralExample(Cell* parent, int numChildren);
  virtual void generateChildren();
};