#include <jni.h>
#include "engine_core.h"

extern "C" JNIEXPORT jintArray JNICALL
Java_com_dungeon_engine_SimulationController_nativeRunSimulation(
        JNIEnv* env, jobject /* this */, jlong deltaSeconds, jint currentFloor, jint currentHp, jint attackStat) {
    
    // Call the C++ engine logic
    LostDungeons::SimulationResult result = LostDungeons::Engine::runSimulation(
            (long)deltaSeconds, (int)currentFloor, (int)currentHp, (int)attackStat);

    // Package the struct back into an IntArray for Kotlin
    jintArray resultArray = env->NewIntArray(4);
    jint fill[4];
    fill[0] = result.finalFloor;
    fill[1] = result.finalHp;
    fill[2] = result.enemiesDefeated;
    fill[3] = result.partyDied ? 1 : 0;
    
    env->SetIntArrayRegion(resultArray, 0, 4, fill);
    return resultArray;
}
