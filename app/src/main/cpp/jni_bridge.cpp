#include <jni.h>
#include "engine_core.h"

extern "C" JNIEXPORT void JNICALL
Java_com_dungeon_engine_SimulationController_nativeRunSimulation(
        JNIEnv* env, jobject /* this */, jlong start, jlong end) {
    LostDungeons::Engine::runSimulation((long)start, (long)end);
}
