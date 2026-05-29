#ifndef GL_RENDERER_H
#define GL_RENDERER_H

#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <android/native_window.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <vector>
#include <string>

namespace LostDungeons {

    enum class RenderState { OVERWORLD, BATTLE };

    class GLRenderer {
    public:
        static void start(ANativeWindow* window);
        static void stop();
        static void setViewport(int w, int h);
        static void setGameState(int state, const std::string& entityId);
        static void updateInput(float moveX, float moveY, float lookX, float lookY);

    private:
        static void renderLoop(ANativeWindow* window);
        static bool initEGL(ANativeWindow* window);
        static void destroyEGL();
        static void setupGraphics();
        static GLuint loadShader(GLenum type, const char* src);
        static void drawFrame();
        
        // Persistent World Generation Methods
        static void updateTerrainMesh();
        static float getTerrainHeight(float worldX, float worldZ);
        
        static void drawOverworldFloor();
        static void drawEntityCube();

        static std::atomic<bool> isRendering;
        static std::thread renderThread;
        static std::mutex stateMutex;

        static EGLDisplay display;
        static EGLSurface surface;
        static EGLContext context;
        static GLuint shaderProgram;

        static GLint mvpLoc, modelLoc, timeLoc, camPosLoc;
        static GLint lightDirLoc, lightColLoc, ambColLoc;
        static GLint skyTopLoc, skyBotLoc;

        static int width;
        static int height;

        // Camera State
        static glm::vec3 cameraPos;
        static glm::vec3 cameraFront;
        static glm::vec3 cameraUp;
        static float yaw;
        static float pitch;

        static RenderState currentState;
        static std::string activeEntity;

        static std::chrono::time_point<std::chrono::steady_clock> startTime;
        
        // Geometry Tracking
        static std::vector<GLfloat> terrainVertices;
        static std::vector<GLuint> terrainIndices;
        static int lastGridX;
        static int lastGridZ;
    };

}

#endif // GL_RENDERER_H
