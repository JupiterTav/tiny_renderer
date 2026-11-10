#include "Model.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

Model::Model(const std::filesystem::path &path) : filePath(path) {
  LoadModel(path);
}

void Model::draw(Shader &shader) {
  for (Mesh &mesh : meshes)
    mesh.draw(shader);
}

void Model::LoadModel(const std::filesystem::path &path) {
  tinyobj::ObjReaderConfig readerConfig;
  readerConfig.triangulate = true;
  readerConfig.mtl_search_path = path.parent_path().string();

  tinyobj::ObjReader reader;
  // ParseFromFile reads .obj and .mtl files and produce attrib, shapes and
  // feedbacks
  if (!reader.ParseFromFile(path.string(), readerConfig)) {
    if (!reader.Error().empty())
      std::cerr << "TinyObjReader::ERROR:: " << reader.Error();
    return;
  }
  if (!reader.Warning().empty())
    std::cout << "TinyObjReader::WARNING:: " << reader.Warning();

  const tinyobj::attrib_t &attrib = reader.GetAttrib();
  const std::vector<tinyobj::shape_t> &shapes = reader.GetShapes();

  boundsMin = glm::vec3(0.0f);
  boundsMax = glm::vec3(0.0f);
  bool haveBounds = false;

  std::size_t totalVertices = 0;
  std::size_t totalTriangles = 0;

  for (const tinyobj::shape_t &shape : shapes) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::map<std::tuple<int, int, int>, unsigned int> uniqueVertices;
    bool computeNormals = false;

    std::size_t indexOffset = 0;

    // Returns the index of the vertex at `corner` of the current face.
    // Vertices are shared when their position/normal/texcoord combination
    // matches, so the same OBJ vertex can be duplicated where needed.
    auto vertexIndex = [&](std::size_t corner) -> unsigned int {
      const tinyobj::index_t idx = shape.mesh.indices[indexOffset + corner];
      // removes the duplication of indices
      const auto key = std::make_tuple(idx.vertex_index, idx.normal_index,
                                       idx.texcoord_index);

      auto found = uniqueVertices.find(key);
      if (found != uniqueVertices.end())
        return found->second;

      // build a new vertex
      Vertex vertex{};

      if (idx.vertex_index >= 0 &&
          3 * static_cast<std::size_t>(idx.vertex_index) + 2 <
              attrib.vertices.size()) {
        const std::size_t base = 3 * static_cast<std::size_t>(idx.vertex_index);
        vertex.position =
            glm::vec3(attrib.vertices[base + 0], attrib.vertices[base + 1],
                      attrib.vertices[base + 2]);
      }

      if (idx.normal_index >= 0 &&
          3 * static_cast<std::size_t>(idx.normal_index) + 2 <
              attrib.normals.size()) {
        const std::size_t base = 3 * static_cast<std::size_t>(idx.normal_index);
        vertex.normal =
            glm::vec3(attrib.normals[base + 0], attrib.normals[base + 1],
                      attrib.normals[base + 2]);
      } else {
        computeNormals = true;
      }

      if (idx.texcoord_index >= 0 &&
          2 * static_cast<std::size_t>(idx.texcoord_index) + 1 <
              attrib.texcoords.size()) {
        const std::size_t base =
            2 * static_cast<std::size_t>(idx.texcoord_index);
        vertex.texcoord =
            glm::vec2(attrib.texcoords[base + 0], attrib.texcoords[base + 1]);
      }

      if (!haveBounds) {
        boundsMin = boundsMax = vertex.position;
        haveBounds = true;
      } else {
        boundsMin = glm::min(boundsMin, vertex.position);
        boundsMax = glm::max(boundsMax, vertex.position);
      }

      const unsigned int newIndex = static_cast<unsigned int>(vertices.size());
      uniqueVertices.emplace(key, newIndex);
      vertices.push_back(vertex);
      return newIndex;
    };

    for (std::size_t face = 0; face < shape.mesh.num_face_vertices.size();
         ++face) {
      const std::size_t faceVertices = shape.mesh.num_face_vertices[face];

      // tinyobjloader triangulates by default. Triangulating here as well
      // keeps polygons working if that option is ever disabled.
      for (std::size_t v = 1; v + 1 < faceVertices; ++v) {
        indices.push_back(vertexIndex(0));
        indices.push_back(vertexIndex(v));
        indices.push_back(vertexIndex(v + 1));
      }

      indexOffset += faceVertices;
    }

    if (vertices.empty() || indices.empty())
      continue;

    if (computeNormals) {
      for (Vertex &vertex : vertices)
        vertex.normal = glm::vec3(0.0f);

      for (std::size_t i = 0; i + 2 < indices.size(); i += 3) {
        Vertex &a = vertices[indices[i + 0]];
        Vertex &b = vertices[indices[i + 1]];
        Vertex &c = vertices[indices[i + 2]];
        const glm::vec3 normal =
            glm::cross(b.position - a.position, c.position - a.position);
        a.normal += normal;
        b.normal += normal;
        c.normal += normal;
      }

      for (Vertex &vertex : vertices) {
        if (glm::dot(vertex.normal, vertex.normal) > 0.0f)
          vertex.normal = glm::normalize(vertex.normal);
      }
    }

    totalVertices += vertices.size();
    totalTriangles += indices.size() / 3;
    meshes.emplace_back(std::move(vertices), std::move(indices));
  }

  std::cout << "Model::Loaded::" << path.string() << " (" << meshes.size()
            << " meshes, " << totalVertices << " vertices, " << totalTriangles
            << " triangles)" << std::endl;
}
