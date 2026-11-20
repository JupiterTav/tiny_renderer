#include "Application.hpp"

#include <glad/gl.h>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdio>
#include <filesystem>

void Application::glfwErrorCallback(int error, const char *description) {
  std::fprintf(stderr, "GLFW::Error::%d: %s\n", error, description);
}

Application::Application() = default;

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

  if (!shader_.load("assets/shaders/basic.vert",
                    "assets/shaders/basic.frag")) {
    std::fprintf(stderr, "Failed to load model shader\n");
    return false;
  }

  loadModel(state_.modelPath);
  if (state_.model.meshes.empty()) {
    std::fprintf(stderr, "Failed to load model\n");
    return false;
  }

  loadTexture(state_.texturePath);

  if (!state_.lightSphere.load("assets/models/sphere.obj"))
    std::fprintf(stderr, "Failed to load light marker sphere\n");

  ui_.setModelLoader([this](const std::filesystem::path &path) {
    loadModel(path);
  });
  ui_.setTextureLoader([this](const std::filesystem::path &path) {
    loadTexture(path);
  });

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

void Application::loadModel(const std::filesystem::path &path) {
  if (!state_.model.load(path)) {
    state_.status = "Failed to load model: " + path.string();
    state_.statusIsError = true;
    return;
  }

  state_.modelPath = path.string();
  state_.camera =
      Camera(state_.model.center(), state_.model.radius() * 2.5f);
  state_.status = "Loaded model: " + path.string();
  state_.statusIsError = false;
}

void Application::loadTexture(const std::filesystem::path &path) {
  if (!state_.model.loadModelTexture(path)) {
    state_.status = "Failed to load texture: " + path.string();
    state_.statusIsError = true;
    return;
  }

  state_.texturePath = path.string();
  state_.useTexture = true;
  state_.status = "Loaded texture: " + path.string();
  state_.statusIsError = false;
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
  shader_.setBool("uUnlit", false);

  // Direct and ambient light properties are sent as one GLSL struct.
  state_.light.apply(shader_);

  // Per-mesh material colors and map_Kd textures are applied here.
  state_.model.draw(shader_, state_.useTexture);

  // Draw a small unlit sphere as a visual marker for the light. The shader
  // treats `direction` as the way light travels, so the source sits in the
  // opposite direction from the model, one model-radius away.
  if (state_.light.showPositionMarker &&
      !state_.lightSphere.meshes.empty() && !state_.model.meshes.empty()) {
    const glm::vec3 markerPosition =
        state_.model.center() -
        glm::normalize(state_.light.direction) * state_.model.radius();
    const float scale = 0.02f;  // radius 10 -> 0.2 world units
    const glm::mat4 markerModel = glm::translate(
        glm::mat4(1.0f), markerPosition - state_.lightSphere.center() * scale);
    shader_.setMat4("uModel", glm::scale(markerModel, glm::vec3(scale)));
    shader_.setBool("uUnlit", true);
    shader_.setVec3("uOverrideColor", state_.light.color);
    state_.lightSphere.draw(shader_, false);
    shader_.setBool("uUnlit", false);
  }

  shader_.unuse();
}

int Application::run() {
  if (!initialize())
    return -1;

  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();
    state_.frame.update();

    // Apply the ImGui vsync checkbox every frame so it takes effect immediately.
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
