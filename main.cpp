#include "Light.hpp"
#include "Camera.hpp"
#include "Models.hpp"
#include "Shaders.hpp"
#include "WindowMaker.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

float lastFrame = 0.0f;

// ---- Helper declaration ----
void drawSuzanne(Shader &shader, Model &model, Light &light, Camera &camera,
                 const glm::mat4 &view, const glm::mat4 &projection,
                 const glm::vec3 &position, float time);

int main() {
  // Window
  WindowMaker wm(2000, 2000);
  GLFWwindow *window = wm.make_window();
  glEnable(GL_DEPTH_TEST);

  // Camera
  Camera camera(window);

  // Shaders
  Shader lambertShader("shaders/basic.vert", "shaders/lambert.frag");
  Shader phongShader("shaders/basic.vert", "shaders/phong.frag");
  Shader blinnShader("shaders/basic.vert", "shaders/blinn.frag");

  // Model
  Model suzanne("./suzanne_display/suzanne_display.obj");

  // Light
  Light light(glm::vec3(0.5f, 3.0f, 0.5f), glm::vec3(1.0f),
              "shaders/light.vert", "shaders/light.frag");

  // Render loop
  while (!glfwWindowShouldClose(window)) {
    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Input
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

    // Clear
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Matrices
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
                                            800.0f / 600.0f, 0.1f, 100.0f);

    float time = static_cast<float>(glfwGetTime());

    // Draw all three models
    drawSuzanne(lambertShader, suzanne, light, camera, view, projection,
                {-2.5f, 0.0f, 0.0f}, time);

    drawSuzanne(phongShader, suzanne, light, camera, view, projection,
                {0.0f, 0.0f, 0.0f}, time);

    drawSuzanne(blinnShader, suzanne, light, camera, view, projection,
                {2.5f, 0.0f, 0.0f}, time);

    // Light debug cube
    light.draw(view, projection);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

void drawSuzanne(Shader &shader, Model &model, Light &light, Camera &camera,
                 const glm::mat4 &view, const glm::mat4 &projection,
                 const glm::vec3 &position, float time) {
  shader.use();

  // Common uniforms
  shader.setMat4("view", view);
  shader.setMat4("projection", projection);
  shader.setVec3("albedo", glm::vec3(0.8f, 0.3f, 0.3f));

  // Some shaders need these, some will ignore them
  shader.setVec3("viewPos", camera.Position);
  shader.setFloat("shininess", 32.0f);

  light.apply(shader);

  // Model transform
  glm::mat4 modelMat = glm::mat4(1.0f);
  modelMat = glm::translate(modelMat, position);
  modelMat = glm::scale(modelMat, glm::vec3(0.6f));
  modelMat = glm::rotate(modelMat, time * 0.6f, glm::vec3(0.0f, 1.0f, 0.0f));

  shader.setMat4("model", modelMat);

  model.draw();
}
