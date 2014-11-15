#pragma once
#include "basics.h"
#include "Cell.h"

// Has an orientation and scale, so provides its own reference frame
class OrientableCell : public Cell
{
public:
  Rotation rotation; // position, rotation, scale. w.r.t. parent. TODO: we are duplicating position with base class!
  Scale scale; 
  OrientableCell(Cell* parent, int numChildren, char* childBuffer = NULL);
  OrientableCell();
  void lookAt(const Position& target);
  bool update(Length maxRadius, bool reattach = true); // When cell has moved and needs to be shifted within the hierarchy

private:
  bool reparent(Cell* testParent, Rotation toParentRot, Position toParentPos, Scale toParentScale, const Cell* avoidChild, bool parentChecked, Length radius);
};
