#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class Camera {
public:
    glm::vec3 Position{0.0f, 0.0f, 3.0f};
    glm::vec3 Front{0.0f, 0.0f, -1.0f};
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp{0.0f, 1.0f, 0.0f};

    float Yaw{-90.0f};
    float Pitch{0.0f};

    float MovementSpeed{2.5f};
    float MouseSensitivity{0.1f};
    float Zoom{45.0f};

    Camera(GLFWwindow* window,
           glm::vec3 position = {0.0f, 0.0f, 3.0f})
        : Position(position)
    {
        updateVectors();

        glfwSetWindowUserPointer(window, this);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetScrollCallback(window, scrollCallback);

        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void processKeyboard(int key, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (key == GLFW_KEY_W) Position += Front * velocity;
        if (key == GLFW_KEY_S) Position -= Front * velocity;
        if (key == GLFW_KEY_A) Position -= Right * velocity;
        if (key == GLFW_KEY_D) Position += Right * velocity;
    }

private:
    bool firstMouse{true};
    float lastX{400.0f};
    float lastY{300.0f};

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos)
    {
        auto* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
        if (!cam) return;

        if (cam->firstMouse) {
            cam->lastX = xpos;
            cam->lastY = ypos;
            cam->firstMouse = false;
        }

        float xoffset = xpos - cam->lastX;
        float yoffset = cam->lastY - ypos;

        cam->lastX = xpos;
        cam->lastY = ypos;

        cam->processMouse(xoffset, yoffset);
    }

    static void scrollCallback(GLFWwindow* window, double, double yoffset)
    {
        auto* cam = static_cast<Camera*>(glfwGetWindowUserPointer(window));
        if (!cam) return;

        cam->Zoom -= static_cast<float>(yoffset);
        if (cam->Zoom < 1.0f) cam->Zoom = 1.0f;
        if (cam->Zoom > 45.0f) cam->Zoom = 45.0f;
    }

    void processMouse(float xoffset, float yoffset)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        if (Pitch > 89.0f)  Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;

        updateVectors();
    }

    void updateVectors()
    {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};
