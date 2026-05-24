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
#include <vector>
#include <chrono>

namespace LostDungeons {
    enum class RenderState { OVERWORLD = 0, BATTLE = 1 };

    class GLRenderer {
    public:
        static void start(ANativeWindow* window);
        static void stop();
        static void setViewport(int width, int height);
        static void setGameState(int state, const std::string& entityId);
        static void updateInput(float moveX, float moveY, float lookX, float lookY);

    private:
        static std::atomic<bool> isRendering;
        static std::thread renderThread;
        static std::mutex stateMutex;
        
        static EGLDisplay display;
        static EGLSurface surface;
        static EGLContext context;
        static GLuint shaderProgram;
        
        // Shader Uniforms
        static GLint mvpLoc, modelLoc, timeLoc, camPosLoc;
        static GLint lightDirLoc, lightColLoc, ambColLoc, skyTopLoc, skyBotLoc;

        static int width;
        static int height;

        // FPS Camera State
        static glm::vec3 cameraPos;
        static glm::vec3 cameraFront;
        static glm::vec3 cameraUp;
        static float yaw;
        static float pitch;

        static RenderState currentState;
        static std::string activeEntity;
        
        // Engine Clock
        static std::chrono::time_point<std::chrono::steady_clock> startTime;

        // Procedural Geometry Buffers
        static std::vector<GLfloat> terrainVertices;
        static std::vector<GLuint> terrainIndices;

        static void renderLoop(ANativeWindow* window);
        static bool initEGL(ANativeWindow* window);
        static void destroyEGL();
        static GLuint loadShader(GLenum type, const char* shaderSrc);
        static void setupGraphics();
        static void generateProceduralTerrain();
        static void drawFrame();
        
        static void drawOverworldFloor();
        static void drawEntityCube();
    };
}
#endif
