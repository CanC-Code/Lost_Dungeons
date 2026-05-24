#include <jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include "engine_core.h"
#include "asset_manager.h"
#include "gl_renderer.h"

// ... (Keep existing nativeInitAssetManager, nativeSurfaceCreated/Changed/Destroyed, nativeRunSimulation) ...

// NEW: Phase 4 - Camera and State Controls
extern "C" JNIEXPORT void JNICALL
Java_com_dungeon_engine_SimulationController_nativeSetGameState(
        JNIEnv* env, jobject /* this */, jint state, jstring entityId) {
    
    const char* cEntityId = env->GetStringUTFChars(entityId, nullptr);
    LostDungeons::GLRenderer::setGameState(state, std::string(cEntityId));
    env->ReleaseStringUTFChars(entityId, cEntityId);
}

extern "C" JNIEXPORT void JNICALL
Java_com_dungeon_engine_SimulationController_nativeMoveCamera(
        JNIEnv* env, jobject /* this */, jfloat dx, jfloat dz) {
    
    LostDungeons::GLRenderer::moveCamera(dx, dz);
}
