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

    // NEW: Phase 4
    external fun nativeSetGameState(state: Int, entityId: String)
    external fun nativeMoveCamera(dx: Float, dz: Float)

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
