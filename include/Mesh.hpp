#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <iostream>

#include "Shader.h"

using namespace std;

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec2 texcoord;
};

class Mesh {
public:
  Mesh();
  Mesh(vector<Vertex> vertices, vector<unsigned int> indices);
  ~Mesh();

  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;
  Mesh(Mesh &&other) noexcept;
  Mesh &operator=(Mesh &&other) noexcept;

  vector<Vertex> vertices_;
  vector<unsigned int> indices_;

  int draw(Shader &shader);
  void setupMesh();

private:
  void release();

  unsigned int vao{0}, vbo{0}, ebo{0};
};
