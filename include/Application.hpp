#pragma once

#include "AppState.hpp"
#include "ImGuiLayer.hpp"
#include "Shader.h"

#include <filesystem>

struct GLFWwindow;

// Owns the window, renderer state, UI and the asset-loading operations.
class Application {
public:
  explicit Application(const char *executablePath = nullptr);
  ~Application();

  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;

  int run();

private:
  bool initialize();
  void shutdown();
  void updateCamera();
  void renderScene(int width, int height);

  void loadModel(const std::filesystem::path &path);
  void loadTexture(const std::filesystem::path &path);
  void loadMetallicMap(const std::filesystem::path &path);
  void loadRoughnessMap(const std::filesystem::path &path);
  void loadNormalMap(const std::filesystem::path &path);
  void requestQuit();

  // Relative asset paths are resolved against the executable directory so the
  // app works regardless of the process working directory on Windows.
  std::filesystem::path resolveAssetPath(const std::filesystem::path &path) const;

  static void glfwErrorCallback(int error, const char *description);

  GLFWwindow *window_{nullptr};
  bool glfwInitialized_{false};
  std::filesystem::path executableDir_;

  AppState state_;
  ImGuiLayer ui_{state_};
  Shader shader_;

  double lastMouseX_{0.0};
  double lastMouseY_{0.0};
  bool hasLastMouse_{false};

  bool initialized_{false};
};
