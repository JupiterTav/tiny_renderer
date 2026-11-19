#include <glad/gl.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "ImGuiFileDialog.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Camera.hpp"
#include "Frame.hpp"
#include "Light.hpp"
#include "Model.hpp"
#include "Shader.h"

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>

namespace {
struct RendererContext {
  GLFWwindow *window;
  int WINDOW_WIDTH{1920};
  int WINDOW_HEIGHT{1080};

  bool vsync{false};

  glm::vec3 skybox{0};
  Camera camera;
  Frame frame;
  bool useTexture{true};

  Model model;
  std::string modelPath{"assets/models/chinese-dragon.obj"};
  std::string texturePath{"assets/textures/Image_0.jpg"};
  std::string status;
  bool statusIsError{false};

  Light light;
};

void glfwErrorCallback(int error, const char *description) {
  std::fprintf(stderr, "GLFW::Error::%d: %s\n", error, description);
}

std::string parentPath(const std::filesystem::path &path) {
  const std::filesystem::path parent = path.parent_path();
  return parent.empty() ? "." : parent.string();
}

void loadModel(RendererContext &context, const std::filesystem::path &path) {
  if (!context.model.load(path)) {
    context.status = "Failed to load model: " + path.string();
    context.statusIsError = true;
    return;
  }

  context.modelPath = path.string();
  context.camera =
      Camera(context.model.center(), context.model.radius() * 2.5f);
  context.status = "Loaded model: " + path.string();
  context.statusIsError = false;
}

void loadTexture(RendererContext &context, const std::filesystem::path &path) {
  if (!context.model.loadModelTexture(path)) {
    context.status = "Failed to load texture: " + path.string();
    context.statusIsError = true;
    return;
  }

  context.texturePath = path.string();
  context.useTexture = true;
  context.status = "Loaded texture: " + path.string();
  context.statusIsError = false;
}

void drawFileDialogs(RendererContext &context) {
  ImGuiFileDialog *dialog = ImGuiFileDialog::Instance();

  if (dialog->Display("LoadModelDlg")) {
    if (dialog->IsOk())
      loadModel(context, dialog->GetFilePathName());
    dialog->Close();
  }

  if (dialog->Display("LoadTextureDlg")) {
    if (dialog->IsOk())
      loadTexture(context, dialog->GetFilePathName());
    dialog->Close();
  }
}
}; // namespace

int imGuiInitialize(RendererContext &context) {

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  if (!ImGui_ImplGlfw_InitForOpenGL(context.window, true)) {
    std::fprintf(stderr, "Failed to initialize ImGui GLFW backend");
    glfwDestroyWindow(context.window);
    glfwTerminate();
    return -1;
  }

  if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
    std::fprintf(stderr, "Failed to initialize ImGui OpenGL");
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(context.window);
    glfwTerminate();
    return -1;
  }

  return 1;
}
void imGuiDrawFrame(RendererContext &context) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("Tiny Renderer");
  ImGui::Text("OpenGL: %s",
              reinterpret_cast<const char *>(glGetString(GL_VERSION)));
  ImGui::Text("Statistics");
  ImGui::Text("FPS: %.0f", context.frame.fps());
  ImGui::Text("Frame: %.2f ms", context.frame.frameTimeMs());
  ImGui::Text("Delta: %.2f ms", context.frame.deltaTime() * 1000.0f);
  ImGui::Text("Frame #: %llu",
              static_cast<unsigned long long>(context.frame.frameCount()));
  ImGui::Text("Elapsed: %.1f s", context.frame.elapsedTime());
  ImGui::Separator();
  ImGui::Checkbox("vsync", &context.vsync);
  ImGui::Separator();
  ImGui::Text("Camera");
  ImGui::Text("Left mouse: orbit | Scroll: zoom");
  ImGui::Text("Distance: %.2f", context.camera.distance());

  ImGui::Separator();
  ImGui::ColorEdit3("Skybox Color", &context.skybox.x);
  ImGui::Checkbox("Use Texture", &context.useTexture);
  ImGui::Separator();
  ImGui::Text("Assets");
  ImGui::Text("Meshes: %zu | Materials: %zu", context.model.meshes.size(),
              context.model.materials.size());
  ImGui::TextWrapped("Model: %s", context.modelPath.c_str());
  if (ImGui::Button("Load Model...")) {
    IGFD::FileDialogConfig config;
    config.path = parentPath(context.modelPath);
    config.fileName =
        std::filesystem::path(context.modelPath).filename().string();
    ImGuiFileDialog::Instance()->OpenDialog("LoadModelDlg", "Choose model",
                                            ".obj", config);
  }
  ImGui::TextWrapped("Texture: %s", context.texturePath.c_str());
  if (ImGui::Button("Load Texture...")) {
    IGFD::FileDialogConfig config;
    config.path = parentPath(context.texturePath);
    config.fileName =
        std::filesystem::path(context.texturePath).filename().string();
    ImGuiFileDialog::Instance()->OpenDialog("LoadTextureDlg", "Choose texture",
                                            ".png,.jpg,.jpeg,.bmp,.tga",
                                            config);
  }
  if (!context.status.empty())
    ImGui::TextColored(context.statusIsError ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
                                             : ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                       "%s", context.status.c_str());
  ImGui::Separator();
  if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::CollapsingHeader("Directional Light",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::DragFloat3("Direction", &context.light.direction.x, 0.01f);
      ImGui::ColorEdit3("Color", &context.light.color.x);
      ImGui::SliderFloat("Intensity", &context.light.intensity, 0.0f, 5.0f);
    }
    if (ImGui::CollapsingHeader("Ambient Light",
                                ImGuiTreeNodeFlags_DefaultOpen))
      ImGui::SliderFloat("Ambient Intensity",
                         &context.light.ambientContribution, 0, 1);
    ImGui::Separator();
  }
  ImGui::End();

  drawFileDialogs(context);
}

void updateCamera(RendererContext &context) {
  static double lastMouseX = 0.0;
  static double lastMouseY = 0.0;
  static bool hasLastMouse = false;

  double mouseX = 0.0;
  double mouseY = 0.0;
  glfwGetCursorPos(context.window, &mouseX, &mouseY);

  if (!hasLastMouse) {
    lastMouseX = mouseX;
    lastMouseY = mouseY;
    hasLastMouse = true;
  }

  const ImGuiIO &io = ImGui::GetIO();
  if (!io.WantCaptureMouse) {
    if (glfwGetMouseButton(context.window, GLFW_MOUSE_BUTTON_LEFT) ==
        GLFW_PRESS) {
      context.camera.orbit(-static_cast<float>(mouseX - lastMouseX),
                           static_cast<float>(mouseY - lastMouseY));
    }

    if (io.MouseWheel != 0.0f)
      context.camera.zoom(io.MouseWheel);
  }

  lastMouseX = mouseX;
  lastMouseY = mouseY;
}

int main(int argc, char *argv[]) {
  glfwSetErrorCallback(glfwErrorCallback);
  if (glfwInit() == GLFW_FALSE)
    return -1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  RendererContext context;

  context.window = glfwCreateWindow(context.WINDOW_WIDTH, context.WINDOW_HEIGHT,
                                    "Tiny Renderer", nullptr, nullptr);
  if (!context.window) {
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(context.window);
  glfwSwapInterval((int)context.vsync);

  if (gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) ==
      GLFW_FALSE) {
    std::fprintf(stderr, "Failed to load OpenGL functions\n");
    glfwDestroyWindow(context.window);
    glfwTerminate();
    return -1;
  }

  if (!imGuiInitialize(context)) {
    std::fprintf(stderr, "Failed to initialize Im GUI\n");
    glfwDestroyWindow(context.window);
    glfwTerminate();
    return -1;
  }

  Shader shader("assets/shaders/basic.vert", "assets/shaders/basic.frag");
  if (!shader) {
    std::fprintf(stderr, "Failed to load model shader\n");
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(context.window);
    glfwTerminate();
    return -1;
  }

  loadModel(context, context.modelPath);
  if (context.model.meshes.empty()) {
    std::fprintf(stderr, "Failed to load model\n");
    shader.destroy();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(context.window);
    glfwTerminate();
    return -1;
  }

  loadTexture(context, context.texturePath);

  glEnable(GL_DEPTH_TEST);

  while (!glfwWindowShouldClose(context.window)) {
    glfwPollEvents();
    context.frame.update();

    imGuiDrawFrame(context);
    updateCamera(context);

    int framebufferWidth{0};
    int framebufferHeight{0};

    glfwGetFramebufferSize(context.window, &framebufferWidth,
                           &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    glClearColor(context.skybox.x, context.skybox.y, context.skybox.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (framebufferWidth > 0 && framebufferHeight > 0) {
      const float aspect =
          static_cast<float>(framebufferWidth) / framebufferHeight;

      shader.use();
      shader.setMat4("uModel", glm::mat4(1.0f));
      shader.setMat4("uView", context.camera.view());
      shader.setMat4("uProjection", context.camera.projection(aspect));
      shader.setVec3("uViewPosition", context.camera.position());
      shader.setInt("uTexture", 0);

      // Direct and ambient light properties are sent as one GLSL struct.
      context.light.apply(shader);

      // Per-mesh material colors and map_Kd textures are applied here.
      context.model.draw(shader, context.useTexture);

      shader.unuse();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(context.window);
  }

  shader.destroy();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(context.window);
  glfwTerminate();

  return 0;
}
