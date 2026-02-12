#include "Light.hpp"
#include <array>
#include <glad/glad.h>
#include "./external/imgui/backends/imgui_impl_glfw.h"
#include "./external/imgui/backends/imgui_impl_opengl3.h"
#include "./external/imgui/imgui.h"
#include "Camera.hpp"
#include "Models.hpp"
#include "Shaders.hpp"
#include "Texture.hpp"
#include "WindowMaker.hpp"
#include "imgui_internal.h"
#include <GLFW/glfw3.h>
#include <glm/ext/scalar_constants.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>
#include <string>

float lastFrame = 0.0f;
bool cameraActive = false;
glm::vec3 uiLightPos = glm::vec3(2.0f, 2.0f, 2.0f);
glm::vec3 uiLightCol = glm::vec3(1.0f);
const std::array<const char *, 2> uiShaderList = {"Blinn-Phong", "Toon"};
int uiSelectedShader = 0;

// Material properties
float uiAmbientStrength = 0.2f;
float uiSpecularStrength = 1.0f;
float uiShininess = 128.0f;
float uiBumpStrength = 1.0f;
bool useNormalMapping = true;
bool uiRotate = true;

void framebuffer_size_callback(GLFWwindow *, int w, int h) {
  glViewport(0, 0, w, h);
}

void drawObject(Shader &shader, Model &model, Camera &camera,
                const glm::vec3 &position, const glm::mat4 &view,
                const glm::mat4 &projection, float scale = 1.0f, const float time = 0, const bool rotate = false ) {
  shader.use();
  glm::mat4 modelMat(1.0f);
  modelMat = glm::translate(modelMat, position);

  modelMat = rotate ? glm::rotate(modelMat, float(glm::radians(90.0f)), glm::vec3(1.0f, 0.0f, 0.0f))  : modelMat;
  // Apply rotations (in degrees) - order: X, Y, Z
  if (uiRotate)  
    modelMat = glm::rotate(modelMat, time/2, glm::vec3(0.0f, 1.0f, 0.0f));
  
  modelMat = glm::scale(modelMat, glm::vec3(scale));
  
  shader.setMat4("model", modelMat);
  shader.setMat4("view", view);
  shader.setMat4("projection", projection);
  shader.setVec3("viewPos", camera.Position);
  shader.setVec3("lightPos", uiLightPos);
  shader.setVec3("lightColor", uiLightCol);
  shader.setFloat("ambientStrength", uiAmbientStrength);
  shader.setFloat("specularStrength", uiSpecularStrength);
  shader.setFloat("shininess", uiShininess);
  shader.setFloat("bumpStrength", uiBumpStrength);
  shader.setBool("useNormalMap", useNormalMapping);
  
  model.draw(shader.ID);
}

void drawSkybox(Shader &shader, Model &model, const glm::mat4 &view,
                const glm::mat4 &projection, Texture &skyboxTexture) {
  glDepthFunc(GL_LEQUAL);
  shader.use();
  skyboxTexture.bind(0);
  shader.setInt("skyboxTexture", 0);
  glm::mat4 modelMat = glm::mat4(1.0f);
  modelMat = glm::scale(modelMat, glm::vec3(100.0f));
  glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
  shader.setMat4("model", modelMat);
  shader.setMat4("view", skyboxView);
  shader.setMat4("projection", projection);
  model.draw(shader.ID);
  glDepthFunc(GL_LESS);
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

  Shader boxShader("./shaders/basic.vert", "./shaders/basic_albedo.frag");
  Shader suzzaneShader("./shaders/basic.vert", "./shaders/basic.frag");
  Shader suzzaneShaderToon("./shaders/basic.vert", "./shaders/toon.frag");
  Shader skyboxShader("./shaders/skybox.vert", "./shaders/skybox.frag");

  std::cout << "Loading models..." << std::endl;
  Model suzzane("./suzzane/test_low.obj");  
  Model manhole("./manhole/normal_map_test_-_manhole.obj");  
  Model box("./ImageToStl.com_r_normal_cube/r_normal_cube.obj");  

  Light light(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(1.0f),
              "./shaders/light.vert", "./shaders/light.frag");

  while (!glfwWindowShouldClose(window)) {
    float currentFrame = glfwGetTime();
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

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                           1800.0f / 900.0f, 0.1f, 100.0f);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("RTR Assignment 3 - Normals", nullptr,
                 ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::Text("Light Settings");
    ImGui::SliderFloat3("Light Position", &uiLightPos[0], -15.0, 15.0);
    ImGui::ColorEdit3("Light Color", &uiLightCol[0]);
    
    ImGui::Separator();
    ImGui::Text("Material Settings");
    ImGui::SliderFloat("Ambient Strength", &uiAmbientStrength, 0.0f, 1.0f);
    ImGui::SliderFloat("Specular Strength", &uiSpecularStrength, 0.0f, 2.0f);
    ImGui::SliderFloat("Shininess", &uiShininess, 1.0f, 256.0f);
    ImGui::SliderFloat("Bumpiness", &uiBumpStrength, 0.0f, 2.0f);
    ImGui::ListBox("Shaders", &uiSelectedShader, &uiShaderList[0], 2);    
    
    ImGui::Separator();
    ImGui::Checkbox("Enable Normal Mapping", &useNormalMapping);
    ImGui::Checkbox("Rotate", &uiRotate);
    
    ImGui::End();

    light.updatePosCol(uiLightCol, uiLightPos);
    light.draw(view, projection);

    if (uiSelectedShader == 0)
      drawObject(suzzaneShader, suzzane, camera, glm::vec3(-2.5f, 0.0f, 0.0f),
                 view, projection, 1, currentFrame);
    else
      drawObject(suzzaneShaderToon, suzzane, camera,
                 glm::vec3(-2.5f, 0.0f, 0.0f), view, projection, 1,
                 currentFrame);
    drawObject(boxShader, box, camera, glm::vec3(2.5f, 0, 0), view, projection,
               1.0f, currentFrame);
    drawObject(boxShader, manhole, camera, glm::vec3(7.5f, 0.0f, 0.0f),
               view, projection, 1.0f,0, true);
    

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
