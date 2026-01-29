#include "Light.hpp"
#include "Camera.hpp"
#include "Models.hpp"
#include "Shaders.hpp"
#include "WindowMaker.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include "./external/imgui/imgui.h"
#include "./external/imgui/backends/imgui_impl_glfw.h"
#include "./external/imgui/backends/imgui_impl_opengl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

float lastFrame = 0.0f;
bool cameraActive = false;

// ---- UI-controlled properties ----
glm::vec3 uiLightPos   = glm::vec3(2.0f, 3.0f, 2.0f);
glm::vec3 uiLightColor = glm::vec3(1.0f);

glm::vec3 uiAlbedo = glm::vec3(0.1f, 0.5f, 0.1f);
float uiShininess  = 32.0f;
float uiToonLevels = 4.0f;
float uiSpecularStrength = 0.5f;
float uiDiffusionStrength = 1.0f;
float uiRoughness = 0.4f;
float uiMetallic  = 0.0f;
float uiAO        = 1.0f;

// Scene
bool rotateModels = true;


void framebuffer_size_callback(GLFWwindow *, int w, int h) {
  glViewport(0, 0, w, h);
}

void drawSuzanne(Shader &shader, Model &model, Light &light, Camera &camera,
                 const glm::vec3 &position, float time, const glm::mat4 &view,
                 const glm::mat4 &projection) {
  shader.use();

  glm::mat4 modelMat(1.0f);
  modelMat = glm::translate(modelMat, position);
  modelMat = glm::scale(modelMat, glm::vec3(0.006f));
  modelMat = glm::rotate(modelMat, time * 0.6f, glm::vec3(0.0f, 1.0f, 0.0f));

  shader.setMat4("model", modelMat);
  shader.setMat4("view", view);
  shader.setMat4("projection", projection);

  // Common uniforms (ignored if unused)
  shader.setVec3("viewPos", camera.Position);
  shader.setVec3("albedo", uiAlbedo);
  shader.setFloat("shininess", uiShininess);
  shader.setFloat("strength", uiSpecularStrength);
  shader.setFloat("diffusionStrength", uiDiffusionStrength);

  // Toon Shader
  shader.setFloat("levels", uiToonLevels);

  // PBR-friendly defaults
  shader.setFloat("roughness", uiRoughness);
  shader.setFloat("metallic", uiMetallic);
  shader.setFloat("ao", uiAO);
  
  light.updatePosCol(uiLightColor, uiLightPos);
  light.apply(shader);
  model.draw();
}

int main() {
   WindowMaker wm(1800, 900);
  GLFWwindow *window = wm.make_window();
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  glEnable(GL_DEPTH_TEST);

  Camera camera(window);
  camera.mouseActive = false;
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  Shader lambert("shaders/basic.vert", "shaders/lambert.frag");
  // Shader blinn("shaders/basic.vert", "shaders/blinn.frag");
  Shader phong("shaders/basic.vert", "shaders/phong.frag");
  Shader toon("shaders/basic.vert", "shaders/toon.frag");
  Shader pbr("shaders/basic.vert", "shaders/pbr.frag");

  Model suzanne("./teapot.obj");

  Light light(uiLightPos, uiLightColor,
              "shaders/light.vert", "shaders/light.frag");


  while (!glfwWindowShouldClose(window)) {
    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    ImGuiIO &io = ImGui::GetIO();

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !io.WantCaptureMouse ) {
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

    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                            1800.0f / 900.0f, 0.1f, 100.0f);

    float t = rotateModels ? static_cast<float>(glfwGetTime()) : 0.0f;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Reflectance Models");
    ImGui::Text("Side-by-side comparison");
    ImGui::Checkbox("Rotate models", &rotateModels);

    ImGui::Text("Light");
    ImGui::DragFloat3("Position", &uiLightPos[0], 0.1f);
    ImGui::ColorEdit3("Color", &uiLightColor[0]);

    ImGui::Separator();

    // ---- Material ----
    ImGui::Text("Material - Phong");
    ImGui::ColorEdit3("Albedo", &uiAlbedo[0]);
    ImGui::SliderFloat("Shininess", &uiShininess, 0.0f, 256.0f);
    ImGui::SliderFloat("SpecularStrength", &uiSpecularStrength, 0.0f, 2.0f);
    ImGui::SliderFloat("DiffusionStrength", &uiDiffusionStrength, 0.0f, 2.0f);

// Toon Shader    

    ImGui::Text("Toon Shader");
    ImGui::SliderFloat("ToonLevels", &uiToonLevels, 0.0f, 6.0f);

    // ---- PBR ----
    ImGui::Separator();
    ImGui::Text("PBR - Cook");
    ImGui::SliderFloat("Roughness", &uiRoughness, 0.02f, 1.0f);
    ImGui::SliderFloat("Metallic", &uiMetallic, 0.0f, 1.0f);
    ImGui::SliderFloat("AO", &uiAO, 0.0f, 1.0f);

    ImGui::Separator();
    ImGui::End();

     //drawSuzanne(lambert, suzanne, light, camera, {-2.0f, 0.0f, 0.0f}, t, view,
     //           projection);
    drawSuzanne(phong, suzanne, light, camera, {0.0f, 0.0f, 0.0f}, t, view,
                projection);
    drawSuzanne(toon, suzanne, light, camera, {2.0f, 0.0f, 0.0f}, t, view,
                projection);
    drawSuzanne(pbr, suzanne, light, camera, {4.0f, 0.0f, 0.0f}, t, view,
                projection);

   // light.updatePosCol(uiLightColor, uiLightPos);
    light.draw(view, projection);

    // ---- Render ImGui ----
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  // ---- Cleanup ----
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  glfwTerminate();
  return 0;
}
