package com.dungeon.engine

import android.appwidget.AppWidgetManager
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.util.Log
import androidx.work.CoroutineWorker
import androidx.work.WorkerParameters
import com.dungeon.database.GameDatabase
import com.dungeon.widget.DungeonWidgetProvider

class DungeonWorker(
    appContext: Context,
    workerParams: WorkerParameters
) : CoroutineWorker(appContext, workerParams) {

    override suspend fun doWork(): Result {
        Log.i("DungeonWorker", "Starting simulation tick.")

        val db = GameDatabase.getDatabase(applicationContext)
        val dao = db.gameStateDao()
        
        val currentState = dao.getGameState()
        val party = dao.getParty()
        val currentTime = System.currentTimeMillis()

        if (currentState == null || !currentState.isSimulationActive || party.isEmpty()) {
            return Result.success()
        }

        val leadMember = party[0]
        
        // Calculate delta time in seconds
        val deltaMillis = currentTime - currentState.lastSyncTimestamp
        val deltaSeconds = deltaMillis / 1000L

        if (deltaSeconds > 0 && leadMember.currentHp > 0) {
            val controller = SimulationController()
            
            // 1. Pass Data to Native C++ Engine
            val result = controller.executeTick(
                deltaSeconds = deltaSeconds,
                currentFloor = currentState.currentFloor,
                currentHp = leadMember.currentHp,
                attackStat = leadMember.attackStat
            )

            // 2. Update Database with Engine Results
            dao.updateGameState(currentState.copy(
                lastSyncTimestamp = currentTime,
                currentFloor = result.finalFloor,
                isSimulationActive = !result.partyDied // Pause sim if they died
            ))

            dao.updatePartyMember(leadMember.copy(
                currentHp = result.finalHp
            ))

            // 3. Force the Widget to redraw with the new HP and Floor data
            refreshWidget()
        } else if (leadMember.currentHp <= 0) {
             // Keep timestamp synced even if dead to avoid massive delta when revived
             dao.updateGameState(currentState.copy(lastSyncTimestamp = currentTime, isSimulationActive = false))
             refreshWidget()
        }

        return Result.success()
    }

    private fun refreshWidget() {
        val intent = Intent(applicationContext, DungeonWidgetProvider::class.java)
        intent.action = AppWidgetManager.ACTION_APPWIDGET_UPDATE
        val ids = AppWidgetManager.getInstance(applicationContext).getAppWidgetIds(
            ComponentName(applicationContext, DungeonWidgetProvider::class.java)
        )
        intent.putExtra(AppWidgetManager.EXTRA_APPWIDGET_IDS, ids)
        applicationContext.sendBroadcast(intent)
    }
}
