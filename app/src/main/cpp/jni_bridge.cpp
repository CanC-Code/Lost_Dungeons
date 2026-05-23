#include <jni.h>
#include "engine_core.h"
#include "asset_manager.h"
#include <nlohmann/json.hpp>
#include <android/log.h>

using json = nlohmann::json;

// --- Phase 2: Asset Management ---
extern "C" JNIEXPORT void JNICALL
Java_com_dungeon_engine_SimulationController_nativeInitAssetManager(
        JNIEnv* env, jobject /* this */, jobject assetManager) {
    
    LostDungeons::AssetManager::init(env, assetManager);

    // Test the pipeline immediately: Load the JSON and parse it
    std::string goblinData = LostDungeons::AssetManager::loadTextFile("data/monsters/goblin.json");
    if (!goblinData.empty()) {
        try {
            json j = json::parse(goblinData);
            std::string name = j["name"];
            int hp = j["base_hp"];
            __android_log_print(ANDROID_LOG_INFO, "LostDungeonsNative", 
                "Successfully parsed JSON! Monster: %s, Base HP: %d", name.c_str(), hp);
        } catch (json::parse_error& e) {
            __android_log_print(ANDROID_LOG_ERROR, "LostDungeonsNative", "JSON Parse Error: %s", e.what());
        }
    }
}

// --- Phase 0: Legacy Idle Engine ---
extern "C" JNIEXPORT jintArray JNICALL
Java_com_dungeon_engine_SimulationController_nativeRunSimulation(
        JNIEnv* env, jobject /* this */, jlong deltaSeconds, jint currentFloor, jint currentHp, jint attackStat) {
    
    LostDungeons::SimulationResult result = LostDungeons::Engine::runSimulation(
            (long)deltaSeconds, (int)currentFloor, (int)currentHp, (int)attackStat);

    jintArray resultArray = env->NewIntArray(4);
    jint fill[4];
    fill[0] = result.finalFloor;
    fill[1] = result.finalHp;
    fill[2] = result.enemiesDefeated;
    fill[3] = result.partyDied ? 1 : 0;
    
    env->SetIntArrayRegion(resultArray, 0, 4, fill);
    return resultArray;
}
