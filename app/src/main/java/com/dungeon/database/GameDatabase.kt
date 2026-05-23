package com.dungeon.database

import android.content.Context
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase
import androidx.sqlite.db.SupportSQLiteDatabase
import com.dungeon.database.entities.*
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

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
                .addCallback(DatabaseCallback())
                .build()
                INSTANCE = instance
                instance
            }
        }
    }

    private class DatabaseCallback : RoomDatabase.Callback() {
        override fun onCreate(db: SupportSQLiteDatabase) {
            super.onCreate(db)
            // Seed the database on a background thread when it is first created
            INSTANCE?.let { database ->
                CoroutineScope(Dispatchers.IO).launch {
                    val dao = database.gameStateDao()
                    
                    // Seed initial state
                    dao.updateGameState(
                        GameStateEntity(
                            lastSyncTimestamp = System.currentTimeMillis(),
                            currentFloor = 1,
                            isSimulationActive = true
                        )
                    )

                    // Seed initial party member (e.g., Psyduck analogue)
                    dao.updatePartyMember(
                        PartyMemberEntity(
                            name = "Water Mystic",
                            currentHp = 150,
                            maxHp = 150,
                            attackStat = 25,
                            isSelected = true
                        )
                    )
                }
            }
        }
    }
}
