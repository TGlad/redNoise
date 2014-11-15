#pragma once
#include "basics.h"
#include "DynamicCell.h"
#include "Timer.h"

// Has collision properties, so collides and bounces off other cells
class PhysicalCell : public DynamicCell
{
public:
  PhysicalCell(Cell* parent, int numChildren, char* childBuffer = NULL);
  PhysicalCell();
  void update(TimePeriod timeStep, Scale ratioToFarPlane);
  struct CellData // All in physicalCell's parent space
  {
    Cell* cell;
    Position position;
    Rotation rotation;
    Scale scale;
    CellData* children;
    Length radius; // scaled by scale
  };
  Stiffness cellStiffness;
private:
  void init();

  static const int maxContacts = 100; 
  // Physics methods
  struct Contact
  {
    // All values in parent-space
    Position position;
    Direction normal;
    ImpulseScalar totalNormalImpulse;
    ImpulseScalar totalLateralImpulse[2];
    Speed exitSpeed;
    Distance depth;
    Length contactRadius;
    Mass massAlongNormal; // effective mass
    AngularVelocity angVelChangePerNormal; // cached as the calculation requires a bit of work for nonuniform inertias
    Direction tangent[2];
    Mass massAlongTangent[2];
    AngularVelocity angVelChangePerTangent[2];
    Length radius;
  } contacts[maxContacts];
  int numContacts; 
  Length skinWidth;
  Direction totalNormal;
  Length maxContactRadius;
  
  Timer collisionTime, physicsTime, rockUpdateTime;

  void addContact(const Position& objectPos, const Position& searchCellPos, Length objectRadius, Length searchCellRadius, const Direction& normal);
  
  int findContactCount;
  int findGroundCount;
  int findObjectCount;

  void addSmallerObjectsThanRadius(CellData* search, CellData* object, CellData** overlappingObjects, int& numOverlapping, float maxRadius);
  void findSmallerSearchCells(CellData** objects, int numObjects, CellData* search);
  void addDeepestContact(const Position& objectPos, const Position& searchCellPos, Length objectRadius, Length searchCellRadius, const Direction& normal, Length& deepestDepth);
  void findContacts(CellData* object, CellData* search, Length minRadius);
};
