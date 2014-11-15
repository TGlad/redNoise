#include "Terrain.h"
#include "Cell.h"
#include "time.h"
#include "ProceduralExample.h"
#include "OrientableCell.h"
#include <new>

class TerrainSize
{
public:
  Cell cell;
  Cell* children[4];
};

Terrain::Terrain(int depth, Length size, Scale amplitude)
{
  loadTimer.start();
  // This just builds a contiguous list, cells isn't used directly in the render
  this->depth = depth;
  this->amplitude = amplitude;
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
  TerrainSize *contiguous = new TerrainSize[count];
  cells = new Cell*[count]; // one big contiguous array (of references)
  for (int i = 0; i < count; i++)
  {
    // Note that placement new is used here to avoid all the headers and padding associated with 
    // doing lots of little news
    if (i >= count - (width * width))
      cells[i] = new (&(contiguous[i].cell)) Cell(NULL, 0); 
    else
      cells[i] = new (&(contiguous[i].cell)) Cell(NULL, 4, (char *)&(contiguous[i].children)); // this will build them up basically contiguously
  }

  maxWidth = 1<<depth;
  baseArray = new TerrainElement*[maxWidth+1];
  for (int i = 0; i < maxWidth+1; i++)
  {
    float x = 2.0f * (float)(i-maxWidth/2) / (float)maxWidth;
    baseArray[i] = new TerrainElement[maxWidth+1];
    for (int j = 0; j < maxWidth+1; j++)
    {
      float z = 2.0f * (float)(j-maxWidth/2) / (float)maxWidth;
      baseArray[i][j].pos.set(x, z*0.04f, z);
      baseArray[i][j].shade = 1.0f;
    }
  }

  static unsigned int randomseed = (unsigned int)time(NULL);
  srand(randomseed);
  int step = maxWidth/4;
  do
  {
    for (int x = 0; x<maxWidth; x+=step)
      for (int y = 0; y<maxWidth; y+=step)
        averageSquare(x, y, x+step, y+step);
    for (int x = 0; x<maxWidth; x+=step)
      for (int y = 0; y<maxWidth; y+=step)
        generateSquare(x, y, x+step, y+step);
    for (int x = -step/2; x<maxWidth; x+=step)
      for (int y = 0; y<maxWidth; y+=step)
        averageDiamond(x, y, x+step, y+step);
    for (int x = 0; x<maxWidth; x+=step)
      for (int y = -step/2; y<maxWidth; y+=step)
        averageDiamond(x, y, x+step, y+step);
    for (int x = -step/2; x<maxWidth; x+=step)
      for (int y = 0; y<maxWidth; y+=step)
        generateDiamond(x, y, x+step, y+step);
    for (int x = 0; x<maxWidth; x+=step)
      for (int y = -step/2; y<maxWidth; y+=step)
        generateDiamond(x, y, x+step, y+step);
    step /=2;
  } while (step > 1);
  loadTimer.stop();
  loadTimer.print("diamondSquare");
  loadTimer.reset();
  loadTimer.start();

  addVolcano(size); // nice function name 
  volcanoPos *= size;
  loadTimer.stop();
  loadTimer.print("addVolcano");
  loadTimer.reset();
  loadTimer.start();

  // Now generate the cell tree at the same time as generating the terrain
  root = (OrientableCell*)generateRecursive(NULL, Position(0,0,0), 0, 0, maxWidth, maxWidth, 0, size, size);
  root->radius *= 4.0f; // temporary. since we won't be looking away from it, this prevents the view jumping into a different direction
//   OrientableCell* testCell = new OrientableCell(root, 4);
//   testCell->position = root->children[3]->position;
//   testCell->radius = root->children[3]->radius;
//   testCell->colour = root->children[3]->colour;
//   testCell->rotation = RotationVector(0,0,0); //(0, 0.1f, -0.5f));
//  // testCell->scale = 0.5f;
//   for (int i = 0; i<4; i++)
//   {
//     testCell->children[i] = root->children[3]->children[i];
//     testCell->children[i]->parent = testCell;
//   } 
//   root->children[3] = testCell;

  // Clean up lists of pointers
  delete[] cells; 
  for (int i = 0; i < maxWidth+1; i++)
    delete[] baseArray[i];
  delete baseArray;

  loadTimer.stop();
  loadTimer.print("generate recursive");
}

void Terrain::addVolcano(Length size)
{
  int volcanoX = maxWidth/2;
  int volcanoZ = maxWidth/2;
  for (int x = 0; x<=maxWidth; x++)
  {
    for (int y = 0; y<=maxWidth; y++)
    {
      Area distanceToCentre = sqrtf(sqr((float)(x - volcanoX)) + sqr((float)(y - volcanoZ)));
      distanceToCentre /= (float)maxWidth;
      Distance height = 1.0f / distanceToCentre;
      Length minRad = 0.02f;
      if (distanceToCentre < minRad)
        height = 60000.f*(sqr(distanceToCentre)-sqr(minRad)) + (1/minRad);
 //     height -= 4.0f;
      height *= amplitude*0.008f; 
      baseArray[x][y].pos.y += height;
    }
  }
  volcanoPos = baseArray[volcanoX][volcanoZ].pos;
}
static float sidewaysScale = 0.5f;
static float radiosityStrength = 0.5f;
void Terrain::averageDiamond(int minX, int minY, int maxX, int maxY)
{
  int midX = (minX + maxX) / 2;
  int midY = (minY + maxY) / 2;

  if (minX < 0 || maxX > maxWidth)
  {
    if (minX < 0)
      minX = 0;
    if (maxX > maxWidth)
      maxX = maxWidth;
    baseArray[midX][midY].pos = (baseArray[midX][minY].pos + baseArray[midX][maxY].pos)*0.5f;
  }
  else if (minY < 0 || maxY > maxWidth)
  {
    if (minY < 0)
      minY = 0;
    if (maxY > maxWidth)
      maxY = maxWidth;
    baseArray[midX][midY].pos = (baseArray[minX][midY].pos + baseArray[maxX][midY].pos)*0.5f;
  }
  else
    baseArray[midX][midY].pos = (baseArray[minX][midY].pos + baseArray[maxX][midY].pos + baseArray[midX][minY].pos + baseArray[midX][maxY].pos)*0.25f;
  baseArray[midX][midY].shade = (baseArray[minX][midY].shade + baseArray[maxX][midY].shade + baseArray[midX][minY].shade + baseArray[midX][maxY].shade)*0.25f;
}
const float featureScale = 80.0f;
const float amplitudeVariation = 0.5f;
void Terrain::generateDiamond(int minX, int minY, int maxX, int maxY)
{
  int midX = (minX + maxX) / 2;
  int midY = (minY + maxY) / 2;
  if (minX < 0)
    minX = 0;
  if (maxX > maxWidth)
    maxX = maxWidth;
  if (minY < 0)
    minY = 0;
  if (maxY > maxWidth)
    maxY = maxWidth;
  Direction dir1 = baseArray[minX][midY].pos - baseArray[maxX][midY].pos;
  Direction dir2 = baseArray[midX][maxY].pos - baseArray[midX][minY].pos;
  Direction normal = Vector3::normalise(Vector3::cross(dir1, dir2));
  Scale blah = 2.0f*((float)maxX - (float)minX)/(float)maxWidth;
  normal *= blah;
  Scale offset = random();
#define BULGY_ROCKS
#if defined(BULGY_ROCKS)
  if (baseArray[midX][midY].pos.y < 3.0f/featureScale && baseArray[midX][midY].pos.y > 0.5f/featureScale)
  {
    if (blah < 0.05f)
    {
      offset *= 0.75f;
      offset += 0.5f;
    }
  }
  else
#endif
  {
    normal.x *= sidewaysScale;
    normal.z *= sidewaysScale;
  //  normal *= normal.y;
  }
  offset *= amplitude * (1.0f + baseArray[midX][midY].pos.x*amplitudeVariation);
  baseArray[midX][midY].shade += offset*radiosityStrength;
  baseArray[midX][minY].shade -= 0.25f*offset*radiosityStrength;
  baseArray[midX][maxY].shade -= 0.25f*offset*radiosityStrength;
  baseArray[minX][midY].shade -= 0.25f*offset*radiosityStrength;
  baseArray[maxX][midY].shade -= 0.25f*offset*radiosityStrength;

  baseArray[midX][midY].pos += normal * offset;
}

void Terrain::averageSquare(int minX, int minY, int maxX, int maxY)
{
  int midX = (minX + maxX) / 2;
  int midY = (minY + maxY) / 2;
  baseArray[midX][midY].pos = (baseArray[minX][minY].pos + baseArray[maxX][minY].pos + baseArray[minX][maxY].pos + baseArray[maxX][maxY].pos)*0.25f;
  baseArray[midX][midY].shade = (baseArray[minX][minY].shade + baseArray[maxX][minY].shade + baseArray[minX][maxY].shade + baseArray[maxX][maxY].shade)*0.25f;
}

void Terrain::generateSquare(int minX, int minY, int maxX, int maxY)
{
  int midX = (minX + maxX) / 2;
  int midY = (minY + maxY) / 2;
  Direction dir1 = baseArray[maxX][maxY].pos - baseArray[minX][minY].pos;
  Direction dir2 = baseArray[maxX][minY].pos - baseArray[minX][maxY].pos;
  Direction normal = Vector3::normalise(Vector3::cross(dir1, dir2));
  Scale blah = 1.414f * 2.0f*((float)maxX - (float)minX)/(float)maxWidth;
  normal *= blah;
  Scale offset = random();
#if defined(BULGY_ROCKS)
  if (baseArray[midX][midY].pos.y < 3.0f/featureScale && baseArray[midX][midY].pos.y > 0.5f/featureScale)
  {
    if (blah < 0.05f)
    {
      offset *= 0.75f;
      offset += 0.5f;
    }
  }
  else
#endif
  {
    normal.x *= sidewaysScale;
    normal.z *= sidewaysScale;
  //   normal *= normal.y;
  }
  offset *= amplitude * (1.0f + baseArray[midX][midY].pos.x*amplitudeVariation);
  baseArray[midX][midY].shade += offset*radiosityStrength;
  baseArray[minX][minY].shade -= 0.25f*offset*radiosityStrength;
  baseArray[maxX][maxY].shade -= 0.25f*offset*radiosityStrength;
  baseArray[minX][maxY].shade -= 0.25f*offset*radiosityStrength;
  baseArray[maxX][minY].shade -= 0.25f*offset*radiosityStrength;

  baseArray[midX][midY].pos += normal * offset;
}

Cell* Terrain::generateRecursive(Cell* parent, const Position& parentPosWorldSpace, int minX, int minY, int maxX, int maxY, int level, Length size, Length radius)
{
  Cell *cell = NULL;
  bool testProceduralLeaves = false; // Makes the smallest parts actually procedural horizontal textures
  bool testOrientableCells = true;
  if (testProceduralLeaves && (level == depth - 1)) // this is crazy, I'm making the leaf nodes infinitely recurse, as they are proocedural
  {
    cell = new ProceduralExample(NULL, 4);
  }
  else if (testOrientableCells && level == 0)
  {
    OrientableCell* oCell = new OrientableCell(NULL, 4);
    oCell->scale = 1.0f;
    cell = oCell;
  }
  else
  {
    cell = cells[levels[level] + numEntries[level]];
  }
  int midX = (minX + maxX) / 2;
  int midY = (minY + maxY) / 2;
  Position centre = (baseArray[midX][midY].pos + baseArray[minX][minY].pos + baseArray[maxX][maxY].pos + baseArray[minX][maxY].pos + baseArray[maxX][minY].pos)*0.2f; 
  centre *= featureScale;
  cell->parent = parent;
  Direction dir1 = baseArray[maxX][maxY].pos - baseArray[minX][minY].pos;
  Direction dir2 = baseArray[maxX][minY].pos - baseArray[minX][maxY].pos;
  Direction normal = Vector3::cross(dir1, dir2);
  normal.normalise();

  Scale shade = (2*baseArray[midX][midY].shade + baseArray[minX][minY].shade + baseArray[maxX][maxY].shade + baseArray[minX][maxY].shade + baseArray[maxX][minY].shade)/ 6.0f; 
  if (centre.y < 0.0f) // change at sea level
  {
    centre.y *= 0.3f;
    shade = (shade-1.0f)*0.3f + 1.0f; // less radiosity due to flattened ground
    normal.y /= 0.3f;
    normal.normalise();
  }
  if (centre.y < -0.2f) // water
  {
    Distance depth = -0.2f - centre.y;
    centre.y = (centre.y + 0.2f)*0.6f - 0.2f; // refraction simulated (a bit)
    cell->colour.set(0.0f, (float)max(0.0f, 0.9f * (1.0f - 0.3f * depth)), 0.7f);
    cell->colour /= 1.0f + 0.5f * depth; // darkness from depth
  }
  else if (centre.y < 0.0f) // sand
  {
    Distance height = centre.y/0.4f + 0.3f;
    cell->colour.set(0.95f - 0.25f * height, 0.9f - 0.4f * height, 0.25f);
  }
  else 
  {
    // trees exist in on a heuristic
    // they prefer flat areas and avoid high areas
    float flatness = normal.y - 1.0f;
    float lowness = 3.0f - centre.y;
    Distance height = centre.y - 0.0f;
    Colour treeColour(0.0f, 0.45f + 0.04f * height, 0.03f * height);
    Distance snowHeight = centre.y - 3.0f;
    Colour snowColour(0.65f + 0.04f * snowHeight, 0.65f + 0.04f * snowHeight, 0.5f + 0.07f * snowHeight);
    float dif = (flatness + 0.5f)*0.75f;
    if (snowHeight > -0.5f)
      dif -= (snowHeight + 0.5f)*0.1f;
    float blend = clamped(dif*2.0f, 0.0f, 1.0f);
    cell->colour = snowColour * (1.0f-blend) + treeColour*blend;
  }
  centre *= size / featureScale;
  cell->position.set(centre);
  if (parent != NULL)
    cell->position -= parentPosWorldSpace;

  cell->normal[0] = (char)(normal.x*127.0f);
  cell->normal[1] = (char)(normal.y*127.0f);
  cell->normal[2] = (char)(normal.z*127.0f);

  // Assuming there is a horizontal ground here, we penalise tipping to the same extent. I.e. assume sky is brighter than below
  // shade += (normal.y - 0.8f)*3.1415f*radiosityStrength * 0.25f;

  cell->colour *= shade;
  cell->colour.clamp(0.0f, 1.0f);

  //      cell.colour.set((centre.y + 2.0f) / 4.0f, (centre.y + 1.0f) / 2.0f, (centre.y + 4.0f) / 8.0f);
  Scale amp = amplitude * (1.0f + baseArray[midX][midY].pos.x*amplitudeVariation);
  cell->radius = sqrtf((2.0f + amplitude) * radius * radius);
  numEntries[level]++;
  if (level == depth - 1)
    return cell;

  level++;
  radius /= 2.0f;

  // ordered in increasing x
  cell->children[0] = generateRecursive(cell, centre, minX, minY, midX, midY, level, size, radius);
  cell->children[1] = generateRecursive(cell, centre, midX, minY, maxX, midY, level, size, radius);
  cell->children[2] = generateRecursive(cell, centre, minX, midY, midX, maxY, level, size, radius);
  cell->children[3] = generateRecursive(cell, centre, midX, midY, maxX, maxY, level, size, radius);
  return cell;
}