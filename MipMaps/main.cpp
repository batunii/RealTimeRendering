#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <array>

#include "../external/imgui/backends/imgui_impl_glfw.h"
#include "../external/imgui/backends/imgui_impl_opengl3.h"
#include "../external/imgui/imgui.h"

#include "Camera.hpp"
#include "Models.hpp"
#include "Shaders.hpp"
#include "Texture.hpp"
#include "WindowMaker.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>

float lastFrame = 0.0f;
bool cameraActive = false;
const std::array<const char *, 6> mipmap_min_list = {"LINEAR_MIPMAP_LINEAR",
                                                     "NEAREST_MIPMAP_LINEAR",
                                                     "LINEAR_MIPMAP_NEAREST",
                                                     "NEAREST_MIPMAP_NEAREST",
                                                     "LINEAR",
                                                     "NEAREST"};
int selectedMinConfig = 0;
const std::array<const char *, 2> mipmap_mag_list = {"LINEAR", "NEAREST"};
int selectedMagConfig = 0;
bool checkered_texture_active;
bool pattern_texture_active;
GLFWwindow *gWindow = nullptr;
Camera *gCamera = nullptr;
GLuint min;
GLuint mag;
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  int fbWidth, fbHeight;
  glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
  glViewport(0, 0, fbWidth, fbHeight);
}

int main() {
  WindowMaker wm(1200, 1800); // width, height
  GLFWwindow *window = wm.make_window("RTR - 3 Minmpas");
  gWindow = window;

  glfwMakeContextCurrent(window);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  int fbWidth, fbHeight;
  glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
  glViewport(0, 0, fbWidth, fbHeight);

  Camera camera(window);
  camera.mouseActive = false;
  gCamera = &camera;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  ModelClass checkered(
      "./checkered-tile-floor/source/Floor-Sketchfab/Floor.obj");
  Shader checkeredShader("./basic.vert", "./basic.frag");
  TextureClass checkeredTexture(
      "./checkered-tile-floor/textures/floor-diffuse-texture.png");
  TextureClass swirlTexture("./gdj-pattern-9842070_1920.png");
  while (!glfwWindowShouldClose(window)) {
    float currentFrame = static_cast<float>(glfwGetTime());
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    ImGuiIO &io = ImGui::GetIO();

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS &&
        !io.WantCaptureMouse) {
      if (!cameraActive) {
        cameraActive = true;
        camera.mouseActive = cameraActive;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      }
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && cameraActive) {
      cameraActive = false;
      camera.mouseActive = cameraActive;
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera.processKeyboard(GLFW_KEY_W, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera.processKeyboard(GLFW_KEY_S, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera.processKeyboard(GLFW_KEY_A, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera.processKeyboard(GLFW_KEY_D, deltaTime);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("RTR Assignment 4 - Mipmaps", nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Min Mapping");
    ImGui::ListBox("Choose min mip map mode", &selectedMinConfig,
                   &mipmap_min_list[0], 6);
    ImGui::Separator();
    ImGui::Text("Mag Mapping");

    ImGui::ListBox("Choose mag mode", &selectedMagConfig, &mipmap_mag_list[0],
                   2);
    ImGui::Separator();
    ImGui::Text("Choose Textures");
    ImGui::Checkbox("Checkered Pattern", &checkered_texture_active);
    ImGui::Checkbox("Swirl Pattern", &pattern_texture_active);
    ImGui::End();
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    if (fbHeight == 0)
      fbHeight = 1;
    float aspect = static_cast<float>(fbWidth) / static_cast<float>(fbHeight);

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection =
        glm::perspective(glm::radians(camera.Zoom), aspect, 0.1f, 10000.0f);

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 modelMat(1.0f);
    modelMat = glm::translate(modelMat, glm::vec3(1.0f, 0.0f, 1.0f));
    switch (selectedMinConfig) {
    case 0:
      min = GL_LINEAR_MIPMAP_LINEAR;
      break;
    case 1:
      min = GL_NEAREST_MIPMAP_LINEAR;
      break;
    case 2:
      min = GL_LINEAR_MIPMAP_NEAREST;
      break;
    case 3:
      min = GL_NEAREST_MIPMAP_NEAREST;
      break;
    case 4:
      min = GL_LINEAR;
      break;
    case 5:
      min = GL_NEAREST;
      break;
    }
    switch (selectedMagConfig) {
    case 0:
      mag = GL_LINEAR;
      break;
    case 1:
      mag = GL_NEAREST;
      break;
    }
    if (checkered_texture_active) {
      checkeredTexture.change_mipmap(min, mag);
      checkeredTexture.bind();
    } else if (pattern_texture_active) {
      swirlTexture.change_mipmap(min, mag);
      swirlTexture.bind();
    }
    checkeredShader.use();
    checkeredShader.setMat4("model", modelMat);
    checkeredShader.setMat4("view", view);
    checkeredShader.setMat4("projection", projection);
    checkered.drawModel(checkeredShader.ID);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();
  return 0;
}
