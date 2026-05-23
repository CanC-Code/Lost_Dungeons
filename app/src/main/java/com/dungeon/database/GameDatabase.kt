package com.dungeon.database

import android.content.Context
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase
import com.dungeon.database.entities.*

@Database(
    entities = [
        GameStateEntity::class,
        PartyMemberEntity::class,
        InventoryItemEntity::class
    ],
    version = 3, // BUMPED TO 3 FOR EVASION STAT
    exportSchema = false
)
abstract class GameDatabase : RoomDatabase() {
    abstract fun gameStateDao(): GameStateDao

    companion object {
        @Volatile
        private var INSTANCE: GameDatabase? = null

        fun getDatabase(context: Context): GameDatabase {
            return INSTANCE ?: synchronized(this) {
                val instance = Room.databaseBuilder(
                    context.applicationContext,
                    GameDatabase::class.java,
                    "dungeon-db"
                )
                .fallbackToDestructiveMigration()
                .build()
                INSTANCE = instance
                instance
            }
        }
    }
}
