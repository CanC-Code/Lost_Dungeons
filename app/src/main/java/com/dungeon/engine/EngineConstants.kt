package com.dungeon.engine

object EngineConstants {
    const val WORK_NAME = "dungeon_simulation_work"
    // The OS limits periodic work to a minimum of 15 minutes. 
    // This is the ideal tick rate for an AFK idle game.
    const val MIN_SIMULATION_INTERVAL_MINUTES = 15L 
}
