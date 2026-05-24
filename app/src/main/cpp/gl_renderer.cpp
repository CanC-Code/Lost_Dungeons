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

    glm::vec3 GLRenderer::cameraPos = glm::vec3(0.0f, 2.0f, 5.0f);
    RenderState GLRenderer::currentState = RenderState::OVERWORLD;
    std::string GLRenderer::activeEntity = "";

    void GLRenderer::setGameState(int state, const std::string& entityId) {
        std::lock_guard<std::mutex> lock(stateMutex);
        currentState = static_cast<RenderState>(state);
        activeEntity = entityId;
        
        if (currentState == RenderState::OVERWORLD) {
            cameraPos = glm::vec3(0.0f, 2.0f, 5.0f);
        } else {
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
        if (!window || isRendering) return;
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
        if (!initEGL(window)) {
            __android_log_print(ANDROID_LOG_ERROR, "LostDungeonsGL", "FATAL: EGL Initialization Failed. Aborting render thread.");
            return; // Gracefully exit instead of crashing
        }
        
        setupGraphics();
        
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

        // Using standard EGL attributes for maximum device compatibility
        const EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, 0x00000040, // EGL_OPENGL_ES3_BIT
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 16,
            EGL_NONE
        };

        EGLint numConfigs = 0;
        EGLConfig config;
        eglChooseConfig(display, attribs, &config, 1, &numConfigs);
        
        // CRITICAL FIX: Prevent memory crash if GPU rejects config
        if (numConfigs == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "LostDungeonsGL", "GPU rejected EGL config. Zero configs returned.");
            return false;
        }

        const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
        if (context == EGL_NO_CONTEXT) return false;

        surface = eglCreateWindowSurface(display, config, window, nullptr);
        if (surface == EGL_NO_SURFACE) return false;

        return eglMakeCurrent(display, surface, surface, context) != EGL_FALSE;
    }

    void GLRenderer::destroyEGL() {
        if (display != EGL_NO_DISPLAY) {
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (context != EGL_NO_CONTEXT) eglDestroyContext(display, context);
            if (surface != EGL_NO_SURFACE) eglDestroySurface(display, surface);
            eglTerminate(display);
        }
        display = EGL_NO_DISPLAY;
        context = EGL_NO_CONTEXT;
        surface = EGL_NO_SURFACE;
    }

    GLuint GLRenderer::loadShader(GLenum type, const char* shaderSrc) {
        if (!shaderSrc || shaderSrc[0] == '\0') return 0;
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &shaderSrc, nullptr);
        glCompileShader(shader);
        GLint compiled;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            __android_log_print(ANDROID_LOG_ERROR, "LostDungeonsGL", "Shader compilation failed");
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    void GLRenderer::setupGraphics() {
        std::string vertSrc = AssetManager::loadTextFile("shaders/base.vert");
        std::string fragSrc = AssetManager::loadTextFile("shaders/base.frag");

        // Safety abort if files are missing from assets folder
        if (vertSrc.empty() || fragSrc.empty()) {
            __android_log_print(ANDROID_LOG_ERROR, "LostDungeonsGL", "Shader files missing! Cannot create program.");
            return;
        }

        GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertSrc.c_str());
        GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragSrc.c_str());
        
        if (vertexShader == 0 || fragmentShader == 0) return;

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
        
        mvpLocation = glGetUniformLocation(shaderProgram, "u_MVP");
        glEnable(GL_DEPTH_TEST);
    }

    void GLRenderer::drawFrame() {
        glViewport(0, 0, width, height);
        glClearColor(0.1f, 0.18f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Safety: Do not attempt to draw math if shader failed to compile
        if (shaderProgram == 0) return;

        glUseProgram(shaderProgram);

        std::lock_guard<std::mutex> lock(stateMutex);
        
        float aspect = (float)width / (float)(height > 0 ? height : 1);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        
        glm::vec3 lookTarget = (currentState == RenderState::OVERWORLD) ? 
                               glm::vec3(cameraPos.x, 0.0f, cameraPos.z - 3.0f) : 
                               glm::vec3(0.0f, 0.5f, 0.0f);
                               
        glm::mat4 view = glm::lookAt(cameraPos, lookTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 model = glm::mat4(1.0f);
        
        glm::mat4 mvp = projection * view * model;
        if (mvpLocation != -1) {
            glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));
        }

        if (currentState == RenderState::OVERWORLD) {
            drawOverworldFloor();
        } else {
            drawEntityCube();
        }
    }

    void GLRenderer::drawOverworldFloor() {
        GLfloat vertices[] = {
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
        float r = 0.8f, g = 0.2f, b = 0.2f;
        if (activeEntity == "Slime") { r = 0.2f; g = 0.2f; b = 0.8f; }

        GLfloat vertices[] = {
            -0.5f, 0.0f,  0.5f,  r, g, b, 1.0f,
             0.5f, 0.0f,  0.5f,  r, g, b, 1.0f,
             0.0f, 1.0f,  0.0f,  r+0.2f, g, b, 1.0f,
            -0.5f, 0.0f, -0.5f,  r-0.2f, g, b, 1.0f,
             0.5f, 0.0f, -0.5f,  r-0.2f, g, b, 1.0f
        };
        
        GLuint indices[] = {
            0, 1, 2,
            1, 4, 2,
            4, 3, 2,
            3, 0, 2
        };

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), vertices);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), &vertices[3]);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glDrawElements(GL_TRIANGLES, 12, GL_UNSIGNED_INT, indices);
    }
}
