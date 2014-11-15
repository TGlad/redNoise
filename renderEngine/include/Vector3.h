#pragma once
#include "basics.h"

class Vector3
{
public:
  Vector3(){}  
  inline Vector3(float X, float Y, float Z)
  {
    x = X; y = Y; z = Z;
  }
  inline Vector3(char norm[]) 
  {
    x = ((float)norm[0]) / 127.0f;
    y = ((float)norm[1]) / 127.0f;
    z = ((float)norm[2]) / 127.0f;
  }
  float x, y, z;
  inline char red() const
  {
    return (char)(x * 255.0f);
  }
  inline char green() const
  {
    return (char)(y * 255.0f);
  }
  inline char blue() const
  {
    return (char)(z * 255.0f);
  }
  inline Vector3 operator *(float f) const
  {
    return Vector3(x * f, y * f, z * f);
  }
  inline Vector3 operator *(const Vector3& vec) const
  {
    return Vector3(x * vec.x, y * vec.y, z * vec.z);
  }
  inline void operator *=(float f)
  {
    x *= f;
    y *= f;
    z *= f;
  }
  inline Vector3 operator /(float f) const
  {
    float div = 1.0f/f;
    return Vector3(x * div, y * div, z * div);
  }
  inline Vector3 operator /(const Vector3& div) const
  {
    return Vector3(x / div.x, y / div.y, z / div.z);
  }
  inline void operator /=(float f) 
  {
    float div = 1.0f/f;
    x *= div;
    y *= div;
    z *= div;
  }
  inline Vector3 operator +(const Vector3& v) const
  {
    return Vector3(x + v.x, y + v.y, z + v.z);
  }
  inline void operator +=(const Vector3& v)
  {
    x += v.x;
    y += v.y;
    z += v.z;
  }
  inline Vector3 operator -() const
  {
    return Vector3(-x, -y, -z);
  }
  inline Vector3 operator -(const Vector3& v) const
  {
    return Vector3(x - v.x, y - v.y, z - v.z);
  }
  inline void operator -=(const Vector3& v)
  {
    x -= v.x;
    y -= v.y;
    z -= v.z;
  }
  inline void set(const Vector3& v)
  {
    x = v.x;
    y = v.y;
    z = v.z;
  }
  inline void set(float _x, float _y, float _z)
  {
    x = _x;
    y = _y;
    z = _z;
  }
  inline void setToZero()
  {
    x = y = z = 0.0f;
  }
  inline float magnitudeSquared() const
  {
    return x*x + y*y + z*z;
  }
  inline float magnitude() const
  {
    return sqrtf(magnitudeSquared());
  }
  inline float normalise()
  {
    float f = sqrtf(magnitudeSquared());
    if (f < 1e-10f)
    {
      set(0,0,1);
      return 0.0f;
    }
    float d = 1.0f / f;
    x *= d; y *= d; z *= d;
    return f;
  }
  static inline Vector3 normalise(const Vector3& v)
  {
    float f = v.magnitudeSquared();
    return f < 1e-10f ? Vector3(0,0,1) : v / sqrtf(f);
  }
  inline float dot(const Vector3& v) const
  {
    return x * v.x + y * v.y + z * v.z;
  }
  static inline Vector3 cross(const Vector3& a, const Vector3& b) 
  {
    return Vector3((a.y * b.z) - (a.z * b.y),
                   (a.z * b.x) - (a.x * b.z),
                   (a.x * b.y) - (a.y * b.x));
  }
  static Vector3 getRotationVector(const Vector3& from, const Vector3& to);

  void rotate(const class Matrix33& mat);
  void inverseRotate(const class Matrix33& mat);
  void clamp(float minVal, float maxVal);
};
Vector3 operator *(float f, const Vector3& v);
