#include "gl_renderer.h"
#include "asset_manager.h"
#include <android/log.h>
#include <vector>

namespace LostDungeons {

    std::atomic<bool> GLRenderer::isRendering{false};
    std::thread GLRenderer::renderThread;
    EGLDisplay GLRenderer::display = EGL_NO_DISPLAY;
    EGLSurface GLRenderer::surface = EGL_NO_SURFACE;
    EGLContext GLRenderer::context = EGL_NO_CONTEXT;
    GLuint GLRenderer::shaderProgram = 0;
    int GLRenderer::width = 0;
    int GLRenderer::height = 0;

    void GLRenderer::start(ANativeWindow* window) {
        if (isRendering) return;
        isRendering = true;
        renderThread = std::thread(renderLoop, window);
    }

    void GLRenderer::stop() {
        isRendering = false;
        if (renderThread.joinable()) {
            renderThread.join();
        }
    }

    void GLRenderer::setViewport(int w, int h) {
        width = w;
        height = h;
    }

    void GLRenderer::renderLoop(ANativeWindow* window) {
        if (!initEGL(window)) {
            __android_log_print(ANDROID_LOG_ERROR, "LostDungeonsGL", "EGL Initialization failed");
            return;
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
        eglInitialize(display, nullptr, nullptr);

        const EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
        };

        EGLint numConfigs;
        EGLConfig config;
        eglChooseConfig(display, attribs, &config, 1, &numConfigs);

        const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
        surface = eglCreateWindowSurface(display, config, window, nullptr);

        if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
            return false;
        }
        return true;
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

        GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertSrc.c_str());
        GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragSrc.c_str());

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);
        glLinkProgram(shaderProgram);
    }

    void GLRenderer::drawFrame() {
        glViewport(0, 0, width, height);
        
        // Background clear color (Dark Forest ambient)
        glClearColor(0.1f, 0.18f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Procedural Base Generation: A basic grass plane (Triangle)
        GLfloat vertices[] = {
             0.0f,  0.5f, 0.0f,   0.2f, 0.8f, 0.2f, 1.0f, // Top (Green)
            -0.5f, -0.5f, 0.0f,   0.1f, 0.5f, 0.1f, 1.0f, // Bottom Left (Dark Green)
             0.5f, -0.5f, 0.0f,   0.1f, 0.5f, 0.1f, 1.0f  // Bottom Right (Dark Green)
        };

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), vertices);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(GLfloat), &vertices[3]);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}
