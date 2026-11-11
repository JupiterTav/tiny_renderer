#pragma once

#include "Mesh.hpp"
#include "Texture.hpp"
#include "glad/gl.h"

#include <filesystem>
#include <vector>

// A loaded Wavefront .obj model.
// One Mesh is created per shape and is stored in `meshes`.
class Model {
public:
  explicit Model(const std::filesystem::path &path);

  void draw(Shader &shader);

  std::vector<Mesh> meshes;
  Texture texture;

  // Axis-aligned bounds of all loaded vertices, in model space.
  glm::vec3 boundsMin{0.0f};
  glm::vec3 boundsMax{0.0f};

  glm::vec3 center() const { return (boundsMin + boundsMax) * 0.5f; }
  float radius() const { return glm::length((boundsMax - boundsMin) * 0.5f); }

  bool loadModelTexture(const std::filesystem::path &path);

private:
  std::filesystem::path filePath;

  void LoadModel(const std::filesystem::path &path);
};
