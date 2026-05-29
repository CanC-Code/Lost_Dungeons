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

    enum class RenderState { OVERWORLD, BATTLE, MENU };

    class GLRenderer {
    public:
        static void start(ANativeWindow* window);
        static void stop();
        static void setViewport(int w, int h);
        static void setGameState(int state, const std::string& entityId);
        static void updateInput(float moveX, float moveY, float lookX, float lookY);
        static void toggleCompassMode();
        static void handleTap(float screenX, float screenY);

    private:
        static void renderLoop(ANativeWindow* window);
        static bool initEGL(ANativeWindow* window);
        static void destroyEGL();
        static void setupGraphics();
        static GLuint loadShader(GLenum type, const char* src);
        static void drawFrame();
        
        static void updateTerrainMesh();
        static float getTerrainHeight(float worldX, float worldZ);
        
        static void drawOverworldFloor();
        static void drawEntityCube();
        
        static void drawCompassHUD(float engineTime);
        static void drawMenuOverlay();

        static std::atomic<bool> isRendering;
        static std::thread renderThread;
        static std::mutex stateMutex;

        static EGLDisplay display;
        static EGLSurface surface;
        static EGLContext context;
        static GLuint shaderProgram;
        static GLuint uiShaderProgram;

        static GLint mvpLoc, modelLoc, timeLoc, camPosLoc;
        static GLint lightDirLoc, lightColLoc, ambColLoc;
        static GLint skyTopLoc, skyBotLoc;
        static GLint uiMvpLoc;

        static int width;
        static int height;

        // Active Camera State
        static glm::vec3 cameraPos;
        static glm::vec3 cameraFront;
        static glm::vec3 cameraUp;
        static float yaw;
        static float pitch;

        // Cached Overworld State
        static glm::vec3 savedOverworldPos;
        static float savedOverworldYaw;
        static float savedOverworldPitch;

        static RenderState currentState;
        static std::string activeEntity;
        
        // Physics-Based Compass State
        static bool compassLockedToNorth;
        static float currentCompassAngle;
        static float compassVelocity;
        static float lastFrameTime;

        static std::chrono::time_point<std::chrono::steady_clock> startTime;
        
        static std::vector<GLfloat> terrainVertices;
        static std::vector<GLuint> terrainIndices;
        static int lastGridX;
        static int lastGridZ;
    };

}

#endif // GL_RENDERER_H
