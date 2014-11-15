#pragma once
#include "basics.h"
#include "glut.h"
#include "Timer.h"
#include "OrientableCell.h"

class View : public OrientableCell // The view is a cell, so it can move around like other cells do
{
public:
  Scale xRatio, yRatio;
  int orderIndex;
  int stage;
  Angle fieldOfViewX;
//  Angle fieldOfViewY;
  Scale xScale;
  Scale yScale;
  float screenW;
  float screenH;
  Length vNearPlane;
  Length nearPlane;
  Length farPlane;
  Scale sizeRelativeToRootView;
  Scale fogScale;
  class EnvironmentBox* box;
  class Screen* screen; // so every view can record itself

  View(Angle fieldOfView, int width, int height, Cell* root);
  void setPlanes(Length nearPlane, Length farPlane);
  void generateEnvironment(int width);
  void renderCheckBounds(const Rotation* parentToViewRot, Scale parentToViewScale, const Position& parentToViewPos, Cell* cell, int level, Length width);
  bool reroot(const Cell* cell, const Cell* avoidChild, bool parentChecked, Length radius);
  void removeExcessProceduralCells(const Position& parentToCell, const Cell* cell);
  void record();
  void recordToScreen(class Screen* screen);
  void renderBox();
  void render();
  void update(); // to update parent based on new position
  Timer recordTime, renderTime, renderBoxTime;
  static const int orderings[8][4];
  inline int getDepth(float distance){ return (unsigned short)(logB + logA/(distance+logC)); }
private:
  Scale logA;
  Scale logB;
  Scale logC;
  float halfInvScreenW;
  Direction lightDirectionViewSpace;
  Direction oldForwards; // used to calculate rotation speed
  int frame;
  int boxSideIndex;
  Direction findSun();
  void recordToScreenInternal(Screen* screen);
};