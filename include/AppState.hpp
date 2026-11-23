#pragma once

#include "Camera.hpp"
#include "Frame.hpp"
#include "Light.hpp"
#include "Model.hpp"

#include <string>

// Shared renderer state. Owned by Application, read and edited by ImGuiLayer.
struct AppState {
  int windowWidth{1920};
  int windowHeight{1080};
  bool vsync{false};

  glm::vec3 skybox{0.0f};
  Camera camera;
  Frame frame;
  bool useTexture{true};

  Model model;
  Model lightSphere;     // visual marker for the light position
  Texture metallicMap;   // metallic factor (map_Pm)
  Texture roughnessMap;  // surface roughness (map_Pr)
  Texture normalMap;     // tangent-space normals (map_Bump)

  std::string modelPath{"assets/models/chinese-dragon.obj"};
  std::string texturePath{"assets/textures/Image_0.jpg"};
  std::string metallicMapPath{"assets/textures/Image_1_metallic.jpg"};
  std::string roughnessMapPath{"assets/textures/Image_1_roughness.jpg"};
  std::string normalMapPath{"assets/textures/Image_2.jpg"};
  bool useMetallicMap{true};
  bool useRoughnessMap{true};
  bool useNormalMap{true};
  std::string status;
  bool statusIsError{false};

  Light light;
};
