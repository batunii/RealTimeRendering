#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>

#include "../external/imgui/backends/imgui_impl_glfw.h"
#include "../external/imgui/backends/imgui_impl_opengl3.h"
#include "../external/imgui/imgui.h"

#include "Camera.hpp"
#include "Models.hpp"
#include "Shaders.hpp"
#include "Texture.hpp"
#include "WindowMaker.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

// -------------------------------------------------------
// Globals
// -------------------------------------------------------
float lastFrame   = 0.0f;
bool cameraActive = false;
GLFWwindow* gWindow = nullptr;
Camera*     gCamera = nullptr;

// Scene FBO (with depth) — Pass A renders here
GLuint sceneFBO = 0, sceneTex = 0;
GLuint sceneRBO = 0;

// Feedback FBOs (no depth) — used for ping-pong passes
GLuint pingPongFBO[2] = {0, 0};
GLuint pingPongTex[2] = {0, 0};

GLuint quadVAO = 0, quadVBO = 0;

// Style params
float g_lambda       = 0.03f;
int   g_numIters     = 6;
float g_angle        = 0.0f;
float g_quantSteps   = 8.0f;
float g_quantStrength= 0.0f;
float g_edgeStrength = 1.0f;
float g_lumaStrength = 1.0f;
int   g_stylePreset  = 0;  // 0 = Normal (passthrough)
bool g_flipuvs = false;
// -------------------------------------------------------
// Fullscreen quad
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
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
        glBindVertexArray(0);
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);
}

// -------------------------------------------------------
// FBO setup — scene FBO with depth, feedback FBOs without
// -------------------------------------------------------
void setupFBOs(int width, int height) {
    // --- Cleanup ---
    if (sceneFBO) {
        glDeleteFramebuffers(1, &sceneFBO);
        glDeleteTextures(1, &sceneTex);
        glDeleteRenderbuffers(1, &sceneRBO);
    }
    if (pingPongFBO[0]) {
        glDeleteFramebuffers(2, pingPongFBO);
        glDeleteTextures(2, pingPongTex);
    }

    // --- Scene FBO: RGB16F color + depth renderbuffer ---
    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    glGenTextures(1, &sceneTex);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTex, 0);

    glGenRenderbuffers(1, &sceneRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, sceneRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, sceneRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "sceneFBO incomplete!\n";

    // --- Feedback FBOs: RGB16F color only, NO depth (saves bandwidth) ---
    for (int i = 0; i < 2; i++) {
        glGenFramebuffers(1, &pingPongFBO[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[i]);

        glGenTextures(1, &pingPongTex[i]);
        glBindTexture(GL_TEXTURE_2D, pingPongTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingPongTex[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "pingPongFBO[" << i << "] incomplete!\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// -------------------------------------------------------
// Preset loader — preset 0 = Normal (no feedback)
// -------------------------------------------------------
void applyPreset(int preset) {
    switch (preset) {
        case 0: // Normal — pure passthrough, no painting
            g_lambda        = 0.0f;
            g_numIters      = 1;
            g_angle         = 0.0f;
            g_quantSteps    = 16.0f;
            g_quantStrength = 0.0f;
            g_edgeStrength  = 0.0f;
            g_lumaStrength  = 0.0f;
            break;
        case 1: // Base Paper
            g_lambda        = 0.03f;
            g_numIters      = 6;
            g_angle         = 0.0f;
            g_quantSteps    = 8.0f;
            g_quantStrength = 0.0f;
            g_edgeStrength  = 1.0f;
            g_lumaStrength  = 1.0f;
            break;
        case 2: // Van Gogh
            g_lambda        = 0.05f;
            g_numIters      = 6;
            g_angle         = 135.0f;
            g_quantSteps    = 12.0f;
            g_quantStrength = 0.1f;
            g_edgeStrength  = 1.2f;
            g_lumaStrength  = 1.4f;
            break;
        case 3: // Watercolour
            g_lambda        = 0.025f;
            g_numIters      = 5;
            g_angle         = 90.0f;
            g_quantSteps    = 6.0f;
            g_quantStrength = 0.25f;
            g_edgeStrength  = 0.6f;
            g_lumaStrength  = 1.8f;
            break;
        case 4: // Cubist
            g_lambda        = 0.04f;
            g_numIters      = 7;
            g_angle         = 45.0f;
            g_quantSteps    = 4.0f;
            g_quantStrength = 0.55f;
            g_edgeStrength  = 1.5f;
            g_lumaStrength  = 0.8f;
            break;
        default: break;
    }
}

// -------------------------------------------------------
// Main
// -------------------------------------------------------
int main() {
    WindowMaker wm(1080, 1920);
    GLFWwindow* window = wm.make_window("RTR - Recursive Camera Painting NPR");
    gWindow = window;

    glfwMakeContextCurrent(window);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
    setupFBOs(fbWidth, fbHeight);

    Camera camera(window);
    camera.mouseActive = false;
    gCamera = &camera;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ModelClass suzzane("./suzzane/test_low.obj");
    // ModelClass church("./old_church_modeling_-_interior_scene/old_church_modeling_-_interior_scene.obj");
    ModelClass city("./building_pack/model.obj");

    Shader object_shader("./basic.vert",    "./basic.frag");
    Shader feedbackShader("./feedback.vert","./feedback.frag");

    const char* styles[] = {
        "Normal (no effect)",
        "Base Paper",
        "Van Gogh",
        "Watercolour",
        "Cubist"
    };

    // -------------------------------------------------------
    // Render loop
    // -------------------------------------------------------
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime    = currentFrame - lastFrame;
        lastFrame          = currentFrame;

        ImGuiIO& io = ImGui::GetIO();

        // --- Input ---
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !io.WantCaptureMouse) {
            if (!cameraActive) {
                cameraActive       = true;
                camera.mouseActive = true;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && cameraActive) {
            cameraActive       = false;
            camera.mouseActive = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_W, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_S, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_A, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_D, deltaTime);

        // --- Resize check ---
        int newW, newH;
        glfwGetFramebufferSize(window, &newW, &newH);
        if (newW != fbWidth || newH != fbHeight) {
            fbWidth  = newW;
            fbHeight = (newH == 0) ? 1 : newH;
            glViewport(0, 0, fbWidth, fbHeight);
            setupFBOs(fbWidth, fbHeight);
        }

        // --- ImGui ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Recursive Camera Painting NPR", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Checkbox("FlipUVs", &g_flipuvs);
        if (ImGui::Combo("Style Preset", &g_stylePreset, styles, IM_ARRAYSIZE(styles)))
            applyPreset(g_stylePreset);

        ImGui::Separator();
        // Hide painting controls when in Normal mode
        if (g_stylePreset != 0) {
            ImGui::SliderFloat("Lambda",         &g_lambda,        0.001f, 0.15f);
            ImGui::SliderInt("Iterations",       &g_numIters,      1,      10);
            ImGui::SliderFloat("Angle",          &g_angle,         0.0f,   360.0f);
            ImGui::SliderFloat("Edge Strength",  &g_edgeStrength,  0.0f,   3.0f);
            ImGui::SliderFloat("Luma Strength",  &g_lumaStrength,  0.0f,   3.0f);
            ImGui::SliderFloat("Quant Steps",    &g_quantSteps,    2.0f,   16.0f);
            ImGui::SliderFloat("Quant Strength", &g_quantStrength, 0.0f,   1.0f);
        } else {
            ImGui::TextDisabled("No painting parameters in Normal mode.");
        }
        ImGui::Separator();
        ImGui::Text("LMB: activate camera | ESC: release");
        ImGui::Text("WASD to move");
        ImGui::End();

        // --- Matrices ---
        float aspect = static_cast<float>(fbWidth) / static_cast<float>(fbHeight);
        glm::mat4 view       = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspect, 0.1f, 10000.0f);
        glm::mat4 modelMat   = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 1.0f));

        // ===================================================
        // Pass A: Render scene into sceneFBO (has depth)
        // ===================================================
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        object_shader.use();
        object_shader.setMat4("projection", projection);
        object_shader.setMat4("view",       view);
        object_shader.setMat4("model",      modelMat);
	object_shader.setBool("flipuvs", g_flipuvs); 
        // suzzane.drawModel(object_shader.ID);
        // church.drawModel(object_shader.ID);
        city.drawModel(object_shader.ID);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);

        // ===================================================
        // Normal mode: blit scene directly to screen, skip all feedback
        // ===================================================
        if (g_stylePreset == 0) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, fbWidth, fbHeight,
                              0, 0, fbWidth, fbHeight,
                              GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        else {
            // ===================================================
            // Pass B: Copy scene into pingPong[0] as starting point
            // ===================================================
            glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pingPongFBO[0]);
            glBlitFramebuffer(0, 0, fbWidth, fbHeight,
                              0, 0, fbWidth, fbHeight,
                              GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // ===================================================
            // Pass C: Recursive feedback — ping-pong N iterations
            // ===================================================
            int readIdx = 0;
            for (int i = 0; i < g_numIters; i++) {
                int writeIdx = 1 - readIdx;

                glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[writeIdx]);
                glViewport(0, 0, fbWidth, fbHeight);
                glClear(GL_COLOR_BUFFER_BIT);

                feedbackShader.use();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, pingPongTex[readIdx]);
                feedbackShader.setInt  ("u_prevFrame",     0);
                feedbackShader.setFloat("u_lambda",        g_lambda);
                feedbackShader.setFloat("u_angle",         glm::radians(g_angle));
                feedbackShader.setVec2 ("u_texelSize",     glm::vec2(1.0f/fbWidth, 1.0f/fbHeight));
                feedbackShader.setFloat("u_edgeStrength",  g_edgeStrength);
                feedbackShader.setFloat("u_lumaStrength",  g_lumaStrength);
                feedbackShader.setFloat("u_quantSteps",    g_quantSteps);
                feedbackShader.setFloat("u_quantStrength", g_quantStrength);
                renderQuad();

                readIdx = writeIdx;
            }

            // ===================================================
            // Pass D: Blit final feedback result to screen
            // ===================================================
            glBindFramebuffer(GL_READ_FRAMEBUFFER, pingPongFBO[readIdx]);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0, 0, fbWidth, fbHeight,
                              0, 0, fbWidth, fbHeight,
                              GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        glEnable(GL_DEPTH_TEST);

        // --- ImGui on top of everything ---
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // -------------------------------------------------------
    // Cleanup
    // -------------------------------------------------------
    glDeleteFramebuffers(1, &sceneFBO);
    glDeleteTextures(1,    &sceneTex);
    glDeleteRenderbuffers(1, &sceneRBO);
    glDeleteFramebuffers(2, pingPongFBO);
    glDeleteTextures(2,    pingPongTex);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1,     &quadVBO);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}
