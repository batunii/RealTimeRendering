#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// Using ImGui for UI
#include "../external/imgui/backends/imgui_impl_glfw.h"
#include "../external/imgui/backends/imgui_impl_opengl3.h"
#include "../external/imgui/imgui.h"

// Header files
#include "Camera.hpp"
#include "Models.hpp"
#include "Shaders.hpp"
#include "Texture.hpp"
#include "WindowMaker.hpp"

// GLM for matrix maths
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

// -------------------------------------------------------
// Globals
// -------------------------------------------------------
float lastFrame    = 0.0f;
bool  cameraActive = false;
GLFWwindow *gWindow  = nullptr;
Camera     *gCamera  = nullptr;

// Ping-pong FBO globals
GLuint pingPongFBO[2];
GLuint pingPongTex[2];
GLuint quadVAO = 0, quadVBO = 0;

// CHANGE 3: ImGui-controllable parameters
float g_lambda   = 0.03f;
int   g_numIters = 6;

// -------------------------------------------------------
// Fullscreen quad helper
// -------------------------------------------------------
void renderQuad() {
    if (quadVAO == 0) {
        float quadVerts[] = {
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// -------------------------------------------------------
// Framebuffer resize callback
// -------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
}

// -------------------------------------------------------
// CHANGE 5: Extracted FBO setup into a reusable function
//           Uses RGBA16F for better precision across iterations
// -------------------------------------------------------
void setupPingPongFBOs(int width, int height) {
    if (pingPongFBO[0]) {
        glDeleteFramebuffers(2, pingPongFBO);
        glDeleteTextures(2, pingPongTex);
    }

    for (int i = 0; i < 2; i++) {
        glGenFramebuffers(1, &pingPongFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[i]);

        glGenTextures(1, &pingPongTex[i]);
        glBindTexture(GL_TEXTURE_2D, pingPongTex[i]);
        // CHANGE 5: RGBA16F instead of RGB UNSIGNED_BYTE
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingPongTex[i], 0);

        GLuint rbo;
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "Ping-pong FBO[" << i << "] incomplete!" << std::endl;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// -------------------------------------------------------
// Main
// -------------------------------------------------------
int main() {
    WindowMaker wm(1080, 1920);
    GLFWwindow *window = wm.make_window("RTR - Recursive Camera Painting");
    gWindow = window;

    glfwMakeContextCurrent(window);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    // Initialise to 0 before setup
    pingPongFBO[0] = pingPongFBO[1] = 0;
    pingPongTex[0] = pingPongTex[1] = 0;
    setupPingPongFBOs(fbWidth, fbHeight);

    // -------------------------------------------------------
    // Camera & ImGui setup
    // -------------------------------------------------------
    Camera camera(window);
    camera.mouseActive = false;
    gCamera = &camera;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // -------------------------------------------------------
    // Assets & shaders
    // -------------------------------------------------------
    ModelClass suzzane("./suzzane/test_low.obj");
    ModelClass church("./old_church_modeling_-_interior_scene/old_church_modeling_-_interior_scene.obj");
    Shader object_shader("./basic.vert", "./basic.frag");
    Shader feedbackShader("./feedback.vert", "./feedback.frag");

    // -------------------------------------------------------
    // Render loop
    // -------------------------------------------------------
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime    = currentFrame - lastFrame;
        lastFrame          = currentFrame;

        ImGuiIO &io = ImGui::GetIO();

        // --- Input ---
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS &&
            !io.WantCaptureMouse) {
            if (!cameraActive) {
                cameraActive       = true;
                camera.mouseActive = cameraActive;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && cameraActive) {
            cameraActive       = false;
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

        // --- Resize check ---
        int newW, newH;
        glfwGetFramebufferSize(window, &newW, &newH);
        if (newW != fbWidth || newH != fbHeight) {
            fbWidth  = newW;
            fbHeight = newH;
            if (fbHeight == 0) fbHeight = 1;
            glViewport(0, 0, fbWidth, fbHeight);
            setupPingPongFBOs(fbWidth, fbHeight);
        }

        // --- ImGui ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // CHANGE 3: Paper parameter controls
        ImGui::Begin("Recursive Camera Painting", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Paper: Akleman et al. 2023");
        ImGui::Separator();
        ImGui::SliderFloat("Lambda (strength)", &g_lambda,   0.001f, 0.15f);
        ImGui::SliderInt("Iterations",          &g_numIters, 1, 10);
        ImGui::Separator();
        ImGui::Text("LMB: activate camera | ESC: release");
        ImGui::Text("Move: WASD");
        ImGui::End();

        // --- Matrices ---
        float aspect     = static_cast<float>(fbWidth) / static_cast<float>(fbHeight);
        glm::mat4 view   = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspect, 0.1f, 10000.0f);
        glm::mat4 modelMat   = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 1.0f));

        // -------------------------------------------------------
        // CHANGE 2: Three-pass pipeline
        //   Pass A: Render scene → FBO 0
        //   Pass B: N recursive feedback iterations (ping-pong)
        //   Pass C: Blit final result to screen
        // -------------------------------------------------------

        // --- Pass A: Render scene into FBO 0 ---
        glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[0]);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        // CHANGE 4: draw full scene (not just emissive)
        object_shader.use();
        object_shader.setMat4("projection", projection);
        object_shader.setMat4("view",       view);
        object_shader.setMat4("model",      modelMat);
        //suzzane.drawModel(object_shader.ID);
        church.drawModel(object_shader.ID);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --- Pass B: Recursive feedback iterations ---
        int readIdx = 0;
        glDisable(GL_DEPTH_TEST);

        for (int i = 0; i < g_numIters; i++) {
            int writeIdx = 1 - readIdx;

            glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[writeIdx]);
            glViewport(0, 0, fbWidth, fbHeight);
            glClear(GL_COLOR_BUFFER_BIT);

            // CHANGE 1: per-pixel color-driven displacement, no global offset
            feedbackShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, pingPongTex[readIdx]);
            feedbackShader.setInt("u_prevFrame", 0);
            feedbackShader.setFloat("u_lambda",  g_lambda);
            renderQuad();

            readIdx = writeIdx;
        }

        glEnable(GL_DEPTH_TEST);

        // --- Pass C: Blit final result to screen ---
        glBindFramebuffer(GL_READ_FRAMEBUFFER, pingPongFBO[readIdx]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, fbWidth, fbHeight,
                          0, 0, fbWidth, fbHeight,
                          GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --- ImGui render (always to default framebuffer) ---
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // -------------------------------------------------------
    // Cleanup
    // -------------------------------------------------------
    glDeleteFramebuffers(2, pingPongFBO);
    glDeleteTextures(2, pingPongTex);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
