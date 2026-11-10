#include <glad/gl.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Camera.hpp"
#include "Frame.hpp"
#include "Model.hpp"
#include "Shader.h"

#include <cstdio>
#include <iostream>

namespace {
struct RendererContext {
  GLFWwindow *window;
  int WINDOW_WIDTH{1920};
  int WINDOW_HEIGHT{1080};

  bool vsync{false};

  glm::vec3 skybox{0};
  Camera camera;
  Frame frame;
  bool loadTexture{true};
};
void glfwErrorCallback(int error, const char *description) {
  std::fprintf(stderr, "GLFW::Error::%d: %s\n", error, description);
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
  ImGui::ColorEdit3("Skybox Color", &context.skybox.x);
  ImGui::Separator();
  ImGui::Text("Camera");
  ImGui::Text("Left mouse: orbit | Scroll: zoom");
  ImGui::Text("Distance: %.2f", context.camera.distance());

  ImGui::End();
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

  Model model("assets/models/utah_teapot.obj");
  if (model.meshes.empty()) {
    std::fprintf(stderr, "Failed to load model\n");
    shader.destroy();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(context.window);
    glfwTerminate();
    return -1;
  }

  context.camera = Camera(model.center(), model.radius() * 2.5f);

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
      shader.setVec3("uLightDirection",
                     glm::normalize(glm::vec3(-0.5f, -1.0f, -0.35f)));
      shader.setVec3("uBaseColor", glm::vec3(0.78f, 0.52f, 0.32f));

      model.draw(shader);
      shader.unuse();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(context.window);
  }

  shader.destroy();
  model.meshes.clear();

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwDestroyWindow(context.window);
  glfwTerminate();

  return 0;
}
