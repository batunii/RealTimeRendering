#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_transform.hpp>
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

float lastFrame    = 0.0f;
bool  cameraActive = false;
GLFWwindow* gWindow = nullptr;
Camera*     gCamera = nullptr;

// G-buffer FBO: MRT color + normals + depth texture
GLuint sceneFBO  = 0;
GLuint sceneTex  = 0;
GLuint normalTex = 0;
GLuint depthTex  = 0;

// Feedback FBOs
GLuint pingPongFBO[2] = {0, 0};
GLuint pingPongTex[2] = {0, 0};

GLuint quadVAO = 0, quadVBO = 0;

int g_model = 0;
// Style params
float g_lambda        = 0.03f;
int   g_numIters      = 6;
float g_angle         = 0.0f;
float g_quantSteps    = 8.0f;
float g_quantStrength = 0.0f;
float g_edgeStrength  = 1.0f;
float g_lumaStrength  = 1.0f;
int   g_stylePreset   = 0;

// Depth smudging
bool  g_useDepth      = false;
float g_depthStrength = 1.0f;
int   g_depthMode     = 1;

// Normal-driven strokes
bool  g_useNormals     = false;
float g_normalStrength = 1.0f;
int   g_normalMode     = 1;

void renderQuad() {
    if (quadVAO == 0) {
        float quadVerts[] = {
            -1.0f,  1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
            -1.0f,  1.0f, 0.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 0.0f,
             1.0f,  1.0f, 1.0f, 1.0f
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

void setupFBOs(int width, int height) {
    if (sceneFBO) {
        glDeleteFramebuffers(1, &sceneFBO);
        glDeleteTextures(1, &sceneTex);
        glDeleteTextures(1, &normalTex);
        glDeleteTextures(1, &depthTex);
    }
    if (pingPongFBO[0]) {
        glDeleteFramebuffers(2, pingPongFBO);
        glDeleteTextures(2, pingPongTex);
    }

    // --- Scene G-buffer FBO ---
    glGenFramebuffers(1, &sceneFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

    // Attachment 0: emissive color
    glGenTextures(1, &sceneTex);
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTex, 0);

    // Attachment 1: view-space normals
    glGenTextures(1, &normalTex);
    glBindTexture(GL_TEXTURE_2D, normalTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normalTex, 0);

    // Depth as texture
    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);

    GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "sceneFBO incomplete!\n";

    // --- Feedback FBOs ---
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
        GLenum buf = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &buf);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "pingPongFBO[" << i << "] incomplete!\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void applyPreset(int preset) {
    g_useDepth   = false;
    g_useNormals = false;
    switch (preset) {
        case 0:
            g_lambda=0.0f; g_numIters=1; g_angle=0.0f;
            g_quantSteps=16.0f; g_quantStrength=0.0f;
            g_edgeStrength=0.0f; g_lumaStrength=0.0f; break;
        case 1:
            g_lambda=0.03f; g_numIters=6; g_angle=0.0f;
            g_quantSteps=8.0f; g_quantStrength=0.0f;
            g_edgeStrength=1.0f; g_lumaStrength=1.0f; break;
        case 2: // Van Gogh
            g_lambda=0.05f; g_numIters=6; g_angle=135.0f;
            g_quantSteps=12.0f; g_quantStrength=0.1f;
            g_edgeStrength=1.2f; g_lumaStrength=1.4f; break;
        case 3: // Watercolour
            g_lambda=0.025f; g_numIters=5; g_angle=90.0f;
            g_quantSteps=6.0f; g_quantStrength=0.25f;
            g_edgeStrength=0.6f; g_lumaStrength=1.8f; break;
        case 4: // Cubist
            g_lambda=0.04f; g_numIters=7; g_angle=45.0f;
            g_quantSteps=4.0f; g_quantStrength=0.55f;
            g_edgeStrength=1.5f; g_lumaStrength=0.8f; break;
        case 5: // Depth Painting
            g_lambda=0.04f; g_numIters=6; g_angle=0.0f;
            g_quantSteps=8.0f; g_quantStrength=0.0f;
            g_edgeStrength=1.0f; g_lumaStrength=1.0f;
            g_useDepth=true; g_depthStrength=1.5f; g_depthMode=1; break;
        case 6: // Normal Strokes
            g_lambda=0.035f; g_numIters=5; g_angle=90.0f;
            g_quantSteps=8.0f; g_quantStrength=0.0f;
            g_edgeStrength=1.0f; g_lumaStrength=1.0f;
            g_useNormals=true; g_normalStrength=1.2f; g_normalMode=1; break;
        default: break;
    }
}

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

    ModelClass suzzane("./suzzane/test_low.obj");
    ModelClass church("./old_church_modeling_-_interior_scene/old_church_modeling_-_interior_scene.obj");
    ModelClass city("./building_pack/model.obj");

    Shader scene_shader("./basic.vert",     "./basic.frag");
    Shader feedbackShader("./feedback.vert","./feedback.frag");

    const char *models[] = {"church", "city", "suzzane"}; 
        const char *styles[] = {
        "Normal (no effect)", "Base Paper", "Van Gogh",
        "Watercolour",        "Cubist",     "Depth Painting",
        "Normal Strokes"};    
    const char* depthModes[]  = { "Near smears", "Far smears", "Mid-focus sharp" };
    const char* normalModes[] = { "Normals only", "Color + Normal blend", "Auto toggle" };

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime    = currentFrame - lastFrame;
        lastFrame          = currentFrame;

        ImGuiIO& io = ImGui::GetIO();
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS && !io.WantCaptureMouse) {
            if (!cameraActive) {
                cameraActive = true;
                camera.mouseActive = true;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS && cameraActive) {
            cameraActive = false;
            camera.mouseActive = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_W, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_S, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_A, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_D, deltaTime);

        int newW, newH;
        glfwGetFramebufferSize(window, &newW, &newH);
        if (newW != fbWidth || newH != fbHeight) {
            fbWidth  = newW;
            fbHeight = (newH == 0) ? 1 : newH;
            glViewport(0, 0, fbWidth, fbHeight);
            setupFBOs(fbWidth, fbHeight);
        }

        // ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Recursive Camera Painting NPR", nullptr,
                     ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Combo("Model Select", &g_model, models, IM_ARRAYSIZE(models));        
        if (ImGui::Combo("Style Preset", &g_stylePreset, styles, IM_ARRAYSIZE(styles)))
            applyPreset(g_stylePreset);

        ImGui::Separator();
        if (g_stylePreset != 0) {
            ImGui::SliderFloat("Lambda",         &g_lambda,        0.001f, 0.15f);
            ImGui::SliderInt  ("Iterations",     &g_numIters,      1,      10);
            ImGui::SliderFloat("Angle",          &g_angle,         0.0f,   360.0f);
            ImGui::SliderFloat("Edge Strength",  &g_edgeStrength,  0.0f,   3.0f);
            ImGui::SliderFloat("Luma Strength",  &g_lumaStrength,  0.0f,   3.0f);
            ImGui::SliderFloat("Quant Steps",    &g_quantSteps,    2.0f,   16.0f);
            ImGui::SliderFloat("Quant Strength", &g_quantStrength, 0.0f,   1.0f);

            ImGui::Separator();
            ImGui::Text("--- Depth Smudging ---");
            ImGui::Checkbox("Enable Depth Smudging", &g_useDepth);
            if (g_useDepth) {
                ImGui::Combo("Depth Mode",     &g_depthMode,     depthModes,  IM_ARRAYSIZE(depthModes));
                ImGui::SliderFloat("Depth Strength", &g_depthStrength, 0.0f, 3.0f);
            }

            ImGui::Separator();
            ImGui::Text("--- Normal Strokes ---");
            ImGui::Checkbox("Enable Normal Strokes", &g_useNormals);
            if (g_useNormals) {
                ImGui::Combo("Normal Mode",     &g_normalMode,     normalModes, IM_ARRAYSIZE(normalModes));
                ImGui::SliderFloat("Normal Strength", &g_normalStrength, 0.0f, 3.0f);
            }
        } else {
            ImGui::TextDisabled("No parameters in Normal mode.");
        }

        ImGui::Separator();
        ImGui::Text("LMB: activate camera | ESC: release");
        ImGui::End();

        // Matrices
        float aspect         = static_cast<float>(fbWidth) / static_cast<float>(fbHeight);
        glm::mat4 view       = camera.getViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspect, 0.1f, 10000.0f);
        glm::mat4 modelMat   = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 1.0f));
        glm::mat4 normalMat  = glm::transpose(glm::inverse(view * modelMat));

        // ===========================================
        // Pass A: G-buffer render (MRT)
        //   attachment 0 -> sceneTex  (color)
        //   attachment 1 -> normalTex (view normals)
        //   depth        -> depthTex
        // ===========================================
        glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        scene_shader.use();
        scene_shader.setMat4("projection", projection);
        scene_shader.setMat4("view",       view);
        scene_shader.setMat4("model",      modelMat);
        scene_shader.setMat4("normalMat",  normalMat);
        scene_shader.setBool("flipuvs",    true);

        switch (g_model) {
        case 0:
          church.drawModel(scene_shader.ID);
          break;
        case 1:
	  modelMat  = glm::scale(modelMat, glm::vec3(0.1, 0.1, 0.1));
          scene_shader.setMat4("model",      modelMat);
          city.drawModel(scene_shader.ID);
          break;
        case 2:
          suzzane.drawModel(scene_shader.ID);
          break;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);

        if (g_stylePreset == 0) {
            // No effect: blit color directly
            glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneFBO);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0,0,fbWidth,fbHeight, 0,0,fbWidth,fbHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        } else {
            // ===========================================
            // Pass B: Copy sceneTex -> pingPong[0]
            // ===========================================
            glBindFramebuffer(GL_READ_FRAMEBUFFER, sceneFBO);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pingPongFBO[0]);
            glBlitFramebuffer(0,0,fbWidth,fbHeight, 0,0,fbWidth,fbHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // ===========================================
            // Pass C: Recursive feedback iterations
            // ===========================================
            int readIdx = 0;
            for (int i = 0; i < g_numIters; i++) {
                int writeIdx = 1 - readIdx;

                glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[writeIdx]);
                glViewport(0, 0, fbWidth, fbHeight);
                glClear(GL_COLOR_BUFFER_BIT);

                feedbackShader.use();

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, pingPongTex[readIdx]);
                feedbackShader.setInt("u_prevFrame", 0);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, depthTex);
                feedbackShader.setInt("u_depthTex", 1);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, normalTex);
                feedbackShader.setInt("u_normalTex", 2);

                feedbackShader.setFloat("u_lambda",        g_lambda);
                feedbackShader.setFloat("u_angle",         glm::radians(g_angle));
                feedbackShader.setVec2 ("u_texelSize",     glm::vec2(1.0f/fbWidth, 1.0f/fbHeight));
                feedbackShader.setFloat("u_edgeStrength",  g_edgeStrength);
                feedbackShader.setFloat("u_lumaStrength",  g_lumaStrength);
                feedbackShader.setFloat("u_quantSteps",    g_quantSteps);
                feedbackShader.setFloat("u_quantStrength", g_quantStrength);

                feedbackShader.setInt  ("u_useDepth",      g_useDepth ? 1 : 0);
                feedbackShader.setFloat("u_depthStrength", g_depthStrength);
                feedbackShader.setInt  ("u_depthMode",     g_depthMode);

                feedbackShader.setInt  ("u_useNormals",    g_useNormals ? 1 : 0);
                feedbackShader.setFloat("u_normalStrength",g_normalStrength);
                feedbackShader.setInt  ("u_normalMode",    g_normalMode);

                renderQuad();
                readIdx = writeIdx;
            }

            // ===========================================
            // Pass D: Blit final result to screen
            // ===========================================
            glBindFramebuffer(GL_READ_FRAMEBUFFER, pingPongFBO[readIdx]);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBlitFramebuffer(0,0,fbWidth,fbHeight, 0,0,fbWidth,fbHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        glEnable(GL_DEPTH_TEST);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteFramebuffers(1, &sceneFBO);
    glDeleteTextures(1, &sceneTex);
    glDeleteTextures(1, &normalTex);
    glDeleteTextures(1, &depthTex);
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
