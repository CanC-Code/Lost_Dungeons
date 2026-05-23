package com.dungeon.widget

import android.app.PendingIntent
import android.appwidget.AppWidgetManager
import android.appwidget.AppWidgetProvider
import android.content.Context
import android.content.Intent
import android.widget.RemoteViews
import com.dungeon.R
import com.dungeon.database.GameDatabase
import com.dungeon.ui.BattleActivity
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

            // 1. Setup the Intent to launch the Transparent Activity Menu (Popup)
            val menuIntent = Intent(context, WidgetDialogActivity::class.java)
            val menuPendingIntent = PendingIntent.getActivity(
                context,
                0,
                menuIntent,
                PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
            )
            views.setOnClickPendingIntent(R.id.btn_widget_menu, menuPendingIntent)

            // 2. Setup the Intent to live launch into the main game (Now BattleActivity)
            val launchIntent = Intent(context, BattleActivity::class.java)
            val launchPendingIntent = PendingIntent.getActivity(
                context,
                1, // Use a different request code to distinguish from the menu intent
                launchIntent,
                PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
            )
            views.setOnClickPendingIntent(R.id.btn_widget_launch, launchPendingIntent)

            // Fetch live data from the database asynchronously
            CoroutineScope(Dispatchers.IO).launch {
                val db = GameDatabase.getDatabase(context)
                val state = db.gameStateDao().getGameState()
                val party = db.gameStateDao().getParty()

                if (state != null) {
                    val statusText = if (state.isSimulationActive) "Engine: Active" else "Engine: Paused"
                    var hpText = "HP: 0/0"
                    
                    if (party.isNotEmpty()) {
                        // Display the HP of the first party member
                        hpText = "Party Lead HP: ${party[0].currentHp}/${party[0].maxHp}"
                    }

                    views.setTextViewText(
                        R.id.tv_widget_status, 
                        "Floor: ${state.currentFloor}\n$hpText\n$statusText"
                    )
                }

                // Push the layout update to the home screen
                appWidgetManager.updateAppWidget(appWidgetId, views)
            }
        }
    }
}
