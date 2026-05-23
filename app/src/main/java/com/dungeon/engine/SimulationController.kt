package com.dungeon.engine

import android.content.res.AssetManager

class SimulationController {

    companion object {
        init {
            System.loadLibrary("engine")
        }
    }

    // Phase 0: Idle math execution
    external fun nativeRunSimulation(deltaSeconds: Long, floor: Int, hp: Int, attack: Int): IntArray
    
    // Phase 2: Hand over the Android AssetManager to C++
    external fun nativeInitAssetManager(assetManager: AssetManager)

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

data class SimulationResultData(
    val finalFloor: Int,
    val finalHp: Int,
    val enemiesDefeated: Int,
    val partyDied: Boolean
)
