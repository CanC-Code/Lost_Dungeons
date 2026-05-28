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

        // --- Diminishing Returns Idle Math ---
        // Base time to clear a floor is 120 seconds.
        // Using division ensures the time per floor smoothly decreases 
        // as attack scales infinitely, preventing a hard cap.
        int secondsPerFloor = std::max(5, static_cast<int>(120.0 / (1.0 + (attackStat / 50.0))));
        
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
