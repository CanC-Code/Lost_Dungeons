#include <jni.h>
#include <android/native_window_jni.h>
#include "engine_core.h"
#include "asset_manager.h"
#include "gl_renderer.h"

// --- Existing Phase 2 Code ---
extern "C" JNIEXPORT void JNICALL
Java_com_dungeon_engine_SimulationController_nativeInitAssetManager(
        JNIEnv* env, jobject /* this */, jobject assetManager) {
    LostDungeons::AssetManager::init(env, assetManager);
}

// --- NEW Phase 3 Code: OpenGL Surface Management ---
extern "C" JNIEXPORT void JNICALL
Java_com_dungeon_engine_SimulationController_nativeSurfaceCreated(
        JNIEnv* env, jobject /* this */, jobject surface) {
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    LostDungeons::GLRenderer::start(window);
}

extern "C" JNIEXPORT void JNICALL
Java_com_dungeon_engine_SimulationController_nativeSurfaceChanged(
        JNIEnv* env, jobject /* this */, jint width, jint height) {
    LostDungeons::GLRenderer::setViewport(width, height);
}

extern "C" JNIEXPORT void JNICALL
Java_com_dungeon_engine_SimulationController_nativeSurfaceDestroyed(
        JNIEnv* env, jobject /* this */) {
    LostDungeons::GLRenderer::stop();
}

// --- Existing Phase 0 Code ---
extern "C" JNIEXPORT jintArray JNICALL
Java_com_dungeon_engine_SimulationController_nativeRunSimulation(
        JNIEnv* env, jobject /* this */, jlong deltaSeconds, jint currentFloor, jint currentHp, jint attackStat) {
    // ... Existing implementation remains unchanged ...
}
