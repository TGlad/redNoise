#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>



#include "Vector3.h"

// TODO: shouldn't be necessary, given templates below
inline float clamped(float value, float left, float right)
{
  return value<left ? left : (value > right ? right : value);
}
inline float random()
{
  return -1.0f + (2.0f * (float)rand() / (float)RAND_MAX);
}
inline float random(float min, float max)
{
  return min + (max-min)*((float)rand() / (float)RAND_MAX);
}
template<class T> inline const T& max(const T& left, const T& right)
{	
	return right>left ? right : left;
}
template<class T> inline const T& min(const T& left, const T& right)
{	
	return right<left ? right : left;
}
template<class T> inline const T& clamped(const T& value, const T& left, const T& right)
{	
  return value<left ? left : (value > right ? right : value);
}
inline float sqr(float a)
{
  return a*a;
}
inline float absf(float a)
{
  int b=(*((int *)(&a)))&0x7FFFFFFF;
  return *((float *)(&b));
}
// Taken from Programming Gems IV
template <typename T> void smoothC1(T &val, T &valRate, const float timeDelta, const T &to, const float smoothTime)
{
  if (smoothTime > 0.0f)
  {
    float omega = 2.0f / smoothTime;
    float x = omega * timeDelta;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    T change = val - to;
    T temp = (valRate + change * omega) * timeDelta;
    valRate = (valRate - temp * omega) * exp;
    val = to + (change + temp) * exp;
  }
  else if (timeDelta > 0.0f)
  {
    valRate = (to - val) / timeDelta;
    val = to;
  }
  else
  {
    val = to;
    valRate -= valRate; // zero it...
  }
}

template <typename T>
inline void smoothC0(T& val, const float timeDelta, const T& to, const float smoothTime)
{
  float lambda = timeDelta / smoothTime;
  val = to + (val - to) / (1.0f + lambda + 0.5f * lambda * lambda);
}

// Set of typedefs to clarify the dimensions being used
typedef float Length;             // can't be negative as a length is a magnitude
typedef float Distance;           // can be negative
typedef float Speed;              // distance per time, can be negative
typedef float AccelerationScalar; // distance per time squared
typedef float Angle;              // radians, unlimited, can be negative
typedef float Strength;           // per time squared. This is s in acc = s*error. The gain of a hookean spring
typedef float Damping;            // per time. This is d in acc = -d*vel. It is linear drag
typedef float Stiffness;          // per time. also called natural frequency, as undamped motion oscillates at this many radians per second. is omega in acc=omega^2*error. 10 approx human stiffness
typedef float DampingRatio;       // unitless ratio, 1 = critical damping (fastest convergence with no oscillation). Is dr in acc=omega^2*error - omega*dr*vel
typedef float AngularSpeed;       // radians per time, can be negative
typedef float Frequency;          // revolutions/repeats per time
typedef float Scale;              // -inf to inf range scaling value. 1 is usually the default (no effect) value
typedef float Weight;             // usually 0 to 1 range, sometimes can be > 1
typedef float TimePeriod;         // can be negative
typedef float Imminence;          // 1/time to event, can be negative
typedef float Mass;               
typedef float Area;               // distance^2, can't be negative
typedef float ImpulseScalar;      // mass distance per time (force * time)
typedef float TorqueImpulseScalar;// mass distance per time (force * time)
typedef float Density;            // mass per distance cubed
typedef float Proximity;          // 1/distance
typedef float InertiaScalar;      // moment of inertia, in mass*area
typedef float InverseMass;        // 1/mass
                                                                    
typedef Vector3 Position;             
typedef Vector3 PositionDelta;        // a local difference in position
typedef Vector3 Velocity;             // distance per time
typedef Vector3 Acceleration;         // distance per time squared
typedef Vector3 Jerk;                 // distance per time cubed
typedef Vector3 RotationVector;       // direction = rotation axis, magnitude = radians anti-clockwise (used when you're interested in the process of rotation, not necessarily the resulting orientation)
typedef Vector3 AngularVelocity;      // radians per time (anti-clockwise looking along direction)
typedef Vector3 AngularAcceleration;  // radians per time squared (anti-clockwise looking along direction)
typedef Vector3 Momentum;             // mass distance per time
typedef Vector3 Impulse;              // mass distance per time (force * time)
typedef Vector3 Force;                // mass distance per time squared (or newtons)
typedef Vector3 AngularMomentum;      // mass distance squared radians per time
typedef Vector3 TorqueImpulse;        // mass distance squared radians per time
typedef Vector3 Torque;               // mass distance squared radians per time squared
typedef Vector3 Extents;              // distance. Radius (half width) extents, usually of an aabb.
typedef Vector3 Direction;            // always unit length, so unitless
typedef Vector3 InertiaVector;        // moment of inertia around the three primary axes, mass*area 
typedef Vector3 Colour;               // 0 to 1 in x,y,z representing r,g,b

#include "Matrix33.h"

typedef Matrix33 Rotation;            // unitless, orthonormal
