package com.dungeon.database

import androidx.room.Database
import androidx.room.RoomDatabase
import com.dungeon.database.entities.*

@Database(
    entities = [
        GameStateEntity::class, 
        PartyMemberEntity::class, 
        InventoryItemEntity::class
    ], 
    version = 1,
    exportSchema = false
)
abstract class GameDatabase : RoomDatabase() {
    abstract fun gameStateDao(): GameStateDao
}
