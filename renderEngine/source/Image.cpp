#include "Image.h"
#include "glut.h"
#include "string.h"

Image::Image(int widthShift, int height)
{
  this->widthShift = widthShift;
  this->width = 1<<widthShift;
  this->height = height;
  data = new ScreenColour[width * height];
  glGenTextures(1, &textureID);
}

Image::~Image(void)
{
}

void Image::draw()
{
  glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, (GLubyte *)data);
}

void Image::generateTexture()
{
//   for (int x = 0; x<width; x+=10)
//   {
//     for (int j = 0; j<height; j+=10)
//     {
//       setPixel(x, j, Vector3(0.5f, 1.0f, 1.0f));
//     }
//   }
  glBindTexture(GL_TEXTURE_2D, textureID);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // TODO: currently can't use GL_LINEAR as it causes a border against the background colour, and is a bit thinner so you get joining lines
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLubyte *)data);
}
void Image::clear()
{
  memset(data, 0, width*height*4);
}