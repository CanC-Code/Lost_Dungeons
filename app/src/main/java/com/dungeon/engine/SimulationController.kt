package com.dungeon.engine

import android.content.res.AssetManager
import android.view.Surface

// Data class mapping the C++ IntArray back to Kotlin properties
data class SimulationResult(
    val finalFloor: Int,
    val finalHp: Int,
    val enemiesDefeated: Int,
    val partyDied: Boolean
)

class SimulationController {
    
    companion object {
        init {
            // Loads the CMake compiled C++ library exactly once
            System.loadLibrary("engine")
        }
    }

    // --- Lifecycle & Assets ---
    external fun nativeInitAssetManager(assetManager: AssetManager)
    external fun nativeSurfaceCreated(surface: Surface)
    external fun nativeSurfaceChanged(width: Int, height: Int)
    external fun nativeSurfaceDestroyed()

    // --- Game State & Math ---
    external fun nativeSetGameState(state: Int, entityId: String)
    external fun nativeRunSimulation(deltaSeconds: Long, currentFloor: Int, currentHp: Int, attackStat: Int): IntArray

    // --- Interaction & Input ---
    external fun nativeUpdateInput(moveX: Float, moveY: Float, lookX: Float, lookY: Float)
    external fun nativeToggleCompass()
    external fun nativeHandleTap(x: Float, y: Float)

    // --- Kotlin Wrappers ---
    // Safely bridges the DungeonWorker inputs to the native array outputs
    fun executeTick(deltaSeconds: Long, currentFloor: Int, currentHp: Int, attackStat: Int): SimulationResult {
        val rawResult = nativeRunSimulation(deltaSeconds, currentFloor, currentHp, attackStat)
        
        return SimulationResult(
            finalFloor = rawResult[0],
            finalHp = rawResult[1],
            enemiesDefeated = rawResult[2],
            partyDied = rawResult[3] == 1
        )
    }
}
