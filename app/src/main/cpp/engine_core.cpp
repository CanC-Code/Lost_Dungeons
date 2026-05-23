#include "engine_core.h"
#include <android/log.h>
#include <algorithm>

namespace LostDungeons {
    SimulationResult Engine::runSimulation(long deltaSeconds, int currentFloor, int currentHp, int attackStat) {
        __android_log_print(ANDROID_LOG_INFO, "LostDungeonsEngine", "Simulating %ld seconds...", deltaSeconds);

        SimulationResult result;
        result.finalFloor = currentFloor;
        result.finalHp = currentHp;
        result.enemiesDefeated = 0;
        result.partyDied = false;

        if (currentHp <= 0) {
            result.partyDied = true;
            return result; // Cannot progress if dead
        }

        // --- Basic Idle Math ---
        // Base time to clear a floor is 120 seconds. 
        // Higher attack reduces this time (min 30 seconds per floor).
        int secondsPerFloor = std::max(30, 120 - (attackStat / 2)); 
        
        int floorsCleared = deltaSeconds / secondsPerFloor;
        
        if (floorsCleared > 0) {
            result.finalFloor += floorsCleared;
            result.enemiesDefeated = floorsCleared * 3; // 3 enemies per floor
            
            // Player takes 2 damage per floor cleared
            int damageTaken = floorsCleared * 2;
            result.finalHp -= damageTaken;

            if (result.finalHp <= 0) {
                result.finalHp = 0;
                result.partyDied = true;
                __android_log_print(ANDROID_LOG_INFO, "LostDungeonsEngine", "Party died on floor %d", result.finalFloor);
            } else {
                __android_log_print(ANDROID_LOG_INFO, "LostDungeonsEngine", "Cleared %d floors. HP: %d", floorsCleared, result.finalHp);
            }
        }

        return result;
    }
}
