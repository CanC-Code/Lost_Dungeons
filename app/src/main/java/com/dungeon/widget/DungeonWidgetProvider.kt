package com.dungeon.widget

import android.app.PendingIntent
import android.appwidget.AppWidgetManager
import android.appwidget.AppWidgetProvider
import android.content.Context
import android.content.Intent
import android.widget.RemoteViews
import com.dungeon.R
import com.dungeon.database.GameDatabase
import com.dungeon.ui.WidgetDialogActivity
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class DungeonWidgetProvider : AppWidgetProvider() {

    override fun onUpdate(
        context: Context,
        appWidgetManager: AppWidgetManager,
        appWidgetIds: IntArray
    ) {
        for (appWidgetId in appWidgetIds) {
            updateAppWidget(context, appWidgetManager, appWidgetId)
        }
    }

    companion object {
        fun updateAppWidget(
            context: Context,
            appWidgetManager: AppWidgetManager,
            appWidgetId: Int
        ) {
            val views = RemoteViews(context.packageName, R.layout.widget_dungeon_info)

            // Setup the Intent to launch the Transparent Activity Menu
            val intent = Intent(context, WidgetDialogActivity::class.java)
            val pendingIntent = PendingIntent.getActivity(
                context,
                0,
                intent,
                PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
            )
            views.setOnClickPendingIntent(R.id.btn_widget_menu, pendingIntent)

            // Fetch live data from the database asynchronously
            CoroutineScope(Dispatchers.IO).launch {
                val db = GameDatabase.getDatabase(context)
                val state = db.gameStateDao().getGameState()
                val party = db.gameStateDao().getParty()

                if (state != null) {
                    val statusText = if (state.isSimulationActive) "Active" else "Paused"
                    var hpText = "HP: 0/0"
                    
                    if (party.isNotEmpty()) {
                        // Display the HP of the first party member
                        hpText = "HP: ${party[0].currentHp}/${party[0].maxHp}"
                    }

                    views.setTextViewText(
                        R.id.tv_widget_status, 
                        "Floor: ${state.currentFloor} | $hpText | $statusText"
                    )
                }

                // Push the layout update to the home screen
                appWidgetManager.updateAppWidget(appWidgetId, views)
            }
        }
    }
}
