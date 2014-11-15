#include "Cell.h"
#include <new>

Cell::Cell()
{
  parent = NULL;
  radius = 0.0f;
  position = Position(0,0,0);
  colour = Colour(0.5f, 0.5f, 0.5f);
  type = Cell::Basic;
  numChildren = 0;
  children = NULL;
}
Cell::Cell(Cell* parent, int numChildren, char *childBuffer)
{
  this->parent = parent;
  position = Position(0, 0, 0);
  type = Cell::Basic;
  dynamics = NULL;

  colour = Colour(0.5f, 0.5f, 0.5f);
  radius = 0.0f;
  if (childBuffer)
    children = new (childBuffer) Cell*[numChildren];
  else if (numChildren > 0)
    children = new Cell*[numChildren];
  else
    children = NULL;
  this->numChildren = numChildren;
}
Cell::~Cell()
{
  delete[] children;
}

void Cell::addChild(Cell* child)
{
  child->parent = this;
  if (!dynamics)
  {
    dynamics = new CellList();
    dynamics->child = child;
    dynamics->next = NULL;
    return;
  }
  CellList *element = dynamics;
  while (element->next)
    element = element->next;
  element->next = new CellList();
  element->next->child = child;
  element->next->next = NULL;
}

bool Cell::removeChild(Cell* child)
{
  if (!dynamics)
    return false;
  CellList *prev = NULL;
  CellList *it = dynamics;
  while (it->child != child)
  {
    prev = it;
    it = it->next;
    if (it == NULL)
      return false; // not found
  }
  if (prev)
    prev->next = it->next;
  else
    dynamics = it->next;
  delete it;
  return true;
}

