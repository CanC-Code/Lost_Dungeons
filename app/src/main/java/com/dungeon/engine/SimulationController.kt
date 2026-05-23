package com.dungeon.engine

class SimulationController {

    companion object {
        init {
            System.loadLibrary("engine")
        }
    }

    // Updated external signature mapping to JNI
    external fun nativeRunSimulation(deltaSeconds: Long, floor: Int, hp: Int, attack: Int): IntArray

    // Wrapper function to make calling it from the Worker cleaner
    fun executeTick(deltaSeconds: Long, currentFloor: Int, currentHp: Int, attackStat: Int): SimulationResultData {
        val rawResult = nativeRunSimulation(deltaSeconds, currentFloor, currentHp, attackStat)
        return SimulationResultData(
            finalFloor = rawResult[0],
            finalHp = rawResult[1],
            enemiesDefeated = rawResult[2],
            partyDied = rawResult[3] == 1
        )
    }
}

// Data class to hold the parsed JNI return values
data class SimulationResultData(
    val finalFloor: Int,
    val finalHp: Int,
    val enemiesDefeated: Int,
    val partyDied: Boolean
)
