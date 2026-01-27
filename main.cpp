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

#include <iostream>

float lastFrame = 0.0f;
bool cameraActive = false;

void framebuffer_size_callback(GLFWwindow *, int w, int h) {
  glViewport(0, 0, w, h);
}

// --------------------------------------------------
// Draw helper
// --------------------------------------------------
void drawSuzanne(Shader &shader, Model &model, Light &light, Camera &camera,
                 const glm::vec3 &position, float time, const glm::mat4 &view,
                 const glm::mat4 &projection) {
  shader.use();

  glm::mat4 modelMat(1.0f);
  modelMat = glm::translate(modelMat, position);
  modelMat = glm::scale(modelMat, glm::vec3(0.6f));
  modelMat = glm::rotate(modelMat, time * 0.6f, glm::vec3(0.0f, 1.0f, 0.0f));

  shader.setMat4("model", modelMat);
  shader.setMat4("view", view);
  shader.setMat4("projection", projection);

  // Common uniforms (ignored if unused)
  shader.setVec3("viewPos", camera.Position);
  shader.setVec3("albedo", glm::vec3(0.8f, 0.3f, 0.3f));
  shader.setFloat("shininess", 32.0f);

  // PBR-friendly defaults
  shader.setFloat("roughness", 0.4f);
  shader.setFloat("metallic", 0.0f);
  shader.setFloat("ao", 1.0f);

  light.apply(shader);
  model.draw();
}

// --------------------------------------------------
// MAIN
// --------------------------------------------------
int main() {
  // ---- Window ----
  WindowMaker wm(1800, 900);
  GLFWwindow *window = wm.make_window();
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  glEnable(GL_DEPTH_TEST);

  Camera camera(window);

  // ---- ImGui init (vendored, normal path) ----
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 330");

  // ---- Shaders ----
  Shader lambert("shaders/basic.vert", "shaders/lambert.frag");
  Shader phong("shaders/basic.vert", "shaders/phong.frag");
  Shader blinn("shaders/basic.vert", "shaders/blinn.frag");
  Shader toon("shaders/basic.vert", "shaders/toon.frag");
  Shader pbr("shaders/basic.vert", "shaders/pbr.frag");

  // ---- Model ----
  Model suzanne("./suzanne_display/suzanne_display.obj");

  // ---- Light ----
  Light light(glm::vec3(2.0f, 3.0f, 2.0f), glm::vec3(1.0f),
              "shaders/light.vert", "shaders/light.frag");

  // ---- UI state ----
  bool rotateModels = true;

  // ---- Render loop ----
  while (!glfwWindowShouldClose(window)) {
    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    ImGuiIO &io = ImGui::GetIO();

    // Click on scene → enable camera
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS &&
        !io.WantCaptureMouse) {
      if (!cameraActive) {
        cameraActive = true;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      }
    }

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && cameraActive) {
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

    // ---- Clear ----
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // ---- Matrices ----
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                            1800.0f / 900.0f, 0.1f, 100.0f);

    float t = rotateModels ? static_cast<float>(glfwGetTime()) : 0.0f;

    // ---- ImGui frame ----
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Reflectance Models");
    ImGui::Text("Side-by-side comparison");
    ImGui::Checkbox("Rotate models", &rotateModels);
    ImGui::End();

    // ---- Draw models ----
    drawSuzanne(lambert, suzanne, light, camera, {-4.0f, 0.0f, 0.0f}, t, view,
                projection);
    drawSuzanne(phong, suzanne, light, camera, {-2.0f, 0.0f, 0.0f}, t, view,
                projection);
    drawSuzanne(blinn, suzanne, light, camera, {0.0f, 0.0f, 0.0f}, t, view,
                projection);
    drawSuzanne(toon, suzanne, light, camera, {2.0f, 0.0f, 0.0f}, t, view,
                projection);
    drawSuzanne(pbr, suzanne, light, camera, {4.0f, 0.0f, 0.0f}, t, view,
                projection);

    // ---- Light debug ----
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
