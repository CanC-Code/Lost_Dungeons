package com.dungeon.engine

import android.content.res.AssetManager
import android.view.Surface

class SimulationController {

    companion object {
        init {
            System.loadLibrary("engine")
        }
    }

    // Phase 0: Idle math execution
    external fun nativeRunSimulation(deltaSeconds: Long, floor: Int, hp: Int, attack: Int): IntArray
    
    // Phase 2: Asset Management
    external fun nativeInitAssetManager(assetManager: AssetManager)
    
    // Phase 3: OpenGL Surface Pipeline
    external fun nativeSurfaceCreated(surface: Surface)
    external fun nativeSurfaceChanged(width: Int, height: Int)
    external fun nativeSurfaceDestroyed()

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
