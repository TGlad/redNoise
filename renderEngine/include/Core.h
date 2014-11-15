#pragma once
#include "basics.h"
#include "View.h"
#include "Terrain.h"
#include "Rock.h"
#include "Camera.h"
#include "Cloud.h"

class Core
{
public:
  void init(int screenWidth, int screenHeight);
  void deinit();
  void render();
  void update(TimePeriod timeStep);
  void getHighestHeight(const Position& testPos, Rotation parentToRootRot, Cell* cell, const Position& parentToRootPos, Length size, Distance& height, Weight& weight);
  Screen* screen;
  View* view;
  Terrain* terrain;
  static const int numRocks = 20;
  Rock* rocks[numRocks];
  Cloud* cloud;
  Angle yaw;
  Angle roll;
  Angle pitch;
  Distance height;
  Direction direction;
  Camera* camera;
private:
  void addTerrainIntoSolarSystem();
};
