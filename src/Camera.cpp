#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

Camera::Camera(const glm::vec3 &target, float distance)
    : target_(target), distance_(distance) {
  setDistance(distance);
}

void Camera::orbit(float deltaX, float deltaY) {
  yaw_ -= deltaX * 0.005f;
  pitch_ += deltaY * 0.005f;
  pitch_ = std::clamp(pitch_, -1.55334303f, 1.55334303f); // +/- 89 degrees
}

void Camera::zoom(float amount) {
  distance_ *= std::exp(-amount * 0.15f);
  setDistance(distance_);
}

void Camera::setDistance(float distance) {
  distance_ = std::clamp(distance, 0.01f, 100000.0f);
}

glm::vec3 Camera::position() const {
  const float cosPitch = std::cos(pitch_);
  const glm::vec3 offset(cosPitch * std::cos(yaw_), std::sin(pitch_),
                         cosPitch * std::sin(yaw_));
  return target_ + offset * distance_;
}

glm::mat4 Camera::view() const {
  return glm::lookAt(position(), target_, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::projection(float aspect) const {
  if (aspect <= 0.0f)
    aspect = 1.0f;

  const float nearPlane = std::max(0.01f, distance_ * 0.01f);
  const float farPlane = std::max(100.0f, distance_ * 10.0f);
  return glm::perspective(fov_, aspect, nearPlane, farPlane);
}
