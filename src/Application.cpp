#include "Application.hpp"

#include <glad/gl.h>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <filesystem>
#include <string>
#include <utility>

namespace {
std::filesystem::path executableDirectory(const char *executablePath) {
#ifdef __linux__
  std::error_code error;
  const std::filesystem::path self =
      std::filesystem::read_symlink("/proc/self/exe", error);
  if (!error)
    return self.parent_path();
#endif

  if (!executablePath || !*executablePath)
    return {};

  std::error_code absoluteError;
  const std::filesystem::path path =
      std::filesystem::absolute(executablePath, absoluteError);
  return absoluteError ? std::filesystem::path{} : path.parent_path();
}
} // namespace

void Application::glfwErrorCallback(int error, const char *description) {
  std::fprintf(stderr, "GLFW::Error::%d: %s\n", error, description);
}

Application::Application(const char *executablePath)
    : executableDir_(executableDirectory(executablePath)) {}

Application::~Application() { shutdown(); }

bool Application::initialize() {
  glfwSetErrorCallback(glfwErrorCallback);
  if (glfwInit() == GLFW_FALSE)
    return false;
  glfwInitialized_ = true;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window_ = glfwCreateWindow(state_.windowWidth, state_.windowHeight,
                             "Tiny Renderer", nullptr, nullptr);
  if (!window_)
    return false;

  glfwMakeContextCurrent(window_);
  glfwSwapInterval(state_.vsync ? 1 : 0);

  if (gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) ==
      GLFW_FALSE) {
    std::fprintf(stderr, "Failed to load OpenGL functions\n");
    return false;
  }

  if (!ui_.init(window_)) {
    std::fprintf(stderr, "Failed to initialize ImGui\n");
    return false;
  }

  if (!shader_.load(resolveAssetPath("assets/shaders/basic.vert"),
                    resolveAssetPath("assets/shaders/basic.frag"))) {
    std::fprintf(stderr, "Failed to load model shader\n");
    return false;
  }

  loadModel(state_.modelPath);
  if (state_.model.meshes.empty()) {
    std::fprintf(stderr, "Failed to load model\n");
    return false;
  }

  loadTexture(state_.texturePath);
  loadMetallicMap(state_.metallicMapPath);
  loadRoughnessMap(state_.roughnessMapPath);
  loadNormalMap(state_.normalMapPath);

  if (!state_.lightSphere.load(resolveAssetPath("assets/models/sphere.obj")))
    std::fprintf(stderr, "Failed to load light marker sphere\n");

  ui_.setModelLoader(
      [this](const std::filesystem::path &path) { loadModel(path); });
  ui_.setTextureLoader(
      [this](const std::filesystem::path &path) { loadTexture(path); });
  ui_.setMetallicLoader(
      [this](const std::filesystem::path &path) { loadMetallicMap(path); });
  ui_.setRoughnessLoader(
      [this](const std::filesystem::path &path) { loadRoughnessMap(path); });
  ui_.setNormalLoader(
      [this](const std::filesystem::path &path) { loadNormalMap(path); });
  ui_.setQuitCallback([this] { requestQuit(); });

  glEnable(GL_DEPTH_TEST);
  initialized_ = true;
  return true;
}

void Application::shutdown() {
  shader_.destroy();
  ui_.shutdown();

  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }
  if (glfwInitialized_) {
    glfwTerminate();
    glfwInitialized_ = false;
  }
  initialized_ = false;
}

void Application::requestQuit() {
  if (window_)
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

std::filesystem::path
Application::resolveAssetPath(const std::filesystem::path &path) const {
  if (path.is_absolute() || executableDir_.empty())
    return path;

  // Packaged builds keep assets next to the executable. Development builds may
  // still run with the repository root as the working directory.
  const std::filesystem::path executableRelative = executableDir_ / path;
  std::error_code error;
  if (std::filesystem::exists(executableRelative, error))
    return executableRelative;
  return path;
}

void Application::loadModel(const std::filesystem::path &path) {
  const std::filesystem::path resolved = resolveAssetPath(path);
  if (!state_.model.load(resolved)) {
    state_.status = "Failed to load model: " + resolved.string();
    state_.statusIsError = true;
    return;
  }

  state_.modelPath = resolved.string();
  state_.camera = Camera(state_.model.center(), state_.model.radius() * 2.5f);
  state_.status = "Loaded model: " + resolved.string();
  state_.statusIsError = false;
}

void Application::loadTexture(const std::filesystem::path &path) {
  const std::filesystem::path resolved = resolveAssetPath(path);
  if (!state_.model.loadModelTexture(resolved)) {
    state_.status = "Failed to load texture: " + resolved.string();
    state_.statusIsError = true;
    return;
  }

  state_.texturePath = resolved.string();
  state_.useTexture = true;
  state_.status = "Loaded texture: " + resolved.string();
  state_.statusIsError = false;
}

// Data maps must stay linear; sRGB decoding would distort their values.
static bool loadDataMap(Texture &target, const std::filesystem::path &path,
                        std::string &status, bool &statusIsError,
                        const char *label) {
  Texture newMap;
  if (!newMap.load(path, false)) {
    status = std::string("Failed to load ") + label + ": " + path.string();
    statusIsError = true;
    return false;
  }

  target = std::move(newMap);
  status = std::string("Loaded ") + label + ": " + path.string();
  statusIsError = false;
  return true;
}

void Application::loadMetallicMap(const std::filesystem::path &path) {
  const std::filesystem::path resolved = resolveAssetPath(path);
  if (loadDataMap(state_.metallicMap, resolved, state_.status,
                  state_.statusIsError, "metallic map")) {
    state_.metallicMapPath = resolved.string();
    state_.useMetallicMap = true;
  }
}

void Application::loadRoughnessMap(const std::filesystem::path &path) {
  const std::filesystem::path resolved = resolveAssetPath(path);
  if (loadDataMap(state_.roughnessMap, resolved, state_.status,
                  state_.statusIsError, "roughness map")) {
    state_.roughnessMapPath = resolved.string();
    state_.useRoughnessMap = true;
  }
}

void Application::loadNormalMap(const std::filesystem::path &path) {
  const std::filesystem::path resolved = resolveAssetPath(path);
  if (loadDataMap(state_.normalMap, resolved, state_.status,
                  state_.statusIsError, "normal map")) {
    state_.normalMapPath = resolved.string();
    state_.useNormalMap = true;
  }
}

void Application::updateCamera() {
  double mouseX = 0.0;
  double mouseY = 0.0;
  glfwGetCursorPos(window_, &mouseX, &mouseY);

  if (!hasLastMouse_) {
    lastMouseX_ = mouseX;
    lastMouseY_ = mouseY;
    hasLastMouse_ = true;
  }

  if (!ui_.wantsCaptureMouse()) {
    if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
      state_.camera.orbit(-static_cast<float>(mouseX - lastMouseX_),
                          static_cast<float>(mouseY - lastMouseY_));
    }

    const float wheel = ui_.mouseWheel();
    if (wheel != 0.0f)
      state_.camera.zoom(wheel);
  }

  lastMouseX_ = mouseX;
  lastMouseY_ = mouseY;
}

void Application::renderScene(int width, int height) {
  const float aspect = static_cast<float>(width) / height;

  shader_.use();
  shader_.setMat4("uModel", glm::mat4(1.0f));
  shader_.setMat4("uView", state_.camera.view());
  shader_.setMat4("uProjection", state_.camera.projection(aspect));
  shader_.setVec3("uViewPosition", state_.camera.position());
  shader_.setInt("uTexture", 0);
  shader_.setInt("uMetallicMap", 1);
  shader_.setInt("uRoughnessMap", 2);
  shader_.setInt("uNormalMap", 3);
  shader_.setBool("uUnlit", false);

  const bool useMetallicMap =
      state_.useMetallicMap && state_.metallicMap.valid();
  const bool useRoughnessMap =
      state_.useRoughnessMap && state_.roughnessMap.valid();
  const bool useNormalMap = state_.useNormalMap && state_.normalMap.valid();
  shader_.setBool("uUseMetallicMap", useMetallicMap);
  shader_.setBool("uUseRoughnessMap", useRoughnessMap);
  shader_.setBool("uUseNormalMap", useNormalMap);
  if (useMetallicMap) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, state_.metallicMap.getTexture());
  }
  if (useRoughnessMap) {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, state_.roughnessMap.getTexture());
  }
  if (useNormalMap) {
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, state_.normalMap.getTexture());
  }

  // Direct and ambient light properties are sent as one GLSL struct.
  state_.light.apply(shader_);

  // Per-mesh material colors and map_Kd textures are applied here.
  state_.model.draw(shader_, state_.useTexture);

  // Draw a small unlit sphere as a visual marker for the light. The shader
  // treats `direction` as the way light travels, so the source sits in the
  // opposite direction from the model, one model-radius away.
  if (state_.light.showPositionMarker && !state_.lightSphere.meshes.empty() &&
      !state_.model.meshes.empty()) {
    const glm::vec3 markerPosition =
        state_.model.center() -
        glm::normalize(state_.light.direction) * state_.model.radius();
    const float scale = 0.02f; // radius 10 -> 0.2 world units
    const glm::mat4 markerModel = glm::translate(
        glm::mat4(1.0f), markerPosition - state_.lightSphere.center() * scale);
    shader_.setMat4("uModel", glm::scale(markerModel, glm::vec3(scale)));
    shader_.setBool("uUnlit", true);
    shader_.setVec3("uOverrideColor", state_.light.color);
    state_.lightSphere.draw(shader_, false);
    shader_.setBool("uUnlit", false);
  }

  if (useMetallicMap) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  if (useRoughnessMap) {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  if (useNormalMap) {
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
  glActiveTexture(GL_TEXTURE0);

  shader_.unuse();
}

int Application::run() {
  if (!initialize())
    return -1;

  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();
    state_.frame.update();

    // Apply the ImGui vsync checkbox every frame so it takes effect
    // immediately.
    glfwSwapInterval(state_.vsync ? 1 : 0);

    ui_.beginFrame();
    ui_.draw();
    updateCamera();

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    glClearColor(state_.skybox.x, state_.skybox.y, state_.skybox.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (framebufferWidth > 0 && framebufferHeight > 0)
      renderScene(framebufferWidth, framebufferHeight);

    ui_.endFrame();
    glfwSwapBuffers(window_);
  }

  shutdown();
  return 0;
}
