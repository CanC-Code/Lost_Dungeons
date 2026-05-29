package com.dungeon.engine

import android.content.res.AssetManager
import android.view.Surface

object SimulationController {
    init {
        // Loads the CMake compiled C++ library (libengine.so)
        System.loadLibrary("engine")
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
}
