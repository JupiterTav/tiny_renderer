#include "Model.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

Model::Model(const std::filesystem::path &path) { load(path); }

bool Model::load(const std::filesystem::path &path) {
  // Build everything separately so a failed load keeps the current model.
  std::vector<Mesh> newMeshes;
  std::vector<Material> newMaterials;
  std::vector<Texture> newMaterialTextures;
  glm::vec3 newMin(0.0f);
  glm::vec3 newMax(0.0f);

  if (!LoadModel(path, newMeshes, newMaterials, newMaterialTextures, newMin,
                 newMax))
    return false;

  meshes = std::move(newMeshes);
  materials = std::move(newMaterials);
  materialTextures = std::move(newMaterialTextures);
  boundsMin = newMin;
  boundsMax = newMax;
  filePath = path;
  return true;
}

void Model::draw(Shader &shader, bool useTexture) {
  for (Mesh &mesh : meshes) {
    shader.setVec3("uAmbientColor", mesh.material.ambientColor);
    shader.setVec3("uDiffuseColor", mesh.material.diffuseColor);
    shader.setVec3("uSpecularColor", mesh.material.specularColor);
    shader.setFloat("uShininess", mesh.material.shininess);

    const Texture *materialTexture = nullptr;
    if (useTexture && mesh.material.index >= 0 &&
        static_cast<std::size_t>(mesh.material.index) <
            materialTextures.size()) {
      const Texture &candidate =
          materialTextures[static_cast<std::size_t>(mesh.material.index)];
      if (candidate.valid())
        materialTexture = &candidate;
    }
    if (materialTexture == nullptr && useTexture && texture.valid())
      materialTexture = &texture;

    shader.setBool("uUseTexture", materialTexture != nullptr);
    if (materialTexture != nullptr)
      materialTexture->bind(0);
    else
      texture.unbind();

    mesh.draw(shader);
  }
}

std::filesystem::path
Model::resolveTexturePath(const std::filesystem::path &objPath,
                          const std::string &textureName) {
  const std::filesystem::path name(textureName);
  if (name.is_absolute())
    return name;

  const std::filesystem::path objDir = objPath.parent_path();
  const std::filesystem::path candidates[] = {
      objDir / name,
      objDir.parent_path() / "textures" / name,
      name,
  };

  for (const std::filesystem::path &candidate : candidates) {
    std::error_code error;
    if (std::filesystem::exists(candidate, error))
      return candidate;
  }
  return candidates[0];
}

bool Model::LoadModel(const std::filesystem::path &path,
                      std::vector<Mesh> &outMeshes,
                      std::vector<Material> &outMaterials,
                      std::vector<Texture> &outMaterialTextures,
                      glm::vec3 &outMin, glm::vec3 &outMax) {
  tinyobj::ObjReaderConfig readerConfig;
  readerConfig.triangulate = true;
  readerConfig.mtl_search_path = path.parent_path().string();

  tinyobj::ObjReader reader;
  // ParseFromFile reads .obj and .mtl files and produce attrib, shapes and
  // feedbacks
  if (!reader.ParseFromFile(path.string(), readerConfig)) {
    if (!reader.Error().empty())
      std::cerr << "TinyObjReader::ERROR:: " << reader.Error();
    return false;
  }
  if (!reader.Warning().empty())
    std::cout << "TinyObjReader::WARNING:: " << reader.Warning();

  const tinyobj::attrib_t &attrib = reader.GetAttrib();
  const std::vector<tinyobj::shape_t> &shapes = reader.GetShapes();
  const std::vector<tinyobj::material_t> &fileMaterials = reader.GetMaterials();

  outMaterials.clear();
  outMaterials.reserve(fileMaterials.size());
  outMaterialTextures.clear();
  outMaterialTextures.resize(fileMaterials.size());

  for (std::size_t i = 0; i < fileMaterials.size(); ++i) {
    const tinyobj::material_t &fileMaterial = fileMaterials[i];

    Material material;
    material.name = fileMaterial.name;
    std::cout << "MODEL_LOADING:MESH::MATERIAL: " << material.name << std::endl;
    material.index = static_cast<int>(i);
    material.ambientColor =
        glm::vec3(fileMaterial.ambient[0], fileMaterial.ambient[1],
                  fileMaterial.ambient[2]);
    material.diffuseColor =
        glm::vec3(fileMaterial.diffuse[0], fileMaterial.diffuse[1],
                  fileMaterial.diffuse[2]);
    material.specularColor =
        glm::vec3(fileMaterial.specular[0], fileMaterial.specular[1],
                  fileMaterial.specular[2]);
    material.shininess =
        std::clamp(static_cast<float>(fileMaterial.shininess), 1.0f, 256.0f);
    outMaterials.push_back(std::move(material));

    if (fileMaterial.diffuse_texname.empty())
      continue;

    const std::filesystem::path texturePath =
        resolveTexturePath(path, fileMaterial.diffuse_texname);
    std::error_code error;
    if (!std::filesystem::exists(texturePath, error)) {
      std::cerr << "Model::Warning::map_Kd texture not found: "
                << texturePath.string() << std::endl;
      continue;
    }
    outMaterialTextures[i].load(texturePath);
  }

  outMin = glm::vec3(0.0f);
  outMax = glm::vec3(0.0f);
  bool haveBounds = false;

  std::size_t totalVertices = 0;
  std::size_t totalTriangles = 0;

  for (const tinyobj::shape_t &shape : shapes) {
    // A single shape can contain several materials, so split its faces into one
    // group per material id.
    struct GeometryData {
      std::vector<Vertex> vertices;
      std::vector<unsigned int> indices;
      std::map<std::tuple<int, int, int>, unsigned int> uniqueVertices;
      bool computeNormals{false};
    };
    std::map<int, GeometryData> groups;

    std::size_t indexOffset = 0;

    for (std::size_t face = 0; face < shape.mesh.num_face_vertices.size();
         ++face) {
      const std::size_t faceVertices = shape.mesh.num_face_vertices[face];
      const int materialId = face < shape.mesh.material_ids.size()
                                 ? shape.mesh.material_ids[face]
                                 : -1;
      GeometryData &group = groups[materialId];

      // Returns the index of the vertex at `corner` of the current face.
      // Vertices are shared when their position/normal/texcoord combination
      // matches, so the same OBJ vertex can be duplicated where needed.
      auto vertexIndex = [&](std::size_t corner) -> unsigned int {
        const tinyobj::index_t idx = shape.mesh.indices[indexOffset + corner];
        const auto key = std::make_tuple(idx.vertex_index, idx.normal_index,
                                         idx.texcoord_index);

        auto found = group.uniqueVertices.find(key);
        if (found != group.uniqueVertices.end())
          return found->second;

        Vertex vertex{};

        if (idx.vertex_index >= 0 &&
            3 * static_cast<std::size_t>(idx.vertex_index) + 2 <
                attrib.vertices.size()) {
          const std::size_t base =
              3 * static_cast<std::size_t>(idx.vertex_index);
          vertex.position =
              glm::vec3(attrib.vertices[base + 0], attrib.vertices[base + 1],
                        attrib.vertices[base + 2]);
        }

        if (idx.normal_index >= 0 &&
            3 * static_cast<std::size_t>(idx.normal_index) + 2 <
                attrib.normals.size()) {
          const std::size_t base =
              3 * static_cast<std::size_t>(idx.normal_index);
          vertex.normal =
              glm::vec3(attrib.normals[base + 0], attrib.normals[base + 1],
                        attrib.normals[base + 2]);
        } else {
          group.computeNormals = true;
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
          outMin = outMax = vertex.position;
          haveBounds = true;
        } else {
          outMin = glm::min(outMin, vertex.position);
          outMax = glm::max(outMax, vertex.position);
        }

        const unsigned int newIndex =
            static_cast<unsigned int>(group.vertices.size());
        group.uniqueVertices.emplace(key, newIndex);
        group.vertices.push_back(vertex);
        return newIndex;
      };

      // tinyobjloader triangulates by default. Triangulating here as well
      // keeps polygons working if that option is ever disabled.
      for (std::size_t v = 1; v + 1 < faceVertices; ++v) {
        group.indices.push_back(vertexIndex(0));
        group.indices.push_back(vertexIndex(v));
        group.indices.push_back(vertexIndex(v + 1));
      }

      indexOffset += faceVertices;
    }

    for (auto &[materialId, group] : groups) {
      if (group.vertices.empty() || group.indices.empty())
        continue;

      if (group.computeNormals) {
        for (Vertex &vertex : group.vertices)
          vertex.normal = glm::vec3(0.0f);

        for (std::size_t i = 0; i + 2 < group.indices.size(); i += 3) {
          Vertex &a = group.vertices[group.indices[i + 0]];
          Vertex &b = group.vertices[group.indices[i + 1]];
          Vertex &c = group.vertices[group.indices[i + 2]];
          const glm::vec3 normal =
              glm::cross(b.position - a.position, c.position - a.position);
          a.normal += normal;
          b.normal += normal;
          c.normal += normal;
        }

        for (Vertex &vertex : group.vertices) {
          if (glm::dot(vertex.normal, vertex.normal) > 0.0f)
            vertex.normal = glm::normalize(vertex.normal);
        }
      }

      Material material;
      if (materialId >= 0 &&
          static_cast<std::size_t>(materialId) < outMaterials.size())
        material = outMaterials[static_cast<std::size_t>(materialId)];

      totalVertices += group.vertices.size();
      totalTriangles += group.indices.size() / 3;
      outMeshes.emplace_back(std::move(group.vertices),
                             std::move(group.indices), std::move(material));
    }
  }

  std::cout << "Model::Loaded::" << path.string() << " (" << outMeshes.size()
            << " meshes, " << outMaterials.size() << " materials, "
            << totalVertices << " vertices, " << totalTriangles << " triangles)"
            << std::endl;
  return !outMeshes.empty();
}

bool Model::loadModelTexture(const std::filesystem::path &path) {
  return texture.load(path);
}
