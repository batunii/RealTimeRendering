#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

class WindowMaker {
public:
  const unsigned int m_height;
  const unsigned int m_width;

  WindowMaker(unsigned int height, unsigned int width)
      : m_height(height), m_width(width) {}

  GLFWwindow *make_window() {
    // 1. Init GLFW
    if (!glfwInit()) {
      throw std::runtime_error("Failed to initialize GLFW");
    }

    // 2. OpenGL version hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 3. Create window
    GLFWwindow *window =
        glfwCreateWindow(m_width, m_height, "Practice", nullptr, nullptr);

    if (!window) {
      glfwTerminate();
      throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);

    // 4. Load GL functions (CRITICAL)
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      glfwTerminate();
      throw std::runtime_error("Failed to initialize GLAD");
    }

    return window;
  }
};
