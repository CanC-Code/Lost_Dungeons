#include "engine_core.h"
#include <android/log.h>

namespace LostDungeons {
    void Engine::runSimulation(long start, long end) {
        __android_log_print(ANDROID_LOG_INFO, "LostDungeons", "Simulating from %ld to %ld", start, end);
        // Add your deterministic math here
    }
}
