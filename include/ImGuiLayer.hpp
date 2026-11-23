#pragma once

#include "AppState.hpp"

#include <filesystem>
#include <functional>
#include <utility>

struct GLFWwindow;

// Owns the Dear ImGui context and draws the application panel. Loading is
// reported through callbacks so this class does not depend on Application.
class ImGuiLayer {
public:
  using LoadCallback = std::function<void(const std::filesystem::path &)>;

  explicit ImGuiLayer(AppState &state);
  ~ImGuiLayer();

  ImGuiLayer(const ImGuiLayer &) = delete;
  ImGuiLayer &operator=(const ImGuiLayer &) = delete;

  bool init(GLFWwindow *window);
  void shutdown();

  void beginFrame();
  void draw();
  void endFrame();

  void setModelLoader(LoadCallback callback) {
    modelLoader_ = std::move(callback);
  }
  void setTextureLoader(LoadCallback callback) {
    textureLoader_ = std::move(callback);
  }
  void setMetallicLoader(LoadCallback callback) {
    metallicLoader_ = std::move(callback);
  }
  void setRoughnessLoader(LoadCallback callback) {
    roughnessLoader_ = std::move(callback);
  }
  void setNormalLoader(LoadCallback callback) {
    normalLoader_ = std::move(callback);
  }

  // Camera input needs to ignore events consumed by ImGui widgets.
  bool wantsCaptureMouse() const;
  float mouseWheel() const;

private:
  void drawFileDialogs();

  AppState &state_;
  LoadCallback modelLoader_;
  LoadCallback textureLoader_;
  LoadCallback metallicLoader_;
  LoadCallback roughnessLoader_;
  LoadCallback normalLoader_;
  bool initialized_{false};
};
