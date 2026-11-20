#include "ImGuiLayer.hpp"

#include <glad/gl.h>

#include "ImGuiFileDialog.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cstdio>
#include <filesystem>
#include <utility>

namespace {
std::string parentPath(const std::filesystem::path &path) {
  const std::filesystem::path parent = path.parent_path();
  return parent.empty() ? "." : parent.string();
}
} // namespace

ImGuiLayer::ImGuiLayer(AppState &state) : state_(state) {}

ImGuiLayer::~ImGuiLayer() { shutdown(); }

bool ImGuiLayer::init(GLFWwindow *window) {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
    std::fprintf(stderr, "Failed to initialize ImGui GLFW backend\n");
    ImGui::DestroyContext();
    return false;
  }

  if (!ImGui_ImplOpenGL3_Init("#version 330 core")) {
    std::fprintf(stderr, "Failed to initialize ImGui OpenGL backend\n");
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    return false;
  }

  initialized_ = true;
  return true;
}

void ImGuiLayer::shutdown() {
  if (!initialized_)
    return;

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  initialized_ = false;
}

void ImGuiLayer::beginFrame() {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiLayer::draw() {
  ImGui::Begin("Tiny Renderer");
  ImGui::Text("OpenGL: %s",
              reinterpret_cast<const char *>(glGetString(GL_VERSION)));

  ImGui::Text("Statistics");
  ImGui::Text("FPS: %.0f", state_.frame.fps());
  ImGui::Text("Frame: %.2f ms", state_.frame.frameTimeMs());
  ImGui::Text("Delta: %.2f ms", state_.frame.deltaTime() * 1000.0f);
  ImGui::Text("Frame #: %llu",
              static_cast<unsigned long long>(state_.frame.frameCount()));
  ImGui::Text("Elapsed: %.1f s", state_.frame.elapsedTime());
  ImGui::Separator();

  ImGui::Checkbox("vsync", &state_.vsync);
  ImGui::Separator();

  ImGui::Text("Camera");
  ImGui::Text("Left mouse: orbit | Scroll: zoom");
  ImGui::Text("Distance: %.2f", state_.camera.distance());
  ImGui::Separator();

  ImGui::ColorEdit3("Skybox Color", &state_.skybox.x);
  ImGui::Checkbox("Use Texture", &state_.useTexture);
  ImGui::Separator();

  ImGui::Text("Assets");
  ImGui::Text("Meshes: %zu | Materials: %zu", state_.model.meshes.size(),
              state_.model.materials.size());
  ImGui::TextWrapped("Model: %s", state_.modelPath.c_str());
  if (ImGui::Button("Load Model...")) {
    IGFD::FileDialogConfig config;
    config.path = parentPath(state_.modelPath);
    config.fileName =
        std::filesystem::path(state_.modelPath).filename().string();
    ImGuiFileDialog::Instance()->OpenDialog("LoadModelDlg", "Choose model",
                                            ".obj", config);
  }

  ImGui::TextWrapped("Texture: %s", state_.texturePath.c_str());
  if (ImGui::Button("Load Texture...")) {
    IGFD::FileDialogConfig config;
    config.path = parentPath(state_.texturePath);
    config.fileName =
        std::filesystem::path(state_.texturePath).filename().string();
    ImGuiFileDialog::Instance()->OpenDialog("LoadTextureDlg", "Choose texture",
                                            ".png,.jpg,.jpeg,.bmp,.tga",
                                            config);
  }

  if (!state_.status.empty())
    ImGui::TextColored(state_.statusIsError ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
                                            : ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                       "%s", state_.status.c_str());
  ImGui::Separator();

  if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
    if (ImGui::CollapsingHeader("Directional Light",
                                ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::DragFloat3("Direction", &state_.light.direction.x, 0.01f);
      ImGui::ColorEdit3("Color", &state_.light.color.x);
      ImGui::SliderFloat("Intensity", &state_.light.intensity, 0.0f, 5.0f);

      ImGui::Checkbox("Show Marker", &state_.light.showPositionMarker);
      if (!state_.model.meshes.empty()) {
        const glm::vec3 markerPosition =
            state_.model.center() -
            glm::normalize(state_.light.direction) * state_.model.radius();
        ImGui::Text("Position: %.2f, %.2f, %.2f", markerPosition.x,
                    markerPosition.y, markerPosition.z);
      }
    }
    if (ImGui::CollapsingHeader("Ambient Light",
                                ImGuiTreeNodeFlags_DefaultOpen))
      ImGui::SliderFloat("Ambient Intensity", &state_.light.ambientContribution,
                         0, 1);
    ImGui::Separator();
  }
  ImGui::End();

  drawFileDialogs();
}

void ImGuiLayer::endFrame() {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiLayer::drawFileDialogs() {
  ImGuiFileDialog *dialog = ImGuiFileDialog::Instance();

  if (dialog->Display("LoadModelDlg")) {
    if (dialog->IsOk() && modelLoader_)
      modelLoader_(dialog->GetFilePathName());
    dialog->Close();
  }

  if (dialog->Display("LoadTextureDlg")) {
    if (dialog->IsOk() && textureLoader_)
      textureLoader_(dialog->GetFilePathName());
    dialog->Close();
  }
}

bool ImGuiLayer::wantsCaptureMouse() const {
  return ImGui::GetIO().WantCaptureMouse;
}

float ImGuiLayer::mouseWheel() const { return ImGui::GetIO().MouseWheel; }
