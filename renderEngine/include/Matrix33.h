#pragma once
#include "basics.h"

class Matrix33
{
public:
  Vector3 row[3];
  Matrix33(){}
  Matrix33(const RotationVector& rotationVector);
  inline Vector3& forwards(){ return row[2]; }
  inline Vector3& right(){ return row[0]; }
  inline Vector3& up(){ return row[1]; }


  void setIdentity();
  void fromForwardAlignedByUp(const Vector3& forwards, const Vector3& up);
  Matrix33 operator*(const Matrix33& m) const;
  Matrix33 operator ~() const;
  void operator*=(const Matrix33& m);
  Matrix33 transposed() const;
  bool invert();
  Vector3 rotateVector(const Vector3& pos) const;
  Vector3 inverseRotateVector(const Vector3& pos) const;
  void scale(Scale scale);
};

