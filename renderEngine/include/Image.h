#pragma once
#include "basics.h"
#include "ScreenColour.h"

class Image
{
public:
  int width;
  int height;
  int widthShift;
  ScreenColour *data;
  GLuint textureID;

  inline unsigned char& alpha(int x, int y)
  {
    ScreenColour *ptr = data + x + (y<<widthShift);
    return ptr->alpha;
  }
  inline void setPixel(int x, int y, const ScreenColour& colour)
  {
    ScreenColour *ptr = data + x + (y<<widthShift);
    *ptr = colour;
  }
  inline void blendPixel(int x, int y, const ScreenColour& colour, float blend)
  {
    ScreenColour *ptr = data + x + (y<<widthShift);
    unsigned char B = (unsigned char)(blend*255.0f);
    unsigned char invB = 255 - B;
    // TODO: can we do this as a single int32 average? does that work?
    ptr->red = (colour.red*B + ptr->red*invB) >> 8;
    ptr->green = (colour.green*B + ptr->green*invB) >> 8;
    ptr->blue = (colour.blue*B + ptr->blue*invB) >> 8;
    if (ptr->alpha==0) // TODO: we wouldn't have to do this is we rendered the background back onto the foreground buffer
      ptr->alpha = B; // The problem with this is that it doesn't blend multiple properly... 
  }
  void draw();
  void clear();
  void generateTexture();

  Image(int widthShift, int height);
  ~Image(void);
};
