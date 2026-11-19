#pragma once

#include "Shader.h"

#include <glm/glm.hpp>

// Matches the `LightProperties` struct in basic.frag.
struct Light {
  // World-space direction the light travels. The shader normalizes it and
  // uses its negation as the direction toward the light.
  glm::vec3 direction{-0.5f, -1.0f, -0.35f};
  glm::vec3 color{1.0f};
  float intensity{1.0f};
  float ambientContribution{0.3f};

  void apply(Shader &shader) const {
    shader.setVec3("uLight.direction", direction);
    shader.setVec3("uLight.color", color);
    shader.setFloat("uLight.intensity", intensity);
    shader.setFloat("uLight.ambientContribution", ambientContribution);
  }
};
