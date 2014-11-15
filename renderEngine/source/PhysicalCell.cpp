#include "PhysicalCell.h"
#include "DebugDraw.h"

const float airResistance = 0.05f;

PhysicalCell::PhysicalCell() : DynamicCell()
{
  type = Cell::Physical;
  init();
}

PhysicalCell::PhysicalCell(Cell* parent, int numChildren, char* childBuffer) : DynamicCell(parent, numChildren, childBuffer)
{
  type = Cell::Physical;
  init();
}

void PhysicalCell::init()
{
  findContactCount = 0;
  findGroundCount = 0;
  findObjectCount = 0;
  numContacts = 0;
  maxContactRadius = radius*scale * 0.5f; 
  cellStiffness = 2000.0f;
}

// Our algorithm here is not a simple recursive function, it is effectively two interleaved recursives, so we
// need to store a stack for the recursively calculated object positions, and do direct recursion just on the search cells
static PhysicalCell::CellData objectStack[100]; 
static int stackSize;

static int frame = 0;
void PhysicalCell::update(TimePeriod timeStep, Scale ratioToFarPlane)
{
  rockUpdateTime.start();
  skinWidth = radius*scale*0.02f;
//  g_debugDraw.drawSphere(root, Position(0, 0.5f, 0), RotationVector(0.5f, 0,0), 0.5f, Colour(0,1,1));
//  g_debugDraw.drawCell(root, Colour(0.5f,0,0));
//  g_debugDraw.drawLine(root, Position(0,0,0), Position(0, 1.0f, 0), Colour(0,1,0));
//   for (int i = 0; i<6; i++)
//   {
//     char *norms = &root->children[i]->normal[0];
//     g_debugDraw.drawLine(root->children[i], Position(0,0,0), PositionDelta((float)norms[0]/127.0f, (float)norms[1]/127.0f, (float)norms[2]/127.0f), Colour(1,1,0));
//   }
  frame++;
  if (numContacts == 0) // in the air, so adjust scale
  {
    Scale blend = clamped(ratioToFarPlane, 0.0f, 1.0f);
    maxContactRadius = radius*scale * 0.2f + blend*0.3f; // 0.2 to 0.5
  }

  position.y += maxContactRadius;
  velocity /= (1.0f + airResistance*timeStep);
  angularVelocity /= (1.0f + airResistance*timeStep);
  DynamicCell::update(timeStep);

  // pull from cell properties
  Scale staticFrictionCoeff[2] = {1.0f, 1.0f};
  ImpulseScalar stickinessMaxImpulse[2] = {0.0f, 0.0f};
  // Sharp is 20,000
  // Soft is 50
  Stiffness stiffness[2] = {this->cellStiffness, 2000.0f};  // This is a proper material property per volume for a given density
  DampingRatio dampingRatio[2] = {1.0f, 1.0f};

  float groundMass = 1e10f; // large mass for the earth
  float groundScale = 1.0f;
  float groundRadius = 6000.0f;
  
  // Proper material property, matches the combined effect of several cells with the same stiffness acting together.
  Mass m0 = mass;
  Mass m1 = groundMass;
  Stiffness fullStiffness0 = stiffness[0] / (radius*scale);
  Stiffness fullStiffness1 = stiffness[1] / (groundRadius*groundScale);
  Strength s0 = sqr(fullStiffness0);
  Strength s1 = sqr(fullStiffness1);
  Damping d0 = 2.0f*fullStiffness0*dampingRatio[0];
  Damping d1 = 2.0f*fullStiffness1*dampingRatio[1];

  Strength s = s0*m0*s1*m1/(s0*m0+s1*m1);
  s /= m0;
  Damping  d = d0*m0*d1*m1/(d0*m0+d1*m1);
  d /= m0;

  Scale bounce = max(0.0f, 1.0f - (d/(2.0f*sqrtf(s)))); // we can hopefully cache these values until the contacts change

  Scale staticFriction = staticFrictionCoeff[0]*staticFrictionCoeff[1];
  Scale overRelaxation = 1.4f; // don't go much higher
  ImpulseScalar stickiness = stickinessMaxImpulse[0]+stickinessMaxImpulse[1];

  if (numContacts > 0) // in previous frame... 
  {
    // Cheap version of roll and spin damping, you have to manually set these damping values
    // TODO: these should be averaged from contact properties in future
    const Damping spinDamping = 2.0f;
    const Damping rollDamping = 1.0f;
    AngularSpeed scaledSpinSpeed = angularVelocity.dot(totalNormal)/totalNormal.magnitudeSquared();
    AngularVelocity rollVel = angularVelocity - totalNormal*scaledSpinSpeed;
    // Note: non-spherical inertias are ignored here in order to keep the damping stable, but for light damping this will be rarely noticeable
    scaledSpinSpeed /= 1.0f + spinDamping*timeStep; // implicit damping
    rollVel /= 1.0f + rollDamping*timeStep; // implicit damping
    angularVelocity = rollVel + totalNormal*scaledSpinSpeed;
  }
  rockUpdateTime.stop();

  collisionTime.start();




  numContacts = 0;
  totalNormal.setToZero();


  CellData* object = &objectStack[0];
  object->cell = this;
  object->position = position;
  object->rotation = rotation;
  object->scale = scale;
  object->radius = object->scale * object->cell->radius;
  object->children = NULL;

  CellData search; // make this the parent of the root
  search.cell = parent;
  search.position = Position(0,0,0);
  search.rotation = RotationVector(0,0,0);
  search.scale = 1.0f;
  search.radius = search.scale * search.cell->radius;

  findContacts(object, &search, maxContactRadius);
  stackSize = 1;
//  findSmallerSearchCells(&object, 1, &search); // we know that search cell is larger than object as it is object's parent
  collisionTime.stop();

  position.y -= maxContactRadius;

  if (numContacts == 0) 
    return; 

  physicsTime.start();
  // Now we have the set of contacts, we need to do an LCP in order to avoid penetration. 
  // For now we assume the 'ground' is stationary, and we use SOR to resolve the contacts, including friction
  InverseMass invMass = 1.0f/mass;
  Velocity oldVel = velocity;
  for (int iterations = 0; iterations<4; iterations++) 
  {
    for (int i = 0; i<numContacts; i++)
    {
      Contact *contact = &contacts[i];
      float contactScale = sqr(contact->radius / (radius*scale));
      Scale S = s*contactScale;
      Scale D = d*contactScale;
      Scale den = 1.0f/(1.0f + S*timeStep*timeStep + D*timeStep); 
      Frequency depthResistance = S*timeStep*den;  
      Scale velocityResistance = (S*timeStep*timeStep + D*timeStep)*den;   

      // normal force
      Speed contactSpeed = getVelocity(contact->position).dot(contact->normal);
      Speed requiredVelocityChange;
//       if (s > 5.0f*sqr(3.1415f/timeStep))
//       {
//         Length depth = contact->depth;
//         if (-contact->exitSpeed > 2.0f*9.8f*timeStep) // don't do bounce if in rest state
//           depth = -contact->exitSpeed*bounce*timeStep;
//         requiredVelocityChange = depth*depthResistance - contactSpeed*velocityResistance;
//       }
//       else
//       {
//         requiredVelocityChange = (contact->depth + contact->exitSpeed*timeStep)*depthResistance - contactSpeed*velocityResistance;
//       }
      requiredVelocityChange = contact->depth*depthResistance - contactSpeed*velocityResistance;
      ImpulseScalar normalImpulse = requiredVelocityChange * contact->massAlongNormal;
      normalImpulse -= contact->totalNormalImpulse*den;

      ImpulseScalar newImpulse = max(stickiness, contact->totalNormalImpulse + normalImpulse);
			normalImpulse = newImpulse - contact->totalNormalImpulse;
			contact->totalNormalImpulse = newImpulse;

      normalImpulse *= overRelaxation;
      
      velocity += contact->normal * (normalImpulse * invMass);
      angularVelocity += contact->angVelChangePerNormal * normalImpulse;

      ImpulseScalar maxFrictionImpulse = contact->totalNormalImpulse * staticFriction;
#define FRICTION
#if defined(FRICTION)
      // friction force
      Velocity contactVelocity = getVelocity(contact->position);
      ImpulseScalar frictionImpulse[2];
      for (int t = 0; t<2; t++)
      {
        Speed tangentSpeed = contactVelocity.dot(contact->tangent[t]);
        frictionImpulse[t] = -tangentSpeed * contact->massAlongTangent[t];
        contact->totalLateralImpulse[t] += frictionImpulse[t];
      }

      float mag2 = sqr(contact->totalLateralImpulse[0]) + sqr(contact->totalLateralImpulse[1]);
      if (mag2 > sqr(maxFrictionImpulse)) // clamped force
      {
        float magScale = maxFrictionImpulse / sqrtf(mag2);
        frictionImpulse[0] += contact->totalLateralImpulse[0]*(magScale - 1.0f);
        frictionImpulse[1] += contact->totalLateralImpulse[1]*(magScale - 1.0f);
        contact->totalLateralImpulse[0] *= magScale;
        contact->totalLateralImpulse[1] *= magScale;
      }
      velocity += (frictionImpulse[0]*contact->tangent[0] + frictionImpulse[1]*contact->tangent[1]) * (overRelaxation * invMass);
      angularVelocity += contact->angVelChangePerTangent[0] * (frictionImpulse[0] * overRelaxation);
      angularVelocity += contact->angVelChangePerTangent[1] * (frictionImpulse[1] * overRelaxation);
#endif
    }
  }
  float totNormImp = 0.0f;
  float totTorqueImp = 0.0f;
  for (int i = 0; i<numContacts; i++)
  {
    totNormImp += contacts[i].totalNormalImpulse;
  }
  Velocity velDif = velocity - oldVel;

  physicsTime.stop();

//   if (frame%30 == 0)
//   {
//     rockUpdateTime.print("rock update time");
//     collisionTime.print("collision time");
//     physicsTime.print("physics time");
//     collisionTime.reset();
//     physicsTime.reset();
//     rockUpdateTime.reset();
// 
//     printf("findObjectCount: %d\n", findObjectCount/10);
//     printf("findGroundCount: %d\n", findGroundCount/10);
//     findObjectCount = 0;
//     findGroundCount = 0;
//     printf("numContacts snapshot: %d\n", numContacts);
//   }

}

// normal should be searchCell normal - object normal. Normalised
// An improvement might be to blend the contacts together
void PhysicalCell::addDeepestContact(const Position& objectPos, const Position& searchCellPos, Length objectRadius, Length searchCellRadius, const Direction& normal, Length& deepestDepth)
{
  if (numContacts == maxContacts-1) // no more memory to add contact with
    return;
  PositionDelta toObject = objectPos - searchCellPos;
  Distance dot = toObject.dot(normal);
  Position nearestPos = toObject - normal*dot;
  Area depthSqr = sqr(objectRadius + searchCellRadius) - nearestPos.magnitudeSquared();
  Length newDepth = depthSqr>0.0f ? sqrtf(depthSqr) : 0.0f; // TODO: add an assert here
  newDepth -= dot;
  if (newDepth < deepestDepth)
    return; // not the deepest
  deepestDepth = newDepth;

  // add to contact list
  Contact *newContact = &contacts[numContacts];
  newContact->normal = normal;
  newContact->depth = newDepth - skinWidth; // skin width

  totalNormal += normal;
  if (newContact->depth < 0.0f)
    newContact->depth = 0.0f;
  newContact->position = objectPos; // don't provide any rotation in a sphere-sphere collision
  newContact->totalNormalImpulse = 0.0f;
  newContact->totalLateralImpulse[0] = newContact->totalLateralImpulse[1] = 0.0f;
  newContact->exitSpeed = getVelocity(newContact->position).dot(newContact->normal); 
  newContact->contactRadius = 1.0f/((1.0f/objectRadius) + (1.0f/searchCellRadius));
  newContact->massAlongNormal = 1.0f/getSpeedChangePerImpulse(newContact->position, newContact->normal, newContact->angVelChangePerNormal);

  // lateral friction
  newContact->tangent[1] = Vector3::normalise(Vector3::cross(newContact->position - position, newContact->normal));
  newContact->tangent[0] = Vector3::cross(newContact->tangent[1], newContact->normal);
  newContact->massAlongTangent[0] = 1.0f/getSpeedChangePerImpulse(newContact->position, newContact->tangent[0], newContact->angVelChangePerTangent[0]);
  newContact->massAlongTangent[1] = 1.0f/getSpeedChangePerImpulse(newContact->position, newContact->tangent[1], newContact->angVelChangePerTangent[1]);
  newContact->radius = objectRadius;
}

// Two ways to do this:
// 1. search cell function generates a dynamic list of object cells that intersect this cell and are smaller than this search cell 
// from this it then generates a new list for each child search cell. 
// Advantage: at the deepest level you immediately have you list of intersections for each search cell
// Disadvantage: is asymettric, so wouldn't work on object-object collision, and requires dynamic lists

// 2. We have a stack for the search cells as well
// Advantage: possibly more efficient, if it culls based on more evenly matched radiuses

// Decision, made copy of code, and trying out method 1, hopefully more optimised for ground/object contact


// This algorithm so far is not very good, if the search cell has a tiny child then all the objects will have to recurse
// all the way to their leaves, creating a deep list.

// Add object to list, or if it is bigger than specified radius then add children to list
void PhysicalCell::addSmallerObjectsThanRadius(CellData* search, CellData* object, CellData** overlappingObjects, int& numOverlapping, float maxRadius)
{
  Length distSqr = (search->position - object->position).magnitudeSquared();
  if (distSqr > sqr(search->radius + object->radius))
    return; // no overlap
  if (object->radius < maxRadius || object->cell->numChildren==0)
  {
    overlappingObjects[numOverlapping++] = object;
    if (numOverlapping == 20)
    {
      int h = 3;
      h++;
    }
    return;
  }
  int numChildren = object->cell->numChildren;
  // split object into child cells, check whether these are already cached from previous call
  if (object->children != NULL)
  {
    for (int i = 0; i<numChildren; i++)
      addSmallerObjectsThanRadius(search, &object->children[i], overlappingObjects, numOverlapping, maxRadius);
    return;
  }
  // not in cache, so generate the children:
  object->children = &objectStack[stackSize];
  int size = stackSize;
  stackSize += numChildren;
  for (int i = 0; i < numChildren; i++)
  {
    CellData& child = objectStack[size++];
    if (stackSize == 100)
    {
      int h = 3;
      h++;
    }
    child.cell = object->cell->children[i];
    child.position = object->position + object->rotation.rotateVector(child.cell->position)*object->scale;
    child.rotation = object->rotation;
    child.scale = object->scale;
    child.children = NULL;
    if (child.cell->type >= Cell::Orientable)
    {
      OrientableCell* oChild = (OrientableCell*)(child.cell);
      child.rotation *= oChild->rotation;
      child.scale *= oChild->scale;
    }
    child.radius = child.scale*child.cell->radius;
 //   g_debugDraw.drawCell(child.cell, Colour(1,1,1));
    findObjectCount++; // for debugging
    addSmallerObjectsThanRadius(search, &child, overlappingObjects, numOverlapping, maxRadius);
  }
}

// TODO: put radius into cellData.

// assuming search radius is greater than object radius
// prerequisites: search radius is bigger than all object radii
void PhysicalCell::findSmallerSearchCells(CellData** objects, int numObjects, CellData* search)
{
  if (numObjects >= 20)
  {
    int h =3;
    h++;
  }
  CellData* overlappingObjects[20]; // can alloca here
  int numOverlapping = 0;
  Length maxRad = max(search->radius, maxContactRadius);
//   if ((search->position - Vector3(46.8, 0, -46.8)).magnitude() < 0.2f && numObjects == 1 && abs(objects[0]->radius - 43.0f)<2.0f)
//   {
//     int h = 3;
//   }
  for (int i = 0; i<numObjects; i++)
    addSmallerObjectsThanRadius(search, objects[i], overlappingObjects, numOverlapping, maxRad);
  if (numOverlapping == 0)
    return;


  if (search->cell->numChildren == 0 || search->radius < maxContactRadius)
  {
    // Since search radius is bigger than all object radiuses, we find the deepest contact here
    float depth = -1e10f; 
    Direction normal = Direction(search->cell->normal);
    normal.normalise();
    for (int i = 0; i<numOverlapping; i++)
    {
//#define CULL_CONTACTS // Currently not making much difference at all
#if !defined(CULL_CONTACTS)
      depth = -1e10f; 
#endif
      addDeepestContact(overlappingObjects[i]->position, search->position, overlappingObjects[i]->cell->radius*overlappingObjects[i]->scale, search->radius, normal, depth);
#if !defined(CULL_CONTACTS)
      numContacts++;
#endif
    }
#if defined(CULL_CONTACTS)
    numContacts++;
#endif
    return;
  }
  for (int i = 0; i < search->cell->numChildren; i++)
  {
    CellData child;
    child.cell = search->cell->children[i];
    child.position = search->position + search->rotation.rotateVector(child.cell->position)*search->scale;
    child.rotation = search->rotation;
    child.scale = search->scale;
    if (child.cell->type >= Cell::Orientable)
    {
      OrientableCell* oChild = (OrientableCell*)(child.cell);
      child.rotation *= oChild->rotation;
      child.scale *= oChild->scale;
    }
    child.radius = child.scale * child.cell->radius;
    findGroundCount++; // for debugging

    findSmallerSearchCells(overlappingObjects, numOverlapping, &child);
  }
}



// normal should be searchCell normal - object normal. Normalised
void PhysicalCell::addContact(const Position& objectPos, const Position& searchCellPos, Length objectRadius, Length searchCellRadius, const Direction& normal)
{
  if (numContacts == maxContacts-1) // no more memory to add contact with
    return;
  // add to contact list
  Contact *newContact = &contacts[numContacts++];
  newContact->normal = normal;
  totalNormal += normal;
  // This is the line to get right.....
  PositionDelta toObject = objectPos - searchCellPos;
  Distance dot = toObject.dot(normal);
  Position nearestPos = toObject - normal*dot;
  Area depthSqr = sqr(objectRadius + searchCellRadius) - nearestPos.magnitudeSquared();
  newContact->depth = depthSqr>0.0f ? sqrtf(depthSqr) : 0.0f; // TODO: add an assert here
  newContact->depth -= dot;
  newContact->depth -= skinWidth; // skin width
  if (newContact->depth < 0.0f)
    newContact->depth = 0.0f;
  newContact->position = objectPos; // don't provide any rotation in a sphere-sphere collision
  newContact->totalNormalImpulse = 0.0f;
  newContact->totalLateralImpulse[0] = newContact->totalLateralImpulse[1] = 0.0f;
  newContact->exitSpeed = getVelocity(newContact->position).dot(newContact->normal); 
  newContact->contactRadius = 1.0f/((1.0f/objectRadius) + (1.0f/searchCellRadius));
  newContact->massAlongNormal = 1.0f/getSpeedChangePerImpulse(newContact->position, newContact->normal, newContact->angVelChangePerNormal);

  // lateral friction
  newContact->tangent[1] = Vector3::normalise(Vector3::cross(newContact->position - position, newContact->normal));
  newContact->tangent[0] = Vector3::cross(newContact->tangent[1], newContact->normal);
  newContact->massAlongTangent[0] = 1.0f/getSpeedChangePerImpulse(newContact->position, newContact->tangent[0], newContact->angVelChangePerTangent[0]);
  newContact->massAlongTangent[1] = 1.0f/getSpeedChangePerImpulse(newContact->position, newContact->tangent[1], newContact->angVelChangePerTangent[1]);
  newContact->radius = objectRadius;
}

// starting from the search cell (a high level, large cell), find the child contacts under a certain size
void PhysicalCell::findContacts(CellData* object, CellData* search, Length minRadius)
{
  Length distSqr = (search->position - object->position).magnitudeSquared();
  Length searchRadius = search->cell->radius * search->scale;
  Length objectRadius = object->cell->radius * object->scale;
  if (distSqr > sqr(searchRadius + objectRadius))
    return; // no overlap here

  // We have an overlap with this cell, so we do one of two things, 
  // 1. if searchCell is bigger than this cell, then recurse into searchCell
  // 2. if searchCell is smaller than this cell then recurse this cell
  // BUT: if sizes are less than min size, or either doesn't have children, then 
  // stop and add the contact
  CellData* maxRadiusCell = searchRadius > objectRadius ? search : object;

  if (maxRadiusCell->cell->numChildren == 0 || (maxRadiusCell->cell->radius*maxRadiusCell->scale) < minRadius)
  {
    // TODO: use averaged normal on dynamics colliding against dynamics
    Direction normal = Direction(search->cell->normal);// - Direction(object->cell->normal);
    normal.normalise();
//    g_debugDraw.drawCell(search->cell, Colour(1,1,1));
    addContact(object->position, search->position, objectRadius, searchRadius, normal);
    return;
  }
  for (int i = 0; i < maxRadiusCell->cell->numChildren; i++)
  {
    CellData child;
    child.cell = maxRadiusCell->cell->children[i];
    child.position = maxRadiusCell->position + maxRadiusCell->rotation.rotateVector(child.cell->position)*maxRadiusCell->scale;
    child.rotation = maxRadiusCell->rotation;
    child.scale = maxRadiusCell->scale;
    if (child.cell->type >= Cell::Orientable)
    {
      OrientableCell* oChild = (OrientableCell*)(child.cell);
      child.rotation *= oChild->rotation;
      child.scale *= oChild->scale;
    }
    findContactCount++; // for debugging
    if (maxRadiusCell == search)
      findContacts(object, &child, minRadius);
    else
      findContacts(&child, search, minRadius);
  }
//   for (CellList* cellDynamics = maxRadiusCell->cell->dynamics; cellDynamics; cellDynamics = cellDynamics->next)
//   {
//     CellData child;
//     child.cell = cellDynamics->child;
//     if (child.cell == this) // don't collide with self, since its a rigid body
//       continue; 
//     child.position = maxRadiusCell->position + maxRadiusCell->rotation.rotateVector(child.cell->position)*maxRadiusCell->scale;
//     child.rotation = maxRadiusCell->rotation;
//     child.scale = maxRadiusCell->scale;
//     if (child.cell->type >= Cell::Orientable)
//     {
//       OrientableCell* oChild = (OrientableCell*)(child.cell);
//       child.rotation *= oChild->rotation;
//       child.scale *= oChild->scale;
//     }
//     findContactCount++; // for debugging
//     if (maxRadiusCell == search)
//       findContacts(object, &child, minRadius);
//     else
//       findContacts(&child, search, minRadius);
//   }
}
