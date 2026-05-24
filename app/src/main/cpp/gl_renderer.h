#ifndef GL_RENDERER_H
#define GL_RENDERER_H

#include <android/native_window.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <thread>
#include <atomic>
#include <mutex>
#include <string>

namespace LostDungeons {
    enum class RenderState { OVERWORLD = 0, BATTLE = 1 };

    class GLRenderer {
    public:
        static void start(ANativeWindow* window);
        static void stop();
        static void setViewport(int width, int height);
        
        // NEW: Game State and Camera Controls
        static void setGameState(int state, const std::string& entityId);
        static void moveCamera(float dx, float dz);

    private:
        static std::atomic<bool> isRendering;
        static std::thread renderThread;
        static std::mutex stateMutex;
        
        static EGLDisplay display;
        static EGLSurface surface;
        static EGLContext context;
        static GLuint shaderProgram;
        static GLint mvpLocation;

        static int width;
        static int height;

        // Camera & State variables
        static glm::vec3 cameraPos;
        static RenderState currentState;
        static std::string activeEntity;

        static void renderLoop(ANativeWindow* window);
        static bool initEGL(ANativeWindow* window);
        static void destroyEGL();
        static GLuint loadShader(GLenum type, const char* shaderSrc);
        static void setupGraphics();
        static void drawFrame();
        
        // Geometry Drawing Helpers
        static void drawOverworldFloor();
        static void drawEntityCube();
    };
}
#endif
