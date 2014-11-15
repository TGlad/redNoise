#include "OrientableCell.h"

OrientableCell::OrientableCell() : Cell()
{
  rotation.setIdentity();
  type = Cell::Orientable;
  scale = 1.0f;
}

OrientableCell::OrientableCell(Cell* parent, int numChildren, char* childBuffer) : Cell(parent, numChildren, childBuffer)
{
  rotation.setIdentity();
  type = Cell::Orientable;
  scale = 1.0f;
}

void OrientableCell::lookAt(const Position& target)
{
  rotation.fromForwardAlignedByUp(Vector3::normalise(target - position), Vector3(0, 1, 0));
}

bool OrientableCell::update(Length maxRadius, bool reattach)
{
  Cell* oldParent = parent;
  reparent(parent, rotation, position, scale, NULL, false, maxRadius*scale);
  if (parent != oldParent && reattach)
  {
    if (oldParent)
      oldParent->removeChild(this);
    if (scale < 0.01f)
    {
      int h = 3;
    }
    if (parent)
      parent->addChild(this);
    return true;
  }
  return false;
}

// TODO: shame that matrices have to be passed by value...
// This function needs work, it is fairly brute-force to check all children for a closest match, probably need a better algorithm.
bool OrientableCell::reparent(Cell* testParent, Rotation toParentRot, Position toParentPos, Scale toParentScale, const Cell* avoidChild, bool parentChecked, Length radius)
{
  const float testScaleFactor = 0.7f; // smaller goes higher in the tree.
  Length distance = toParentPos.magnitude();
  if (distance + radius < testParent->radius * testScaleFactor) // cell is too large, so check its children
  {
    bool found = false;
    if (testParent->radius < parent->radius)
    {
      parent = testParent;
      rotation = toParentRot;
      position = toParentPos;
      scale = toParentScale;
      found = true;
    }
    for (int i = 0; i < testParent->numChildren; i++)
    {
      Cell* child = testParent->children[i];
      if (child == this)
        continue;
      if (child != avoidChild)
      {
        Position newToParentPos = toParentPos - child->position;
        Rotation newToParentRot = toParentRot;
        Scale newToParentScale = toParentScale;
        Length newRadius = radius;
        if (child->type >= Cell::Orientable)
        {
          OrientableCell* oChild = (OrientableCell *)child;
          newToParentRot *= ~oChild->rotation;
          newToParentPos.inverseRotate(oChild->rotation);
          newToParentPos /= oChild->scale;
          newToParentScale /= oChild->scale;
          newRadius /= oChild->scale;
        }
        reparent(child, newToParentRot, newToParentPos, newToParentScale, NULL, true, newRadius);
      }
    }
    return found;
  }
  // cell is too small so check its parent
  if (parentChecked || testParent->parent == NULL) // this is as far as we can go
    return false;
  parent = testParent->parent;
  if (testParent->type >= Cell::Orientable)
  {
    OrientableCell* oParent = (OrientableCell*)testParent;
    toParentRot *= oParent->rotation;
    toParentPos.rotate(oParent->rotation);
    toParentScale *= oParent->scale;
    toParentPos *= oParent->scale;
    radius *= oParent->scale;
  }
  toParentPos += testParent->position;
  rotation = toParentRot;
  position = toParentPos;
  scale = toParentScale;
  reparent(parent, toParentRot, toParentPos, toParentScale, testParent, false, radius);
  return true;
}

