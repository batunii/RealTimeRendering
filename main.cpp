#include "Light.hpp"
#include "Camera.hpp"
#include "Models.hpp"
#include "Shaders.hpp"
#include "WindowMaker.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

float lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow *, int w, int h) {
  glViewport(0, 0, w, h);
}

void drawSuzanne(Shader &shader, Model &model, Light &light, Camera &camera,
                 const glm::vec3 &position, float time, const glm::mat4 &view,
                 const glm::mat4 &projection) {
  shader.use();

  glm::mat4 modelMat(1.0f);
  modelMat = glm::translate(modelMat, position);
  modelMat = glm::scale(modelMat, glm::vec3(0.6f));
  modelMat = glm::rotate(modelMat, time * 0.6f, glm::vec3(0, 1, 0));

  shader.setMat4("model", modelMat);
  shader.setMat4("view", view);
  shader.setMat4("projection", projection);

  shader.setVec3("viewPos", camera.Position);
  shader.setVec3("albedo", glm::vec3(0.8f, 0.3f, 0.3f));
  shader.setFloat("shininess", 32.0f);

  // Only for PBR

  shader.setFloat("roughness", 0.4f);
  shader.setFloat("metallic", 0.0f);
  shader.setFloat("ao", 1.0f);

  light.apply(shader);
  model.draw();
}

// -------------------- MAIN --------------------
int main() {
  // ---- Window ----
  WindowMaker wm(1800, 900);
  GLFWwindow *window = wm.make_window();
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  glEnable(GL_DEPTH_TEST);

  Camera camera(window);

  Shader lambert("shaders/basic.vert", "shaders/lambert.frag");
  Shader phong("shaders/basic.vert", "shaders/phong.frag");
  Shader blinn("shaders/basic.vert", "shaders/blinn.frag");
  Shader toon("shaders/basic.vert", "shaders/toon.frag");
  Shader pbr("shaders/basic.vert", "shaders/pbr.frag");

  Model suzanne("./suzanne_display/suzanne_display.obj");

  Light light(glm::vec3(2.0f, 3.0f, 2.0f), glm::vec3(1.0f),
              "shaders/light.vert", "shaders/light.frag");

  while (!glfwWindowShouldClose(window)) {
    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

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

    float t = static_cast<float>(glfwGetTime());

    drawSuzanne(lambert, suzanne, light, camera, {-4.0f, 0.0f, 0.0f}, t, view,
                projection);
    drawSuzanne(phong, suzanne, light, camera, {-2.0f, 0.0f, 0.0f}, t, view,
                projection);
    drawSuzanne(toon, suzanne, light, camera, {0.0f, 0.0f, 0.0f}, t, view,
                projection);
    drawSuzanne(pbr, suzanne, light, camera, {2.0f, 0.0f, 0.0f}, t, view,
                projection);

    light.draw(view, projection);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
