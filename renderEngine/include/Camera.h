#pragma once
#include "basics.h"
#include "View.h"

class Camera
{
public:
  Camera(View* view);
  void update(TimePeriod timeStep);

  Position targetPos;
  Direction targetDirection;
  TimePeriod smoothTime;

private:
  Velocity velocity;
  AngularVelocity angularVelocity;
  View *view;
};
