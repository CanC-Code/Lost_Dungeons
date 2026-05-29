#include "gl_renderer.h"
#include "asset_manager.h"
#include <android/log.h>
#include <android/native_window.h>
#include <cmath>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace LostDungeons {

    // --- Rendering State ---
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

    // --- Camera State ---
    glm::vec3 GLRenderer::cameraPos = glm::vec3(0.0f, 1.7f, 5.0f);
    glm::vec3 GLRenderer::cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 GLRenderer::cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    float GLRenderer::yaw = -90.0f;
    float GLRenderer::pitch = 0.0f;

    // --- Smoothing Variables ---
    glm::vec3 GLRenderer::targetCameraPos = glm::vec3(0.0f, 1.7f, 5.0f);
    glm::vec3 GLRenderer::currentCameraPos = glm::vec3(0.0f, 1.7f, 5.0f);
    glm::vec3 GLRenderer::cameraVelocity = glm::vec3(0.0f);
    float GLRenderer::cameraSmoothTime = 0.1f;

    // --- Saved State ---
    glm::vec3 GLRenderer::savedOverworldPos = glm::vec3(0.0f, 1.7f, 5.0f);
    float GLRenderer::savedOverworldYaw = -90.0f;
    float GLRenderer::savedOverworldPitch = 0.0f;

    RenderState GLRenderer::currentState = RenderState::OVERWORLD;
    std::string GLRenderer::activeEntity = "";

    // --- Compass State ---
    bool GLRenderer::compassLockedToNorth = false;
    float GLRenderer::currentCompassAngle = 0.0f;
    float GLRenderer::compassVelocity = 0.0f;
    float GLRenderer::lastFrameTime = 0.0f;

    // --- Terrain State ---
    std::chrono::time_point<std::chrono::steady_clock> GLRenderer::startTime;
    std::vector<GLfloat> GLRenderer::terrainVertices;
    std::vector<GLuint> GLRenderer::terrainIndices;
    int GLRenderer::lastGridX = -9999;
    int GLRenderer::lastGridZ = -9999;

    // --- Compass Constants ---
    static constexpr float COMPASS_CX = 80.0f;
    static constexpr float COMPASS_CY = 80.0f;
    static constexpr float COMPASS_HIT_RADIUS = 72.0f;

    // --- Helper Functions ---
    void GLRenderer::toggleCompassMode() {
        compassLockedToNorth = !compassLockedToNorth;
    }

    void GLRenderer::handleTap(float screenX, float screenY) {
        float dx = screenX - COMPASS_CX;
        float dy = screenY - COMPASS_CY;
        if (std::sqrt(dx * dx + dy * dy) <= COMPASS_HIT_RADIUS) {
            toggleCompassMode();
        }
    }

    void GLRenderer::setGameState(int state, const std::string& entityId) {
        std::lock_guard<std::mutex> lock(stateMutex);
        RenderState newState = static_cast<RenderState>(state);

        if (newState == currentState) return;

        // Cache overworld position
        if (currentState == RenderState::OVERWORLD) {
            savedOverworldPos = cameraPos;
            savedOverworldYaw = yaw;
            savedOverworldPitch = pitch;
        }

        if (newState == RenderState::BATTLE) {
            cameraPos = glm::vec3(0.0f, getTerrainHeight(0.0f, 0.0f) + 1.5f, 4.0f);
            targetCameraPos = cameraPos;
            yaw = -90.0f;
            pitch = 0.0f;
            cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
        }
        else if (newState == RenderState::OVERWORLD) {
            cameraPos = savedOverworldPos;
            targetCameraPos = cameraPos;
            yaw = savedOverworldYaw;
            pitch = savedOverworldPitch;

            glm::vec3 front;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            cameraFront = glm::normalize(front);
        }
        else if (newState == RenderState::MENU) {
            // Freeze camera in overworld position
            cameraPos = savedOverworldPos;
            targetCameraPos = cameraPos;
            yaw = savedOverworldYaw;
            pitch = savedOverworldPitch;

            glm::vec3 front;
            front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
            front.y = sin(glm::radians(pitch));
            front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
            cameraFront = glm::normalize(front);
        }

        currentState = newState;
        activeEntity = entityId;
    }

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

        // --- Smooth Movement ---
        glm::vec3 right = glm::normalize(glm::cross(cameraFront, cameraUp));
        glm::vec3 flatFront = glm::normalize(glm::vec3(cameraFront.x, 0.0f, cameraFront.z));

        // Apply movement to target position (not directly to cameraPos)
        targetCameraPos += flatFront * moveY * 0.1f;
        targetCameraPos += right * moveX * 0.1f;

        // Ensure target Y is aligned with terrain
        targetCameraPos.y = getTerrainHeight(targetCameraPos.x, targetCameraPos.z) + 1.7f;
    }

    float GLRenderer::getTerrainHeight(float worldX, float worldZ) {
        float y = 0.0f;
        y += sin(worldX * 0.008f) * cos(worldZ * 0.008f) * 60.0f;
        y += sin(worldX * 0.03f + worldZ * 0.02f) * 12.0f;
        y += sin(worldX * 0.2f - worldZ * 0.2f) * 0.5f;
        if (y < 5.0f) {
            y = 5.0f + (y - 5.0f) * 0.15f;
        }
        return y;
    }

    void GLRenderer::updateTerrainMesh() {
        terrainVertices.clear();
        terrainIndices.clear();

        int gridSize = 240;
        float spacing = 2.5f;

        int camX = (int)floor(cameraPos.x);
        int camZ = (int)floor(cameraPos.z);

        for (int z = -gridSize / 2; z < gridSize / 2; ++z) {
            for (int x = -gridSize / 2; x < gridSize / 2; ++x) {
                float fx = x * spacing;
                float fz = z * spacing;

                float worldX = camX + fx;
                float worldZ = camZ + fz;
                float fy = getTerrainHeight(worldX, worldZ);

                float r, g, b;
                if (fy < 6.0f) { r = 0.2f; g = 0.5f; b = 0.2f; }
                else if (fy < 25.0f) { r = 0.45f; g = 0.35f; b = 0.2f; }
                else { r = 0.9f; g = 0.9f; b = 0.95f; }

                float colorNoise = (sin(worldX * 2.0f) * cos(worldZ * 2.0f)) * 0.05f;
                r += colorNoise; g += colorNoise; b += colorNoise;

                terrainVertices.insert(terrainVertices.end(), {
                    fx, fy, fz, r, g, b, 1.0f, 0.0f, 1.0f, 0.0f
                });
            }
        }

        for (int z = 0; z < gridSize - 1; ++z) {
            for (int x = 0; x < gridSize - 1; ++x) {
                int topLeft = (z * gridSize) + x;
                int topRight = topLeft + 1;
                int bottomLeft = ((z + 1) * gridSize) + x;
                int bottomRight = bottomLeft + 1;

                terrainIndices.insert(terrainIndices.end(), {
                    (GLuint)topLeft, (GLuint)bottomLeft, (GLuint)topRight,
                    (GLuint)topRight, (GLuint)bottomLeft, (GLuint)bottomRight
                });
            }
        }
    }

    // --- Rendering Loop ---
    void GLRenderer::start(ANativeWindow* window) {
        if (!window || isRendering) return;
        isRendering = true;
        startTime = std::chrono::steady_clock::now();
        renderThread = std::thread(renderLoop, window);
    }

    void GLRenderer::stop() {
        isRendering = false;
        if (renderThread.joinable()) renderThread.join();
    }

    void GLRenderer::setViewport(int w, int h) {
        width = w;
        height = h;
    }

    void GLRenderer::renderLoop(ANativeWindow* window) {
        if (!initEGL(window)) {
            __android_log_print(ANDROID_LOG_ERROR, "LostDungeonsGL", "EGL Init failed.");
            if (window) ANativeWindow_release(window);
            return;
        }
        setupGraphics();

        auto lastTime = std::chrono::steady_clock::now();
        while (isRendering) {
            auto currentTime = std::chrono::steady_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;

            // --- Smooth Camera Position ---
            if (currentState == RenderState::OVERWORLD || currentState == RenderState::MENU) {
                currentCameraPos = glm::mix(
                    currentCameraPos,
                    targetCameraPos,
                    cameraSmoothTime * 60.0f * deltaTime
                );
                cameraPos = currentCameraPos;
            }

            drawFrame();
            eglSwapBuffers(display, surface);
        }

        destroyEGL();
        if (window) ANativeWindow_release(window);
    }

    bool GLRenderer::initEGL(ANativeWindow* window) {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) return false;
        if (!eglInitialize(display, nullptr, nullptr)) return false;

        const EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 16,
            EGL_NONE
        };
        EGLint numConfigs = 0;
        EGLConfig config;
        eglChooseConfig(display, attribs, &config, 1, &numConfigs);
        if (numConfigs == 0) return false;

        const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
        surface = eglCreateWindowSurface(display, config, window, nullptr);
        return eglMakeCurrent(display, surface, surface, context) != EGL_FALSE;
    }

    void GLRenderer::destroyEGL() {
        if (display != EGL_NO_DISPLAY) {
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (context != EGL_NO_CONTEXT) { eglDestroyContext(display, context); context = EGL_NO_CONTEXT; }
            if (surface != EGL_NO_SURFACE) { eglDestroySurface(display, surface); surface = EGL_NO_SURFACE; }
            eglTerminate(display); display = EGL_NO_DISPLAY;
        }
        if (shaderProgram != 0) { glDeleteProgram(shaderProgram); shaderProgram = 0; }
        if (uiShaderProgram != 0) { glDeleteProgram(uiShaderProgram); uiShaderProgram = 0; }
    }

    GLuint GLRenderer::loadShader(GLenum type, const char* src) {
        if (!src || src[0] == '\0') return 0;
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        GLint compiled;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) { glDeleteShader(shader); return 0; }
        return shader;
    }

    void GLRenderer::setupGraphics() {
        std::string vertSrc = AssetManager::loadTextFile("shaders/base.vert");
        std::string fragSrc = AssetManager::loadTextFile("shaders/base.frag");
        if (vertSrc.empty() || fragSrc.empty()) return;

        GLuint vs = loadShader(GL_VERTEX_SHADER, vertSrc.c_str());
        GLuint fs = loadShader(GL_FRAGMENT_SHADER, fragSrc.c_str());

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vs);
        glAttachShader(shaderProgram, fs);
        glLinkProgram(shaderProgram);

        mvpLoc = glGetUniformLocation(shaderProgram, "u_MVP");
        modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
        timeLoc = glGetUniformLocation(shaderProgram, "u_Time");
        camPosLoc = glGetUniformLocation(shaderProgram, "u_CameraPos");
        lightDirLoc = glGetUniformLocation(shaderProgram, "u_LightDir");
        lightColLoc = glGetUniformLocation(shaderProgram, "u_LightColor");
        ambColLoc = glGetUniformLocation(shaderProgram, "u_AmbientColor");
        skyTopLoc = glGetUniformLocation(shaderProgram, "u_SkyColorTop");
        skyBotLoc = glGetUniformLocation(shaderProgram, "u_SkyColorBottom");

        // UI Shader Setup
        const char* uiVertSrc = R"(#version 300 es
            layout(location = 0) in vec2 a_Position;
            layout(location = 1) in vec4 a_Color;
            uniform mat4 u_MVP;
            out vec4 v_Color;
            void main() {
                gl_Position = u_MVP * vec4(a_Position, 0.0, 1.0);
                v_Color = a_Color;
            })";

        const char* uiFragSrc = R"(#version 300 es
            precision mediump float;
            in vec4 v_Color;
            out vec4 FragColor;
            void main() { FragColor = v_Color; })";

        GLuint uiVs = loadShader(GL_VERTEX_SHADER, uiVertSrc);
        GLuint uiFs = loadShader(GL_FRAGMENT_SHADER, uiFragSrc);

        uiShaderProgram = glCreateProgram();
        glAttachShader(uiShaderProgram, uiVs);
        glAttachShader(uiShaderProgram, uiFs);
        glLinkProgram(uiShaderProgram);

        uiMvpLoc = glGetUniformLocation(uiShaderProgram, "u_MVP");
        glEnable(GL_DEPTH_TEST);
    }

    void GLRenderer::drawFrame() {
        if (!shaderProgram) return;

        auto now = std::chrono::steady_clock::now();
        float engineTime = std::chrono::duration<float>(now - startTime).count();

        // Day/Night Cycle
        float timeOfDayAngle = (engineTime / 3000.0f) * 3.14159265f * 2.0f;
        float cycle = sin(timeOfDayAngle);
        bool isDay = cycle > 0.0f;

        glm::vec3 skyTop = isDay ? glm::vec3(0.1f, 0.4f, 0.8f) : glm::vec3(0.01f, 0.02f, 0.1f);
        glm::vec3 skyBot = isDay ? glm::vec3(0.6f, 0.8f, 0.9f) : glm::vec3(0.0f, 0.0f, 0.05f);
        glm::vec3 lightDir = glm::vec3(cos(timeOfDayAngle), sin(timeOfDayAngle), 0.5f);
        glm::vec3 lightCol = isDay ? glm::vec3(1.0f, 0.9f, 0.8f) : glm::vec3(0.2f, 0.3f, 0.6f);
        glm::vec3 ambCol = isDay ? glm::vec3(0.3f, 0.3f, 0.3f) : glm::vec3(0.05f, 0.05f, 0.1f);

        glViewport(0, 0, width, height);
        glClearColor(skyBot.r, skyBot.g, skyBot.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        std::lock_guard<std::mutex> lock(stateMutex);

        // Update terrain mesh if camera moved
        int currentGridX = (int)floor(cameraPos.x);
        int currentGridZ = (int)floor(cameraPos.z);
        if (currentGridX != lastGridX || currentGridZ != lastGridZ) {
            updateTerrainMesh();
            lastGridX = currentGridX;
            lastGridZ = currentGridZ;
        }

        // Set uniforms
        glUniform1f(timeLoc, engineTime);
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));
        glUniform3fv(lightColLoc, 1, glm::value_ptr(lightCol));
        glUniform3fv(ambColLoc, 1, glm::value_ptr(ambCol));
        glUniform3fv(skyTopLoc, 1, glm::value_ptr(skyTop));
        glUniform3fv(skyBotLoc, 1, glm::value_ptr(skyBot));
        glUniform3fv(camPosLoc, 1, glm::value_ptr(cameraPos));

        // Projection and view matrices
        float aspect = (float)width / (float)(height > 0 ? height : 1);
        glm::mat4 projection = glm::perspective(glm::radians(65.0f), aspect, 0.1f, 1000.0f);

        glm::vec3 lookTarget = (currentState == RenderState::OVERWORLD || currentState == RenderState::MENU)
            ? (cameraPos + cameraFront)
            : glm::vec3(0.0f, getTerrainHeight(0.f, 0.f) + 0.5f, 0.0f);

        glm::mat4 view = glm::lookAt(cameraPos, lookTarget, cameraUp);
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 mvp = projection * view * model;

        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // Draw scene
        if (currentState == RenderState::OVERWORLD || currentState == RenderState::MENU) {
            drawOverworldFloor();
            drawCompassHUD(engineTime);
        } else {
            drawEntityCube();
        }

        // Draw menu overlay if in MENU state
        if (currentState == RenderState::MENU) {
            drawMenuOverlay();
        }
    }

    void GLRenderer::drawOverworldFloor() {
        if (terrainVertices.empty()) return;
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), terrainVertices.data());
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), terrainVertices.data() + 3);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), terrainVertices.data() + 7);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3((float)lastGridX, 0.0f, (float)lastGridZ));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glDrawElements(GL_TRIANGLES, (GLsizei)terrainIndices.size(), GL_UNSIGNED_INT, terrainIndices.data());
    }

    void GLRenderer::drawEntityCube() {
        float r = 0.8f, g = 0.2f, b = 0.2f;
        if (activeEntity == "Slime") { r = 0.2f; g = 0.2f; b = 0.8f; }

        GLfloat vertices[] = {
            -0.5f, 0.0f, 0.5f, r, g, b, 1.0f, -0.5f, 0.5f, 0.5f,
            0.5f, 0.0f, 0.5f, r, g, b, 1.0f, 0.5f, 0.5f, 0.5f,
            0.0f, 1.0f, 0.0f, r + 0.2f, g, b, 1.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.0f, -0.5f, r - 0.2f, g, b, 1.0f, -0.5f, 0.5f, -0.5f,
            0.5f, 0.0f, -0.5f, r - 0.2f, g, b, 1.0f, 0.5f, 0.5f, -0.5f
        };
        GLuint indices[] = { 0, 1, 2, 1, 4, 2, 4, 3, 2, 3, 0, 2 };

        float entityY = getTerrainHeight(0.0f, 0.0f) + 0.0f;
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, entityY, 0.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), vertices);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), &vertices[3]);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), &vertices[7]);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, indices);
    }

    void GLRenderer::drawCompassHUD(float engineTime) {
        if (!uiShaderProgram) return;

        static std::vector<GLfloat> compVerts;
        static std::vector<GLuint> compIndices;
        static bool initCompassMesh = false;

        if (!initCompassMesh) {
            compVerts.insert(compVerts.end(), { 0.0f, 0.0f, 0.8f, 0.8f, 0.8f, 1.0f });

            const float PI = 3.14159265359f;
            int ringSegments = 36;
            float ringRadius = 1.0f;
            float ringInnerRadius = 0.85f;

            for (int i = 0; i < 16; ++i) {
                float angle = i * (PI * 2.0f / 16.0f) - (PI / 2.0f);
                float radius; float r, g, b;
                if (i == 0) { radius = 0.8f; r = 0.9f; g = 0.1f; b = 0.1f; }
                else if (i % 4 == 0) { radius = 0.8f; r = 0.8f; g = 0.8f; b = 0.8f; }
                else if (i % 2 == 0) { radius = 0.5f; r = 0.6f; g = 0.6f; b = 0.6f; }
                else { radius = 0.2f; r = 0.2f; g = 0.2f; b = 0.2f; }

                compVerts.insert(compVerts.end(), {
                    cos(angle) * radius, sin(angle) * radius, r, g, b, 1.0f
                });
            }
            for (int i = 1; i <= 16; ++i) {
                int next = (i == 16) ? 1 : i + 1;
                compIndices.insert(compIndices.end(), { 0u, (GLuint)i, (GLuint)next });
            }

            int ringStartIdx = (int)(compVerts.size() / 6);
            for (int i = 0; i <= ringSegments; ++i) {
                float angle = i * (PI * 2.0f / ringSegments);
                compVerts.insert(compVerts.end(), {
                    cos(angle) * ringInnerRadius, sin(angle) * ringInnerRadius, 0.6f, 0.5f, 0.1f, 1.0f
                });
                compVerts.insert(compVerts.end(), {
                    cos(angle) * ringRadius, sin(angle) * ringRadius, 0.9f, 0.8f, 0.2f, 1.0f
                });
            }
            for (int i = 0; i < ringSegments; ++i) {
                GLuint inner1 = ringStartIdx + (i * 2);
                GLuint outer1 = inner1 + 1;
                GLuint inner2 = inner1 + 2;
                GLuint outer2 = inner1 + 3;
                compIndices.insert(compIndices.end(), {
                    inner1, outer1, inner2, outer1, outer2, inner2
                });
            }
            initCompassMesh = true;
        }

        glDisable(GL_DEPTH_TEST);
        glUseProgram(uiShaderProgram);

        glm::mat4 projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
        glm::mat4 baseCompassModel = glm::translate(glm::mat4(1.0f),
            glm::vec3(COMPASS_CX, COMPASS_CY, 0.0f));

        float targetAngle = compassLockedToNorth ? 0.0f : glm::radians(yaw + 90.0f);
        float dt = engineTime - lastFrameTime;
        if (dt > 0.0f) {
            if (dt > 0.1f) dt = 0.1f;
            float diff = targetAngle - currentCompassAngle;
            while (diff > M_PI) diff -= 2.0f * M_PI;
            while (diff < -M_PI) diff += 2.0f * M_PI;
            float springTension = 30.0f, damping = 3.5f;
            compassVelocity += (diff * springTension - compassVelocity * damping) * dt;
            currentCompassAngle += compassVelocity * dt;
            lastFrameTime = engineTime;
        }

        float wobble = sin(engineTime * 35.0f) * 0.015f * std::min(std::abs(compassVelocity), 3.0f);
        float finalAngle = currentCompassAngle + wobble;

        // Draw needle and gold ring
        glm::mat4 needleModel = glm::rotate(baseCompassModel, finalAngle, glm::vec3(0.0f, 0.0f, 1.0f));
        needleModel = glm::scale(needleModel, glm::vec3(60.0f, 60.0f, 1.0f));
        glm::mat4 mvp = projection * needleModel;
        glUniformMatrix4fv(uiMvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), compVerts.data());
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), compVerts.data() + 2);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glDrawElements(GL_TRIANGLES, (GLsizei)compIndices.size(), GL_UNSIGNED_INT, compIndices.data());

        // Draw day/night clock diamond
        float sunX = cos(timeOfDayAngle) * 78.0f;
        float sunY = -sin(timeOfDayAngle) * 78.0f;
        glm::mat4 clockModel = glm::translate(baseCompassModel, glm::vec3(sunX, sunY, 0.0f));
        clockModel = glm::scale(clockModel, glm::vec3(12.0f, 12.0f, 1.0f));
        glm::mat4 clockMvp = projection * clockModel;
        glUniformMatrix4fv(uiMvpLoc, 1, GL_FALSE, glm::value_ptr(clockMvp));

        GLfloat clockVerts[] = {
            0.0f, -1.0f, 1.0f, 0.9f, 0.2f, 1.0f,
            -1.0f, 0.0f, 1.0f, 0.8f, 0.1f, 1.0f,
            1.0f, 0.0f, 1.0f, 0.8f, 0.1f, 1.0f,
            0.0f, 1.0f, 1.0f, 0.9f, 0.2f, 1.0f
        };
        GLuint clockInds[] = { 0, 1, 2, 1, 3, 2 };

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), clockVerts);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), clockVerts + 2);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, clockInds);

        glEnable(GL_DEPTH_TEST);
    }

    void GLRenderer::drawMenuOverlay() {
        if (!uiShaderProgram) return;

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(uiShaderProgram);

        glm::mat4 projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f, -1.0f, 1.0f);
        glUniformMatrix4fv(uiMvpLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // 70% opacity black quad to dim the 3D scene
        GLfloat vertices[] = {
            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f,
            (float)width, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f,
            0.0f, (float)height, 0.0f, 0.0f, 0.0f, 0.7f,
            (float)width, (float)height, 0.0f, 0.0f, 0.0f, 0.7f
        };
        GLuint indices[] = { 0, 1, 2, 1, 3, 2 };

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), vertices);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), vertices + 2);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }
}