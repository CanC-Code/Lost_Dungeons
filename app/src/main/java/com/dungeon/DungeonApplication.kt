package com.dungeon

import android.app.Application
import androidx.work.PeriodicWorkRequestBuilder
import androidx.work.WorkManager
import androidx.work.ExistingPeriodicWorkPolicy
import com.dungeon.engine.DungeonWorker
import com.dungeon.engine.EngineConstants
import java.util.concurrent.TimeUnit

class DungeonApplication : Application() {

    override fun onCreate() {
        super.onCreate()

        // Schedule the headless C++ engine to run periodically
        val workRequest = PeriodicWorkRequestBuilder<DungeonWorker>(
            EngineConstants.MIN_SIMULATION_INTERVAL_MINUTES, TimeUnit.MINUTES
        ).build()

        WorkManager.getInstance(this).enqueueUniquePeriodicWork(
            EngineConstants.WORK_NAME,
            ExistingPeriodicWorkPolicy.KEEP, // Do not restart if already running
            workRequest
        )
    }
}
