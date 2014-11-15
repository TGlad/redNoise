#include "ProceduralExample.h"

ProceduralExample::ProceduralExample(Cell* parent, int numChildren) : Cell(parent, numChildren)
{
  type = Cell::Procedural;
}

void ProceduralExample::generateChildren()
{
  // Simple fractal example, the cell creates 4 children on a plane with a small offset
  int child = 0;
  for (int i = -1; i <= 1; i += 2)
  {
    for (int j = -1; j <= 1; j += 2)
    {
      children[child] = new ProceduralExample(this, 4);
      children[child]->radius = radius * 0.5f;
      children[child]->colour = colour;// * 0.5f;
      children[child]->position.set(((float)i) * radius * 0.5f, 0.1f * radius, ((float)j) * radius * 0.5f);
      child++;
    }
  }
}