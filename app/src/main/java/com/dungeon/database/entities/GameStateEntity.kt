package com.dungeon.database.entities

import androidx.room.Entity
import androidx.room.PrimaryKey

@Entity(tableName = "game_state")
data class GameStateEntity(
    @PrimaryKey val id: Int = 0, // Singleton state
    val lastSyncTimestamp: Long,
    val currentFloor: Int,
    val isSimulationActive: Boolean
)
