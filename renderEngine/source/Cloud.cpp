#include "Cloud.h"
#include "Cell.h"
#include "PhysicalCell.h"
#include "time.h"
#include <new>

class CloudCellSize
{
public:
  Cell cell;
  Cell* children[4];
};

Cloud::Cloud(int depth, Scale amplitude, Scale size)
{
  // This just builds a contiguous list, cells isn't used directly in the render
  this->size = size;
  m_depth = depth;
  maxWidth = 1<<depth;
  m_amplitude = amplitude;
  levels = new int[depth];
  numEntries = new int[depth];
  int count = 0;
  int width = 0;
  for (int i = 0; i < depth; i++)
  {
    numEntries[i] = 0;
    width = 1<<i;
    levels[i] = count;
    count += width*width;
  }
  CloudCellSize *contiguous = new CloudCellSize[count*2]; // one for each face
  cells = new Cell*[count*2]; // one big contiguous array (of references)
  for (int face = 0; face < 2; face++)
  {
    CloudCellSize *rockCell = contiguous + face*count;
    for (int i = 0; i < count; i++)
    {
      // Note that placement new is used here to avoid all the headers and padding associated with 
      // doing lots of little news
      if (i >= count - (width * width))
        cells[i + face*count] = new (&(rockCell[i].cell)) Cell(NULL, 0);
      else
        cells[i + face*count] = new (&(rockCell[i].cell)) Cell(NULL, 4, (char *)&(rockCell[i].children)); // this will build them up basically contiguously
      Cell* newCell = cells[i + face*count];
      newCell->type = Cell::Transparent;
    }
  }

  baseArray = new CloudElement**[2];
  for (int face = 0; face < 2; face++)
  {
    baseArray[face] = new CloudElement*[maxWidth+1];
    for (int i = 0; i < maxWidth+1; i++)
    {
      baseArray[face][i] = new CloudElement[maxWidth+1];
      for (int j = 0; j < maxWidth+1; j++)
      {
        baseArray[face][i][j].height = 0.0f;
        baseArray[face][i][j].shade = 1.0f;
      }
    }
  }
  baseArray[0][maxWidth/2][maxWidth/2].height = -0.5f;
  baseArray[0][maxWidth/4][maxWidth/4].height = -0.5f;
  baseArray[0][3*maxWidth/4][maxWidth/4].height = -0.5f;
  baseArray[0][maxWidth/4][3*maxWidth/4].height = -0.5f;
  baseArray[0][3*maxWidth/4][3*maxWidth/4].height = -0.5f;
  baseArray[1][maxWidth/2][maxWidth/2].height = 0.5f;
  baseArray[1][maxWidth/4][maxWidth/4].height = 0.5f;
  baseArray[1][3*maxWidth/4][maxWidth/4].height = 0.5f;
  baseArray[1][maxWidth/4][3*maxWidth/4].height = 0.5f;
  baseArray[1][3*maxWidth/4][3*maxWidth/4].height = 0.5f;
//  srand((unsigned int)time(NULL) );
  for (currentFace = 0; currentFace < 2; currentFace++)
  {
    generateBaseRecursive(0, 0, maxWidth, maxWidth);
  }


  // Now generate the cell tree at the same time as generating the rock
  root = new OrientableCell(NULL, 2);
  for (currentFace = 0; currentFace<2; currentFace++)
  {
    for (int i = 0; i < depth; i++)
      numEntries[i] = 0;
    float radius = 1.8f*sqrtf((1.0f + 0.5f*m_amplitude) * 0.5f); // bit of a heuristic formula here, don't think it can be any smaller
    boxFace[currentFace] = generateRecursive(cells + count*currentFace, root, Vector3(0,0,0), 0, 0, maxWidth, maxWidth, 0, 1.0f, radius);
    root->children[currentFace] = boxFace[currentFace];
    root->radius = 1.732f;
  }

  root->position.set(-4.0f, 3.0f, 5.0f); // relative to where it goes in the scene
  root->scale = size;
  
  // Clean up lists of pointers
  delete[] cells; 
  for (int face = 0; face < 2; face++)
  {
    for (int i = 0; i < maxWidth+1; i++)
      delete[] baseArray[face][i];
    delete[] baseArray[face];
  }
  delete[] baseArray;
}
Vector3 Cloud::boxPos(int x, int y)
{
  float X, Y, Z;
  X=-1.f + 2.0f*(float)x/(float)maxWidth;
  Y=0.0f;//currentFace ? -1.f : 1.f;
  Z=-1.f + 2.0f*(float)y/(float)maxWidth;
 // Y *= 0.02f + 0.5f*(1.f-Z)*(-1.f-Z)*(1.f-X)*(-1.f-X);
  return Vector3(X,Y,Z); 
}

// To ensure the correct overlap of the box edges, we convert the 2d position to 3d sides
Cloud::CloudElement& Cloud::element(int x, int y)
{
  if (currentFace == 0 || x==0 || y==0 || x==maxWidth || y==maxWidth)
    return baseArray[0][x][y];
  return baseArray[1][x][y];
}

static float radiosityStrength = 1.5f;
void Cloud::generateBaseRecursive(int minX, int minY, int maxX, int maxY)
{
  if ((maxX - minX) <= 1 && (maxY - minY) <= 1)
    return;
  int midX = (minX + maxX) / 2;
  int midY = (minY + maxY) / 2;
  if (element(midX, midY).height == 0.0f)
    element(midX, midY).average(element(minX, minY), element(maxX, minY), element(minX, maxY), element(maxX, maxY));

  if (element(minX, midY).height == 0.0f)
  {
    CloudElement& el = element(minX, midY);
    el.average(element(minX, minY), element(minX, maxY));
    Scale offset = m_amplitude * random() * (float)(maxY - minY)/(float)maxWidth;
    el.height += offset;
    el.shade += offset*radiosityStrength;
  }
  if (element(maxX, midY).height == 0.0f)
  {
    CloudElement& el = element(maxX, midY);
    el.average(element(maxX, minY), element(maxX, maxY));
    Scale offset = m_amplitude * random() * (float)(maxY - minY)/(float)maxWidth;
    el.height += offset;
    el.shade += offset*radiosityStrength;
  }
  if (element(midX, minY).height == 0.0f)
  {
    CloudElement& el = element(midX, minY);
    el.average(element(maxX, minY), element(minX, minY));
    Scale offset = m_amplitude * random() * (float)(maxX - minX)/(float)maxWidth;
    el.height += offset;
    el.shade += offset*radiosityStrength;
  }
  if (element(midX, maxY).height == 0.0f)
  {
    CloudElement& el = element(midX, maxY);
    el.average(element(maxX, maxY), element(minX, maxY));
    Scale offset = m_amplitude * random() * (float)(maxX - minX)/(float)maxWidth;
    el.height += offset;
    el.shade += offset*radiosityStrength;
  }

  generateBaseRecursive(minX, minY, midX, midY);
  generateBaseRecursive(midX, minY, maxX, midY);
  generateBaseRecursive(minX, midY, midX, maxY);
  generateBaseRecursive(midX, midY, maxX, maxY);
}

Position Cloud::getPos(int x, int y)
{
  Position result = boxPos(x, y);
  Distance height = element(x, y).height;
  result.y += height;
  return result;
}
Cell* Cloud::generateRecursive(Cell** cellList, Cell* parent, const Position& parentPosWorldSpace, int minX, int minY, int maxX, int maxY, int level, Length size, Length radius)
{
  Cell *cell = NULL;
  cell = cellList[levels[level] + numEntries[level]];
  int midX = (minX + maxX) / 2;
  int midY = (minY + maxY) / 2;
  Scale scale = 2.0f / (float)maxWidth;

  Position centre = getPos(midX, midY) * size;

  cell->parent = parent;
  cell->colour.set(0.5f, 0.5f, 0.5f);
  cell->colour *= element(midX, midY).shade; // shade
  cell->colour.clamp(0.0f, 1.0f);
  cell->position.set(centre);
  if (parent != NULL)
    cell->position -= parentPosWorldSpace;

  PositionDelta xChange = getPos(minX, minY) - getPos(maxX, maxY);
  PositionDelta yChange = getPos(maxX, minY) - getPos(minX, maxY);
  Direction normal = Vector3::cross(xChange, yChange);
  if (currentFace == 1) // Not completely robust here!
    normal = -normal;
  normal.normalise();
  cell->normal[0] = (char)(normal.x*127.0f);
  cell->normal[1] = (char)(normal.y*127.0f);
  cell->normal[2] = (char)(normal.z*127.0f);

  cell->radius = radius; 
  numEntries[level]++;
  if (level == m_depth - 1)
    return cell;

  level++;
  radius /= 2.0f;

  // ordered in decreasing x, since it is transparent
  cell->children[3] = generateRecursive(cellList, cell, centre, minX, minY, midX, midY, level, size, radius);
  cell->children[2] = generateRecursive(cellList, cell, centre, midX, minY, maxX, midY, level, size, radius);
  cell->children[1] = generateRecursive(cellList, cell, centre, minX, midY, midX, maxY, level, size, radius);
  cell->children[0] = generateRecursive(cellList, cell, centre, midX, midY, maxX, maxY, level, size, radius);
  return cell;
}
