package com.dungeon.engine

import android.content.Context
import android.util.Log
import androidx.work.CoroutineWorker
import androidx.work.WorkerParameters
import androidx.room.Room
import com.dungeon.database.GameDatabase
import com.dungeon.database.entities.GameStateEntity

class DungeonWorker(
    appContext: Context,
    workerParams: WorkerParameters
) : CoroutineWorker(appContext, workerParams) {

    override suspend fun doWork(): Result {
        Log.i("DungeonWorker", "Starting simulation tick.")

        // 1. Initialize Database Access
        // In a production environment, this should ideally be injected via a Dependency Injection 
        // framework like Hilt, or accessed via a singleton in DungeonApplication.
        val db = Room.databaseBuilder(
            applicationContext,
            GameDatabase::class.java, "dungeon-db"
        ).build()

        val dao = db.gameStateDao()
        
        // 2. Fetch current state and time
        var currentState = dao.getGameState()
        val currentTime = System.currentTimeMillis()

        if (currentState == null) {
            // Initialize the database on the very first run
            currentState = GameStateEntity(
                lastSyncTimestamp = currentTime,
                currentFloor = 1,
                isSimulationActive = true
            )
            dao.updateGameState(currentState)
            return Result.success()
        }

        if (!currentState.isSimulationActive) {
            Log.i("DungeonWorker", "Simulation is currently paused by the user.")
            return Result.success()
        }

        // 3. Trigger C++ Simulation logic
        val controller = SimulationController()
        val startTime = currentState.lastSyncTimestamp
        
        if (currentTime > startTime) {
            // Execute the delta calculation
            controller.executeTick(startTime, currentTime)
            
            // 4. Update the state with the new timestamp
            // Note: In an advanced implementation, your C++ engine would return a struct 
            // containing the new floor and HP, or interact with SQLite directly. 
            // For now, we update the timestamp to ensure the next tick calculates the next time delta.
            val updatedState = currentState.copy(lastSyncTimestamp = currentTime)
            dao.updateGameState(updatedState)
            
            // TODO: In the next phase, trigger a broadcast here to update the Widget UI
        }

        Log.i("DungeonWorker", "Simulation tick completed successfully.")
        return Result.success()
    }
}
