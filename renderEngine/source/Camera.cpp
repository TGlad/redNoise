#include "Camera.h"

Camera::Camera(View* view)
{
  this->view = view;
  smoothTime = 0.5f;
  // extract the target pitch/yaw/roll from the initial view direction?
  targetDirection = view->rotation.forwards();
  targetPos = view->position;
  velocity.setToZero();
  angularVelocity.setToZero();
}
// how do you rotate the camera when the orientation can change?
// this amounts to considering how you would orient it on a tipping boat...
// we can make it easier by considering just yaw and pitch, which is just a target direction... much easier.
// from that we can extract the yaw and pitch easy enough.
void Camera::update(TimePeriod timeStep)
{
  smoothC1(view->position, velocity, timeStep, targetPos, smoothTime);

  RotationVector angleDelta = Vector3::getRotationVector(view->rotation.forwards(), targetDirection);
  RotationVector angle(0,0,0);
  smoothC1(angle, angularVelocity, timeStep, angleDelta, smoothTime);
  Direction dir = Rotation(angle).rotateVector(view->rotation.forwards());
  view->rotation.fromForwardAlignedByUp(dir, Vector3(0,1,0)); // this will pop when going to new up angle.. oh well

  // TODO: make this somehow wrapped up, since it is duplicating dynamic cell a bit. Use multiple arguments to function?
  Rotation oldRotation = view->rotation;
  Position oldPosition = view->position;
  Scale oldScale = view->scale;
  Cell* oldParent = view->parent;
  view->update();
  if (view->parent != oldParent)
  {
    // update target position
    targetPos -= oldPosition;
    targetPos.inverseRotate(oldRotation); // into world space
    targetPos.rotate(view->rotation); // back into parent space
    targetPos *= view->scale / oldScale;
    targetPos += view->position;

    velocity.inverseRotate(oldRotation); // into world space
    velocity.rotate(view->rotation); // back into parent space
    velocity *= view->scale / oldScale;

    angularVelocity.inverseRotate(oldRotation); // into world space
    angularVelocity.rotate(view->rotation); // back into parent space

    targetDirection.inverseRotate(oldRotation); // into world space
    targetDirection.rotate(view->rotation); // back into parent space
  }
}
