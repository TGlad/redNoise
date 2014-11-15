#pragma once
#include "basics.h"

class DebugDraw 
{
public:
  DebugDraw();
  void drawCircle(class Cell* cell, const Position& posInCellSpace, Length radius, const Colour& colour);
  void drawCell(class Cell* cell, const Colour& colour);
  void drawSphere(class Cell* cell, const Position& posInCellSpace, const Rotation& rotationInCellSpace, Length radius, const Colour& colour);
  void drawLine(Cell* cell, const Position& start, const Position& end, const Colour& colour);

  static const int maxCircles = 20;
  static const int maxSpheres = 180;
  static const int maxLines = 20;
  struct Circle
  {
    class Cell* cell;
    Position position;
    Length radius;
    Colour colour;
  } circles[maxCircles];
  int numCircles;

  struct Sphere
  {
    class Cell* cell;
    Rotation rotation;
    Position position;
    Length radius;
    Colour colour;
  } spheres[maxSpheres];
  int numSpheres;

  struct Line
  {
    class Cell* cell;
    Position start;
    Position end;
    Colour colour;
  } lines[maxSpheres];
  int numLines;

  void drawAll(class View* view);
private:
  void drawCircles(View* view, const Rotation& sceneToView);
  void drawSpheres(View* view, const Rotation& sceneToView);
  void drawLines(View* view, const Rotation& sceneToView);
};
extern DebugDraw g_debugDraw;
