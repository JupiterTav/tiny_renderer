#pragma once

#include "Mesh.hpp"
#include "Texture.hpp"
#include "glad/gl.h"

#include <filesystem>
#include <string>
#include <vector>

// A loaded Wavefront .obj model.
// One Mesh is created per shape and is stored in `meshes`.
class Model {
public:
  Model() = default;
  explicit Model(const std::filesystem::path &path);

  // Reloads geometry from `path`. The existing texture is kept. Returns false
  // (leaving the current geometry untouched) when the file cannot be loaded.
  bool load(const std::filesystem::path &path);

  // Draws the model. Material uniforms and any .mtl map_Kd texture are
  // applied per mesh; `texture` is used as a fallback when a material has
  // no texture of its own.
  void draw(Shader &shader, bool useTexture = true);

  std::vector<Mesh> meshes;
  std::vector<Material> materials;
  Texture texture;

  // Axis-aligned bounds of all loaded vertices, in model space.
  glm::vec3 boundsMin{0.0f};
  glm::vec3 boundsMax{0.0f};

  glm::vec3 center() const { return (boundsMin + boundsMax) * 0.5f; }
  float radius() const { return glm::length((boundsMax - boundsMin) * 0.5f); }

  bool loadModelTexture(const std::filesystem::path &path);

private:
  std::filesystem::path filePath;
  // Indexed by material index; invalid entries mean the material has no map_Kd.
  std::vector<Texture> materialTextures;

  bool LoadModel(const std::filesystem::path &path,
                 std::vector<Mesh> &outMeshes,
                 std::vector<Material> &outMaterials,
                 std::vector<Texture> &outMaterialTextures, glm::vec3 &outMin,
                 glm::vec3 &outMax);

  static std::filesystem::path resolveTexturePath(
      const std::filesystem::path &objPath, const std::string &textureName);
};
