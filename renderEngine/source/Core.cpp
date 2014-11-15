#include "Core.h"
#include "Screen.h"
#include <conio.h>
#include "Cell.h"
#include "PhysicalCell.h"
#include "EnvironmentBox.h"
#include "DebugDraw.h"
#include <windows.h>

// Make the terrain a patch on the surface of an earth, then place earth around sun, plus add a moon
void Core::addTerrainIntoSolarSystem()
{
  // Using Gm
  DynamicCell* solarSystem = new DynamicCell(NULL, 3); // 3 children, sun, earth and moon
  solarSystem->radius = 6000.0f; // up to pluto

  Cell* sun = new Cell(solarSystem, 0);
  sun->colour = Colour(1,1,0.8f);
  sun->radius = 0.7f; // Gm

  // attach terrain to an earth cell
  DynamicCell* earth = new DynamicCell(solarSystem, 1);
  earth->gravityEtc.setToZero(); // even with the sun sun, net acc is 0 due to orbiting
  earth->scale = 0.000001f; // so earth is measured in km, and the solar system measured in million kms, or Gm
  earth->radius = 42000.f; // km
  earth->position = Position(150.0f, 0.0f, 0.0f); // 150Gm from sun
  TimePeriod lengthOfDay = 60.0f*60.0f*24.0f;
  earth->angularVelocity = AngularVelocity(0.4f, 0.92f, 0.0f) * 2.0f*3.1415f/lengthOfDay; // Tilt and spin of earth
  earth->rotation *= RotationVector(earth->angularVelocity * lengthOfDay * 4.0f/24.0f); // put the day at 4pm
  earth->angularVelocity *= 10.0f; // hack: remove

  earth->children[0] = terrain->root;
  terrain->root->parent = earth;
  terrain->root->position.x = -6378.0f;
  terrain->root->rotation = RotationVector(0, 0, 3.1415f/2.0f); // stick the terrain at the equator
  terrain->root->scale = 0.001f; // terrain is in metres compared to earth which is in km

  Cell* moon = new Cell(solarSystem, 0);
  moon->colour = Colour(0.7f,0.7f,0.7f);
  moon->radius = 0.0017f; // Gm
  moon->position = earth->position + PositionDelta(0, 0, 0.38f);

  Vector3 sunNormal = Vector3::normalise(sun->position - earth->position); // just makes sun bright
  sun->normal[0] = (char)(sunNormal.x*127.0f);
  sun->normal[1] = (char)(sunNormal.y*127.0f);
  sun->normal[2] = (char)(sunNormal.z*127.0f);

  Vector3 moonNormal = Vector3::normalise(earth->position - moon->position); // single moon cell is facing earth
  moon->normal[0] = (char)(moonNormal.x*127.0f);
  moon->normal[1] = (char)(moonNormal.y*127.0f);
  moon->normal[2] = (char)(moonNormal.z*127.0f);

  solarSystem->children[0] = sun;
  solarSystem->children[1] = earth;
  solarSystem->children[2] = moon;
}

void Core::init(int screenWidth, int screenHeight)
{
#if defined(_DEBUG)
  int terrainDepth = 10;
#else
  int terrainDepth = 12;
#endif
  terrain = new Terrain(terrainDepth, 500, 0.2f);
 
  addTerrainIntoSolarSystem();

  for (int i = 0; i<numRocks; i++)
  {
    float maxRockRadius = 10.0f;
    float exponent = random(-5.0f, 0.0f);
    float rsize = powf(2.0f, exponent);
    float rockRadius = maxRockRadius * rsize;
    int detail = (int)(exponent + 7);
    rocks[i] = new Rock(detail, 0.3f, rockRadius);
    rocks[i]->root->position = Vector3(-80.0f + (float)i * 45.0f, 175.f, -40.0f);
    terrain->root->addChild(rocks[i]->root);
  }
//   cloud = new Cloud(7, 0.3f, 40);
//   cloud->root->position.set(0, 25, -240.0f);
//   terrain->root->addChild(cloud->root);

  screen = new Screen(screenWidth, screenHeight);
  view = new View(90.0f, screenWidth, screenHeight, terrain->root);
  view->generateEnvironment(screen->width / 2);

  view->position.set(0, 30.0f, -125.0f);
  view->lookAt(view->position + PositionDelta(0, 0.0f, 1.0f));

  camera = new Camera(view);

  height = 3.f;
}

void Core::deinit()
{
  delete camera;
  delete view;
  delete terrain;
}
void Core::render()
{
  view->recordToScreen(screen);
//   {
// //    Cell* test = cloud->root->children[0]->children[0]->children[3]->children[3];//->children[3]->children[3];
//     g_debugDraw.drawCell(test->children[View::orderings[view->orderIndex][0]], Colour(1,0,0));
//     g_debugDraw.drawCell(test->children[View::orderings[view->orderIndex][1]], Colour(0,1,0));
//     g_debugDraw.drawCell(test->children[View::orderings[view->orderIndex][2]], Colour(0,0,1));
//     g_debugDraw.drawCell(test->children[View::orderings[view->orderIndex][3]], Colour(1,1,1));
//   }  
  view->renderBox();
  view->render();
}

// This core will eventually become character. Since a character contains a view, and a size etc.


void Core::getHighestHeight(const Position& testPos, Rotation cellToRootRot, Cell* cell, const Position& cellToRootPos, Length size, Distance& height, Weight& weight)
{
  PositionDelta dif = testPos - cellToRootPos;
  dif.y = 0.0f;
  Area radi2 = cell->radius*cell->radius;
  Scale dist = (radi2 - dif.magnitudeSquared())/radi2; // i.e. centred = 1, edge = 0
  if (dist <= 0.0f)
  {
    return; // not valid
  }
  if (cell->numChildren == 0 || cell->radius < size) 
  {
    Distance h = cellToRootPos.y + cell->radius;
    height += h*dist;
    weight += dist;
    return;
  }
  for (int i = 0; i<cell->numChildren; i++)
  {
    Cell* child = cell->children[i];
    PositionDelta offset = cellToRootRot.rotateVector(child->position);
    Position childToRootPos = cellToRootPos + offset;
    Rotation childToRootRot = cellToRootRot;
    if (child->type >= Cell::Orientable)
    {
      OrientableCell *oCell = (OrientableCell*)cell;
      childToRootRot *= oCell->rotation;
      childToRootRot.scale(oCell->scale);
//      childToRootPos *= oCell->scale;
    }
    getHighestHeight(testPos, childToRootRot, child, childToRootPos, size, height, weight);
  }
}
static Timer rockTimer;
static int FRAME = 0;
static int frame = 0;
void Core::update(TimePeriod timeStep)
{
  FRAME++;
  Speed angularSpeed = 3.0f;
  Speed speed = 10.0f;
  Direction right = Vector3::cross(camera->targetDirection, Vector3(0,1,0));
  if (GetKeyState(VK_SHIFT)<0)
  {
//     if (GetKeyState(VK_LEFT)<0)
//       camera->targetRoll -= angularSpeed*timeStep;
//     if (GetKeyState(VK_RIGHT)<0)
//       camera->targetRoll += angularSpeed*timeStep;
    if (GetKeyState(VK_UP)<0)
      camera->targetDirection.rotate(right * (angularSpeed*timeStep));
    if (GetKeyState(VK_DOWN)<0)
      camera->targetDirection.rotate(right * -(angularSpeed*timeStep));
  }
  else if (GetKeyState(VK_CONTROL)<0)
  {
    if (GetKeyState(VK_LEFT)<0)
      camera->targetPos -= right * speed*timeStep;
    if (GetKeyState(VK_RIGHT)<0)
      camera->targetPos += right * speed*timeStep;
    if (GetKeyState(VK_UP)<0)
      height += speed*timeStep;
    if (GetKeyState(VK_DOWN)<0)
      height -= speed*timeStep;
  }
  else
  {
    if (GetKeyState(VK_LEFT)<0)
      camera->targetDirection.rotate(Vector3(0,1,0) * (angularSpeed*timeStep));
    if (GetKeyState(VK_RIGHT)<0)
      camera->targetDirection.rotate(Vector3(0,1,0) * -(angularSpeed*timeStep));
    if (GetKeyState(VK_UP)<0)
      camera->targetPos += view->rotation.forwards() * speed*timeStep;
    if (GetKeyState(VK_DOWN)<0)
      camera->targetPos -= view->rotation.forwards() * speed*timeStep;
  }

  // Now we know which cell he is in, we just need to interact with the children of this cell
  // The aim is to get the height under the character
  Distance highestHeight = 0.0f;
  Weight weight = 0.0f;
  getHighestHeight(camera->targetPos, RotationVector(0,0,0), (Cell *)view->parent, Position(0,0,0), height*0.5f, highestHeight, weight);
  if (weight > 0.0f)
    camera->targetPos.y = height + (highestHeight/weight);
  // smoothly updates view based on target pos and direction
  camera->update(timeStep);

  int i = frame / 8;
  if (frame%8 == 0 && i<numRocks)
  {
    // rocks[i]->root->reparentTo(terrain->root);
      // TODO: need a wrapper for this, moving a cell to a location relative to a specified cell.
      rocks[i]->root->parent->removeChild(rocks[i]->root);
      terrain->root->addChild(rocks[i]->root);
      rocks[i]->root->gravityEtc.set(0.0f, -9.8f, 0.0f);
      rocks[i]->root->scale = rocks[i]->size;

    Scale velocityScale = 10.0f;//5.0f / sqrtf(rocks[i]->size);
    DynamicCell* rock = rocks[i]->root;
    rock->velocity = Velocity(random(), 0.0f, random());
    rock->position = terrain->volcanoPos + rock->velocity*3.0f + Position(0, rocks[i]->size*2.0f + 2.0f, 0);
    rock->velocity -= view->rotation.forwards() * 0.5f;
    rock->angularVelocity = 1.f*Vector3::cross(Direction(0,1,0), rock->velocity);
    rock->velocity.y += random(3.0f, 4.0f);
    rock->velocity *= velocityScale;
  }
  frame++;
  if (frame >= 500)
    frame = 0;
  // update the rock with a bit of a spin
  rockTimer.start();
  for (int i = 0; i<numRocks; i++)
  {
    // Below is wrong!: since rock root position isn't in view space.
    Scale ratio = 0.0f;//(rocks[i]->root->position - view->position).magnitude() / view->farPlane; // used as optimisation
    if (rocks[i]->root->parent->radius > 2000.0f && rocks[i]->root->position.y < (rocks[i]->root->radius*rocks[i]->root->scale))
      rocks[i]->root->cellStiffness = 30.0f;
    else
      rocks[i]->root->cellStiffness = 2000.0f; // is water! funny way to check
    rocks[i]->root->update(timeStep, ratio); 
  }
  rockTimer.stop();

  ((DynamicCell*)terrain->root->parent)->update(timeStep); // just spins the earth

//   if (FRAME%10==0)
//   {
    rockTimer.print("rockTime");
    rockTimer.reset();
//  }
}
