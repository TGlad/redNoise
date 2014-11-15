#include "DynamicCell.h"

DynamicCell::DynamicCell() : OrientableCell()
{
  type = Cell::Dynamic;
  init();
}

DynamicCell::DynamicCell(Cell* parent, int numChildren, char* childBuffer) : OrientableCell(parent, numChildren, childBuffer)
{
  type = Cell::Dynamic;
  init();
}

void DynamicCell::init()
{
  velocity.setToZero();
  angularVelocity.setToZero();
  gravityEtc.set(0, -9.8f, 0); // a good default
}

void DynamicCell::update(TimePeriod timeStep)
{
  position += velocity*timeStep;
  rotation *= Rotation(RotationVector(angularVelocity*timeStep));
  velocity += gravityEtc*timeStep; 

  Rotation oldRotation = rotation;
  Scale oldScale = scale;
  Cell* oldParent = parent;
  OrientableCell::update(radius);
  if (parent != oldParent)
  {
    // TODO: Why are these rotates and inverse rotates the wrong way around??? Argh!
    velocity.inverseRotate(oldRotation); // into world space
    velocity.rotate(rotation); // back into parent space
    velocity *= scale / oldScale;

// maybe we shouldn't rotate the gravity vector... not sure
    gravityEtc.inverseRotate(oldRotation); // into world space
    gravityEtc.rotate(rotation); // back into parent space
    gravityEtc *= scale / oldScale;

    angularVelocity.inverseRotate(oldRotation); // into world space
    angularVelocity.rotate(rotation); // back into parent space
  }
}


void DynamicCell::applyImpulse(const Position& position, const Impulse& impulse)
{
  velocity += impulse / mass;
  angularVelocity += getAngularVelocityChangePerImpulse(position, impulse);
}

void DynamicCell::applyTorqueImpulse(const Impulse& torqueImpulse)
{
  angularVelocity += getAngularVelocityChangePerTorqueImpulse(torqueImpulse);
}

Vector3 DynamicCell::getVelocity(const Position& position)
{
  return velocity + Vector3::cross(angularVelocity, position - this->position);
}

Speed DynamicCell::getSpeedChangePerImpulse(const Position& position, const Impulse& impulse, AngularVelocity& angVel)
{
  angVel = getAngularVelocityChangePerImpulse(position, impulse);
  Velocity velChange = Vector3::cross(angVel, position - this->position);
  return (1.0f / this->mass) + velChange.dot(impulse);
}

AngularSpeed DynamicCell::getAngularSpeedChangePerTorqueImpulse(const TorqueImpulse& torqueImpulse, AngularVelocity& angVel)
{
  angVel = getAngularVelocityChangePerTorqueImpulse(torqueImpulse);
  return angVel.dot(torqueImpulse);
}

// TODO: make more accurate
void DynamicCell::setDensity(Density density)
{
  float rad = radius * scale;
  if (numChildren == 0)
  {
    // simple approximation by assuming the outer sphere is close to the actual shape
    mass = density*rad*rad*rad*3.14157f * 4.0f/3.0f;
    InertiaScalar momentOfInertia = 0.4f*mass*rad*rad;
    inertiaTensor.set(momentOfInertia, momentOfInertia, momentOfInertia);
    return;
  }
  // how do we work out the inertia tensor on each axis? 
  // the simplest algorithm is to look just at the direct children and add their components
  mass = 0.0f;
  inertiaTensor.setToZero();
  for (int i = 0; i<numChildren; i++)
  {
    Length childRad = children[i]->radius * scale;
    if (children[i]->type >= Cell::Orientable)
      childRad *= ((OrientableCell*)children[i])->scale;
    Mass childMass = density*childRad*childRad*childRad*3.14157f * 4.0f/3.0f;
    InertiaScalar childInertia = 0.4f*childMass*childRad*childRad;

    // how do we add the inertia from an offset sphere?
    mass += childMass;
    Vector3 posSquared = children[i]->position * children[i]->position;
    inertiaTensor += InertiaVector(childInertia, childInertia, childInertia) + childMass*posSquared; // parallel axis theorem
  }
}
