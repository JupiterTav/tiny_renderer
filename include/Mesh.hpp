#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <iostream>
#include <string>

#include "Shader.h"

using namespace std;

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texcoord;
};

// Material properties from an OBJ's .mtl file. When no MTL is present the
// default values are used.
struct Material {
  string name;
  int index{-1};
  glm::vec3 ambientColor{0.0f};
  glm::vec3 diffuseColor{0.78f, 0.52f, 0.32f};
  glm::vec3 specularColor{0.25f};
  float shininess{64.0f};
};

class Mesh {
public:
  Mesh();
  Mesh(vector<Vertex> vertices, vector<unsigned int> indices,
       Material material = {});
  ~Mesh();

  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;
  Mesh(Mesh &&other) noexcept;
  Mesh &operator=(Mesh &&other) noexcept;

  vector<Vertex> vertices_;
  vector<unsigned int> indices_;
  Material material;

  int draw(Shader &shader);
  void setupMesh();

private:
  void release();

  unsigned int vao{0}, vbo{0}, ebo{0};
};
