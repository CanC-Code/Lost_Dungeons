#include "gl_renderer.h"
#include "asset_manager.h"
#include <android/log.h>
#include <cmath>

namespace LostDungeons {

    std::atomic<bool> GLRenderer::isRendering{false};
    std::thread GLRenderer::renderThread;
    std::mutex GLRenderer::stateMutex;
    
    EGLDisplay GLRenderer::display = EGL_NO_DISPLAY;
    EGLSurface GLRenderer::surface = EGL_NO_SURFACE;
    EGLContext GLRenderer::context = EGL_NO_CONTEXT;
    GLuint GLRenderer::shaderProgram = 0;
    
    GLint GLRenderer::mvpLoc = -1, GLRenderer::modelLoc = -1, GLRenderer::timeLoc = -1;
    GLint GLRenderer::lightDirLoc = -1, GLRenderer::lightColLoc = -1, GLRenderer::ambColLoc = -1;
    GLint GLRenderer::skyTopLoc = -1, GLRenderer::skyBotLoc = -1;
    
    int GLRenderer::width = 0;
    int GLRenderer::height = 0;

    glm::vec3 GLRenderer::cameraPos = glm::vec3(0.0f, 2.0f, 5.0f);
    RenderState GLRenderer::currentState = RenderState::OVERWORLD;
    std::string GLRenderer::activeEntity = "";
    
    std::chrono::time_point<std::chrono::steady_clock> GLRenderer::startTime;
    std::vector<GLfloat> GLRenderer::terrainVertices;
    std::vector<GLuint> GLRenderer::terrainIndices;

    void GLRenderer::setGameState(int state, const std::string& entityId) {
        std::lock_guard<std::mutex> lock(stateMutex);
        currentState = static_cast<RenderState>(state);
        activeEntity = entityId;
        
        if (currentState == RenderState::OVERWORLD) {
            cameraPos = glm::vec3(cameraPos.x, 2.0f, cameraPos.z); // Maintain X/Z position
        } else {
            cameraPos = glm::vec3(0.0f, 1.5f, 4.0f); // Lock for battle
        }
    }

    void GLRenderer::moveCamera(float dx, float dz) {
        std::lock_guard<std::mutex> lock(stateMutex);
        if (currentState == RenderState::OVERWORLD) {
            cameraPos.x += dx;
            cameraPos.z += dz;
        }
    }

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

    void GLRenderer::setViewport(int w, int h) { width = w; height = h; }

    void GLRenderer::renderLoop(ANativeWindow* window) {
        if (!initEGL(window)) return;
        setupGraphics();
        generateProceduralTerrain();
        
        while (isRendering) {
            drawFrame();
            eglSwapBuffers(display, surface);
        }
        destroyEGL();
    }

    bool GLRenderer::initEGL(ANativeWindow* window) {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) return false;
        if (!eglInitialize(display, nullptr, nullptr)) return false;
        const EGLint attribs[] = { EGL_RENDERABLE_TYPE, 0x00000040, EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 16, EGL_NONE };
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
            if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
            if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
            eglTerminate(display);
        }
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
        if (!vs || !fs) return;
        
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vs);
        glAttachShader(shaderProgram, fs);
        glLinkProgram(shaderProgram);
        
        mvpLoc = glGetUniformLocation(shaderProgram, "u_MVP");
        modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
        timeLoc = glGetUniformLocation(shaderProgram, "u_Time");
        lightDirLoc = glGetUniformLocation(shaderProgram, "u_LightDir");
        lightColLoc = glGetUniformLocation(shaderProgram, "u_LightColor");
        ambColLoc = glGetUniformLocation(shaderProgram, "u_AmbientColor");
        skyTopLoc = glGetUniformLocation(shaderProgram, "u_SkyColorTop");
        skyBotLoc = glGetUniformLocation(shaderProgram, "u_SkyColorBottom");

        glEnable(GL_DEPTH_TEST);
    }

    // Mathematical grid generation for realistic soil/grass
    void GLRenderer::generateProceduralTerrain() {
        terrainVertices.clear();
        terrainIndices.clear();
        
        int gridSize = 40;
        float spacing = 0.5f;

        for (int z = -gridSize/2; z < gridSize/2; ++z) {
            for (int x = -gridSize/2; x < gridSize/2; ++x) {
                float fx = x * spacing;
                float fz = z * spacing;
                
                // Procedural Elevation (Simulating Rocks and Soil)
                float fy = sin(fx * 0.5f) * cos(fz * 0.5f) * 0.5f; 
                fy += sin(fx * 1.5f + fz) * 0.2f; // Detail noise

                // Color mapping based on elevation
                float r = 0.2f, g = 0.5f, b = 0.2f; // Base Grass
                if (fy < -0.2f) { r = 0.3f; g = 0.2f; b = 0.1f; } // Soil/Dirt in dips
                if (fy > 0.3f) { r = 0.4f; g = 0.4f; b = 0.4f; }  // Rocky peaks
                
                // Position (3), Color (4), Normal (3) = 10 floats per vertex
                terrainVertices.insert(terrainVertices.end(), {
                    fx, fy, fz,   r, g, b, 1.0f,   0.0f, 1.0f, 0.0f // Upward normal for simplicity
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

    void GLRenderer::drawFrame() {
        if (!shaderProgram) return;

        // 1. Calculate Engine Time (Used for Wind and Sun Cycle)
        auto now = std::chrono::steady_clock::now();
        float engineTime = std::chrono::duration<float>(now - startTime).count();

        // 2. Day / Night Cycle Logic (Cycle every 60 seconds)
        float cycle = sin(engineTime * 0.1f); // Range -1 to 1
        bool isDay = cycle > 0.0f;

        glm::vec3 skyTop = isDay ? glm::vec3(0.1f, 0.4f, 0.8f) : glm::vec3(0.01f, 0.02f, 0.1f);
        glm::vec3 skyBot = isDay ? glm::vec3(0.6f, 0.8f, 0.9f) : glm::vec3(0.0f, 0.0f, 0.05f);
        glm::vec3 lightDir = glm::vec3(cos(engineTime*0.1f), sin(engineTime*0.1f), 0.5f);
        glm::vec3 lightCol = isDay ? glm::vec3(1.0f, 0.9f, 0.8f) : glm::vec3(0.2f, 0.3f, 0.6f); // Warm Sun vs Cold Moon
        glm::vec3 ambCol = isDay ? glm::vec3(0.3f, 0.3f, 0.3f) : glm::vec3(0.05f, 0.05f, 0.1f);

        // Clear screen with sky bottom color to blend seamlessly
        glViewport(0, 0, width, height);
        glClearColor(skyBot.r, skyBot.g, skyBot.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        std::lock_guard<std::mutex> lock(stateMutex);

        // 3. Update Uniforms
        glUniform1f(timeLoc, engineTime);
        glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));
        glUniform3fv(lightColLoc, 1, glm::value_ptr(lightCol));
        glUniform3fv(ambColLoc, 1, glm::value_ptr(ambCol));
        glUniform3fv(skyTopLoc, 1, glm::value_ptr(skyTop));
        glUniform3fv(skyBotLoc, 1, glm::value_ptr(skyBot));

        float aspect = (float)width / (float)(height > 0 ? height : 1);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        glm::vec3 lookTarget = (currentState == RenderState::OVERWORLD) ? 
                               glm::vec3(cameraPos.x, 0.0f, cameraPos.z - 3.0f) : glm::vec3(0.0f, 0.5f, 0.0f);
                               
        glm::mat4 view = glm::lookAt(cameraPos, lookTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 mvp = projection * view * model;
        
        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        if (currentState == RenderState::OVERWORLD) {
            drawOverworldFloor();
        } else {
            drawEntityCube();
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
        glDrawElements(GL_TRIANGLES, terrainIndices.size(), GL_UNSIGNED_INT, terrainIndices.data());
    }

    void GLRenderer::drawEntityCube() {
        float r = 0.8f, g = 0.2f, b = 0.2f;
        if (activeEntity == "Slime") { r = 0.2f; g = 0.2f; b = 0.8f; }

        GLfloat vertices[] = {
            // Pos(3), Color(4), Normal(3)
            -0.5f, 0.0f,  0.5f,  r, g, b, 1.0f,  -0.5f, 0.5f,  0.5f,
             0.5f, 0.0f,  0.5f,  r, g, b, 1.0f,   0.5f, 0.5f,  0.5f,
             0.0f, 1.0f,  0.0f,  r+0.2f, g, b, 1.0f,  0.0f, 1.0f, 0.0f,
            -0.5f, 0.0f, -0.5f,  r-0.2f, g, b, 1.0f, -0.5f, 0.5f, -0.5f,
             0.5f, 0.0f, -0.5f,  r-0.2f, g, b, 1.0f,  0.5f, 0.5f, -0.5f
        };
        GLuint indices[] = { 0, 1, 2,  1, 4, 2,  4, 3, 2,  3, 0, 2 };

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), vertices);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), &vertices[3]);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(GLfloat), &vertices[7]);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, indices);
    }
}
