#ifndef GL_RENDERER_H
#define GL_RENDERER_H

#include <android/native_window.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <thread>
#include <atomic>

namespace LostDungeons {
    class GLRenderer {
    public:
        static void start(ANativeWindow* window);
        static void stop();
        static void setViewport(int width, int height);

    private:
        static std::atomic<bool> isRendering;
        static std::thread renderThread;
        
        static EGLDisplay display;
        static EGLSurface surface;
        static EGLContext context;
        static GLuint shaderProgram;

        static int width;
        static int height;

        static void renderLoop(ANativeWindow* window);
        static bool initEGL(ANativeWindow* window);
        static void destroyEGL();
        static GLuint loadShader(GLenum type, const char* shaderSrc);
        static void setupGraphics();
        static void drawFrame();
    };
}

#endif
