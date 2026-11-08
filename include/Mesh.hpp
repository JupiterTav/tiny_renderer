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
  Mesh(vector<Vertex> vertices, vector<unsigned int> indices);

  vector<Vertex> vertices_;
  vector<unsigned int> indices_;

  int draw(Shader &shader);

private:
  unsigned int vao, vbo, ebo;

  int setupMesh();
};
