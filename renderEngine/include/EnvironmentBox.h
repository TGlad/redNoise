#pragma once
#include "basics.h"
#include "View.h"

class EnvironmentBox 
{
public:
  Length width;
  Rotation faces[6];
  class Screen* screens[6];
  View* currentView;
  Scale timesLargerThanOwner;
  EnvironmentBox(int width, const View* parent, Screen** sharedScreens = NULL);
  ~EnvironmentBox();
  void draw();
};

