#include "gl_renderer.h"
#include "asset_manager.h"
#include <android/log.h>
#include <android/native_window.h>
#include <cmath>
#include <algorithm>

namespace LostDungeons {

    // --- Smoothing Variables ---
    static glm::vec3 targetCameraPos = glm::vec3(0.0f, 1.7f, 5.0f);
    static glm::vec3 currentCameraPos = glm::vec3(0.0f, 1.7f, 5.0f);
    static glm::vec3 cameraVelocity = glm::vec3(0.0f);
    static const float cameraSmoothTime = 0.1f;
    static const float maxCameraSpeed = 5.0f;

    // --- Existing Variables ---
    std::atomic<bool> GLRenderer::isRendering{false};
    std::thread GLRenderer::renderThread;
    std::mutex GLRenderer::stateMutex;

    EGLDisplay GLRenderer::display = EGL_NO_DISPLAY;
    EGLSurface GLRenderer::surface = EGL_NO_SURFACE;
    EGLContext GLRenderer::context = EGL_NO_CONTEXT;
    GLuint GLRenderer::shaderProgram = 0;
    GLuint GLRenderer::uiShaderProgram = 0;

    GLint GLRenderer::mvpLoc = -1, GLRenderer::modelLoc = -1, GLRenderer::timeLoc = -1;
    GLint GLRenderer::camPosLoc = -1, GLRenderer::lightDirLoc = -1, GLRenderer::lightColLoc = -1;
    GLint GLRenderer::ambColLoc = -1, GLRenderer::skyTopLoc = -1, GLRenderer::skyBotLoc = -1;
    GLint GLRenderer::uiMvpLoc = -1;

    int GLRenderer::width = 0;
    int GLRenderer::height = 0;

    glm::vec3 GLRenderer::cameraPos = glm::vec3(0.0f, 1.7f, 5.0f);
    glm::vec3 GLRenderer::cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 GLRenderer::cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    float GLRenderer::yaw = -90.0f;
    float GLRenderer::pitch = 0.0f;

    // --- Rest of your existing variables ---
    // (Keep all other existing variables as-is)

    void GLRenderer::updateInput(float moveX, float moveY, float lookX, float lookY) {
        std::lock_guard<std::mutex> lock(stateMutex);
        if (currentState != RenderState::OVERWORLD) return;

        // --- Smooth Camera Rotation ---
        yaw += lookX * 0.15f;
        pitch -= lookY * 0.15f;
        pitch = std::clamp(pitch, -89.0f, 89.0f);

        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        cameraFront = glm::normalize(front);

        // --- Smooth Movement with Delta Time ---
        glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
        glm::vec3 flatFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));

        // Apply movement input to target position
        targetCameraPos += flatFront * moveY * 0.1f;
        targetCameraPos += right * moveX * 0.1f;

        // Ensure target Y is aligned with terrain
        targetCameraPos.y = getTerrainHeight(targetCameraPos.x, targetCameraPos.z) + 1.7f;
    }

    void GLRenderer::drawFrame() {
        if (!shaderProgram) return;

        auto now = std::chrono::steady_clock::now();
        float engineTime = std::chrono::duration<float>(now - startTime).count();

        // --- Smooth Camera Position ---
        float deltaTime = std::min(0.1f, std::chrono::duration<float>(now - std::chrono::time_point_cast<std::chrono::steady_clock::duration>(std::chrono::steady_clock::now() - std::chrono::milliseconds(16))).count());
        currentCameraPos = glm::mix(currentCameraPos, targetCameraPos, cameraSmoothTime * deltaTime * 60.0f);
        cameraPos = currentCameraPos;

        // --- Rest of your existing drawFrame logic ---
        // (Keep all other existing code as-is)
    }
}