#pragma once
#include "basics.h"
//#include "Cell.h"
#include "OrientableCell.h"

// Has mass and velocity, so requires an update function
class DynamicCell : public OrientableCell
{
public:
  DynamicCell(Cell* parent, int numChildren, char* childBuffer = NULL);
  DynamicCell();

  Velocity velocity;
  AngularVelocity angularVelocity;
  Mass mass;
  InertiaVector inertiaTensor; // cell oriented along primary tensor axes
  Acceleration gravityEtc; // usually equals gravity
  void setDensity(Density density); // calculates mass and inertia, assuming that rotation is the primary inertia axes
  void update(TimePeriod timeStep); // when cell has moved and needs to be shifted within the hierarchy
  void applyImpulse(const Position& position, const Impulse& impulse);
  void applyTorqueImpulse(const Impulse& torqueImpulse);
  Velocity getVelocity(const Position& position);
  Speed getSpeedChangePerImpulse(const Position& position, const Impulse& impulse, AngularVelocity& angVel); 
  AngularSpeed getAngularSpeedChangePerTorqueImpulse(const TorqueImpulse& torqueImpulse, AngularVelocity& angVel);

  inline AngularVelocity getAngularVelocityChangePerImpulse(const Position& position, const Impulse& impulse)
  {
    TorqueImpulse torqueImpulse = Vector3::cross(position - this->position, impulse);
    return getAngularVelocityChangePerTorqueImpulse(torqueImpulse);
  }
  inline AngularVelocity getAngularVelocityChangePerTorqueImpulse(const Impulse& torqueImpulse)
  {
    AngularVelocity angVel;
    angVel  = rotation.row[0] * (torqueImpulse.dot(rotation.row[0])/inertiaTensor.x);
    angVel += rotation.row[1] * (torqueImpulse.dot(rotation.row[1])/inertiaTensor.y);
    angVel += rotation.row[2] * (torqueImpulse.dot(rotation.row[2])/inertiaTensor.z);
    return angVel;
  }
private:
  void init();
};
