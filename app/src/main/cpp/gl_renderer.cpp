#include "gl_renderer.h"
#include "asset_manager.h"
#include <android/log.h>

namespace LostDungeons {

    std::atomic<bool> GLRenderer::isRendering{false};
    std::thread GLRenderer::renderThread;
    std::mutex GLRenderer::stateMutex;
    
    EGLDisplay GLRenderer::display = EGL_NO_DISPLAY;
    EGLSurface GLRenderer::surface = EGL_NO_SURFACE;
    EGLContext GLRenderer::context = EGL_NO_CONTEXT;
    GLuint GLRenderer::shaderProgram = 0;
    GLint GLRenderer::mvpLocation = -1;
    
    int GLRenderer::width = 0;
    int GLRenderer::height = 0;

    // Default Camera starts slightly raised and pulled back
    glm::vec3 GLRenderer::cameraPos = glm::vec3(0.0f, 2.0f, 5.0f);
    RenderState GLRenderer::currentState = RenderState::OVERWORLD;
    std::string GLRenderer::activeEntity = "";

    void GLRenderer::setGameState(int state, const std::string& entityId) {
        std::lock_guard<std::mutex> lock(stateMutex);
        currentState = static_cast<RenderState>(state);
        activeEntity = entityId;
        
        // Reset camera when returning to overworld
        if (currentState == RenderState::OVERWORLD) {
            cameraPos = glm::vec3(0.0f, 2.0f, 5.0f);
        } else {
            // Lock camera for battle
            cameraPos = glm::vec3(0.0f, 1.5f, 4.0f); 
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
        if (isRendering) return;
        isRendering = true;
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
        if (!initEGL(window)) return;
        setupGraphics();
        while (isRendering) {
            drawFrame();
            eglSwapBuffers(display, surface);
        }
        destroyEGL();
    }

    bool GLRenderer::initEGL(ANativeWindow* window) {
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglInitialize(display, nullptr, nullptr);
        const EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
            EGL_BLUE_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_RED_SIZE, 8, EGL_DEPTH_SIZE, 16, EGL_NONE
        };
        EGLint numConfigs;
        EGLConfig config;
        eglChooseConfig(display, attribs, &config, 1, &numConfigs);
        const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
        surface = eglCreateWindowSurface(display, config, window, nullptr);
        return eglMakeCurrent(display, surface, surface, context) != EGL_FALSE;
    }

    void GLRenderer::destroyEGL() {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(display, context);
        eglDestroySurface(display, surface);
        eglTerminate(display);
    }

    GLuint GLRenderer::loadShader(GLenum type, const char* shaderSrc) {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &shaderSrc, nullptr);
        glCompileShader(shader);
        return shader;
    }

    void GLRenderer::setupGraphics() {
        std::string vertSrc = AssetManager::loadTextFile("shaders/base.vert");
        std::string fragSrc = AssetManager::loadTextFile("shaders/base.frag");
        GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertSrc.c_str());
        GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragSrc.c_str());
        
        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        
        mvpLocation = glGetUniformLocation(shaderProgram, "u_MVP");
        glEnable(GL_DEPTH_TEST); // Enable 3D Depth testing
    }

    void GLRenderer::drawFrame() {
        glViewport(0, 0, width, height);
        glClearColor(0.1f, 0.18f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        std::lock_guard<std::mutex> lock(stateMutex);
        
        // --- 1. Calculate GLM Camera Matrices ---
        float aspect = (float)width / (float)(height > 0 ? height : 1);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        
        // Camera looks slightly downward in Overworld
        glm::vec3 lookTarget = (currentState == RenderState::OVERWORLD) ? 
                               glm::vec3(cameraPos.x, 0.0f, cameraPos.z - 3.0f) : 
                               glm::vec3(0.0f, 0.5f, 0.0f);
                               
        glm::mat4 view = glm::lookAt(cameraPos, lookTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 model = glm::mat4(1.0f);
        
        glm::mat4 mvp = projection * view * model;
        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));

        // --- 2. Draw based on State ---
        if (currentState == RenderState::OVERWORLD) {
            drawOverworldFloor();
        } else {
            drawEntityCube();
        }
    }

    void GLRenderer::drawOverworldFloor() {
        // A large green plane representing the ground
        GLfloat vertices[] = {
            // X, Y, Z             R, G, B, A
            -10.0f, 0.0f, -10.0f,  0.1f, 0.4f, 0.1f, 1.0f,
             10.0f, 0.0f, -10.0f,  0.1f, 0.4f, 0.1f, 1.0f,
            -10.0f, 0.0f,  10.0f,  0.1f, 0.4f, 0.1f, 1.0f,
             10.0f, 0.0f,  10.0f,  0.1f, 0.4f, 0.1f, 1.0f
        };
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), vertices);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), &vertices[3]);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    void GLRenderer::drawEntityCube() {
        // Simple 3D colored placeholder for the monster
        float r = 0.8f, g = 0.2f, b = 0.2f; // Default Red (Goblin)
        if (activeEntity == "Slime") { r = 0.2f; g = 0.2f; b = 0.8f; } // Blue Slime

        GLfloat vertices[] = {
            // Front Face
            -0.5f, 0.0f,  0.5f,  r, g, b, 1.0f,
             0.5f, 0.0f,  0.5f,  r, g, b, 1.0f,
             0.0f, 1.0f,  0.0f,  r+0.2f, g, b, 1.0f, // Peak
             // Back left
            -0.5f, 0.0f, -0.5f,  r-0.2f, g, b, 1.0f,
             // Back right
             0.5f, 0.0f, -0.5f,  r-0.2f, g, b, 1.0f
        };
        
        // Using a basic pyramid (5 vertices) for quick 3D visual proof
        GLuint indices[] = {
            0, 1, 2, // Front
            1, 4, 2, // Right
            4, 3, 2, // Back
            3, 0, 2  // Left
        };

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), vertices);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), &vertices[3]);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, indices);
    }
}
