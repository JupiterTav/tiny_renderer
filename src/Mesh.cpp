#include "Mesh.hpp"

#include <utility>

Mesh::Mesh(vector<Vertex> vertices, vector<unsigned int> indices)
    : vertices_(std::move(vertices)), indices_(std::move(indices)) {
  setupMesh();
}

Mesh::Mesh() = default;

Mesh::~Mesh() { release(); }

Mesh::Mesh(Mesh &&other) noexcept
    : vertices_(std::move(other.vertices_)),
      indices_(std::move(other.indices_)), vao(std::exchange(other.vao, 0)),
      vbo(std::exchange(other.vbo, 0)), ebo(std::exchange(other.ebo, 0)) {}

Mesh &Mesh::operator=(Mesh &&other) noexcept {
  if (this != &other) {
    release();
    vertices_ = std::move(other.vertices_);
    indices_ = std::move(other.indices_);
    vao = std::exchange(other.vao, 0);
    vbo = std::exchange(other.vbo, 0);
    ebo = std::exchange(other.ebo, 0);
  }
  return *this;
}

int Mesh::draw(Shader &shader) {
  // The shader is expected to already be bound and configured by the caller.
  (void)shader;

  if (vao == 0 || indices_.empty())
    return 0;

  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()),
                 GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
  return 0;
}

void Mesh::setupMesh() {
  if (vertices_.empty() || indices_.empty())
    return;

  // Make setupMesh safe to call again on the same instance.
  release();

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

  glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(Vertex),
               vertices_.data(), GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int),
               indices_.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);

  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, normal));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, texcoord));

  glBindVertexArray(0);
}

void Mesh::release() {
  if (ebo != 0) {
    glDeleteBuffers(1, &ebo);
    ebo = 0;
  }
  if (vbo != 0) {
    glDeleteBuffers(1, &vbo);
    vbo = 0;
  }
  if (vao != 0) {
    glDeleteVertexArrays(1, &vao);
    vao = 0;
  }
}
