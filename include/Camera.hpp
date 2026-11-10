#pragma once

#include <glm/glm.hpp>

// Simple orbit camera: it always looks at `target` and is controlled with
// mouse drag (orbit) and the scroll wheel (zoom).
class Camera {
public:
  Camera() = default;
  explicit Camera(const glm::vec3 &target, float distance);

  void orbit(float deltaX, float deltaY);
  void zoom(float amount);

  void setTarget(const glm::vec3 &target) { target_ = target; }
  const glm::vec3 &target() const { return target_; }

  void setDistance(float distance);
  float distance() const { return distance_; }

  glm::vec3 position() const;
  glm::mat4 view() const;
  glm::mat4 projection(float aspect) const;

private:
  glm::vec3 target_{0.0f};
  float distance_{5.0f};
  float yaw_{0.78539816f};  // radians
  float pitch_{0.43633231f}; // radians
  float fov_{0.78539816f};   // radians
};
