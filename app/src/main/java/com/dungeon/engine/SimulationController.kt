package com.dungeon.engine

import android.content.res.AssetManager
import android.view.Surface

class SimulationController {

    companion object {
        init {
            System.loadLibrary("engine")
        }
    }

    external fun nativeRunSimulation(deltaSeconds: Long, floor: Int, hp: Int, attack: Int): IntArray
    external fun nativeInitAssetManager(assetManager: AssetManager)
    external fun nativeSurfaceCreated(surface: Surface)
    external fun nativeSurfaceChanged(width: Int, height: Int)
    external fun nativeSurfaceDestroyed()
    external fun nativeSetGameState(state: Int, entityId: String)
    
    // NEW: Handles smooth omnidirectional movement and camera rotation
    external fun nativeUpdateInput(moveX: Float, moveY: Float, lookX: Float, lookY: Float)

    fun executeTick(deltaSeconds: Long, currentFloor: Int, currentHp: Int, attackStat: Int): SimulationResultData {
        val rawResult = nativeRunSimulation(deltaSeconds, currentFloor, currentHp, attackStat)
        return SimulationResultData(rawResult[0], rawResult[1], rawResult[2], rawResult[3] == 1)
    }
}

data class SimulationResultData(val finalFloor: Int, val finalHp: Int, val enemiesDefeated: Int, val partyDied: Boolean)
