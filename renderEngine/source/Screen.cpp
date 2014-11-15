#include "Screen.h"
#include "string.h"

Screen::Screen(int w, int h)
{
  width = w;
  height = h;

  m_depth = 0;
  while (1 << m_depth < w)
    m_depth++;
  // Create a new bitmap.
  image = new Image(m_depth, height);

  closestDistance = new unsigned short*[m_depth + 2];
  for (int i = 0; i < m_depth + 1; i++)
  {
    closestDistance[i] = new unsigned short[w * w]; 
    w /= 2;
  }
  closestDistance[m_depth + 1] = new unsigned short[1 * 1];
  closestDistance[m_depth + 1][0] = 0; // we don't even need to reset this each frame, it just avoids an assert on a fully occluded screen
}

Screen::~Screen()
{
  for (int i = 0; i < m_depth + 2; i++)
    delete closestDistance[i];
  delete closestDistance;
}

void Screen::setDistance(int level, int x, int y, unsigned short bufferDepth)
{
  unsigned short *table = closestDistance[level];
  int shift = m_depth - level;
  int index = x + (y<<shift); // Wrong... may possible have to store the widths!
  unsigned short old = table[index];
  table[index] = bufferDepth;
  int X = x>>1;
  int Y = y>>1;
  level++;

  if (closestDistance[level][X + (Y<<(shift-1))] >= 3)
  {
    if (level <= m_depth && old <= 3)
    {
      // now we have to search the list for the largest
      int xx = X << 1;
      int yy = Y << 1;
      unsigned short smallest = min(min(table[xx + (yy<<shift)], table[xx + 1 + (yy<<shift)]), min(table[xx + ((yy + 1)<<shift)], table[xx + 1 + ((yy + 1)<<shift)]));
      if (smallest > 3)
        setDistance(level, X, Y, smallest); // recursive
    }
  }
  else
    closestDistance[level][X + (Y<<(shift-1))]++; 
}

// A debug only feature
void Screen::drawOcclusion()
{
  int topX = 420;
  for (int level = 3; level < m_depth + 1; level++)
  {
    int topY = ((level-3) * (64 + 2)) + 10;
    for (int i = 0; i < 64; i++)
    {
      for (int j = 0; j < 64; j++)
      {
        int x = i>>(level-3);
        int y = j>>(level-3);
        unsigned short depth = closestDistance[level][x + (y<<(m_depth-level))];
        float shade = (float)depth / 65536.0f;
        image->setPixel(i + topX, j + topY, ScreenColour(Vector3(shade, shade, shade)));
      }
    }
  }
}

void Screen::clear()
{
  int w = width;
  for (int level = 0; level < m_depth + 1; level++)
  {
    memset(closestDistance[level], 0, w*w*sizeof(unsigned short));
    w >>= 1;
  }
}

