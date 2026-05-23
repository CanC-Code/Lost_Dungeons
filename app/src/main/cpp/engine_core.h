#ifndef ENGINE_CORE_H
#define ENGINE_CORE_H

namespace LostDungeons {
    
    // Struct to hold the results of the background math
    struct SimulationResult {
        int finalFloor;
        int finalHp;
        int enemiesDefeated;
        bool partyDied;
    };

    class Engine {
    public:
        // Now accepts delta time and party stats, returns the resulting state
        static SimulationResult runSimulation(long deltaSeconds, int currentFloor, int currentHp, int attackStat);
    };
}

#endif
