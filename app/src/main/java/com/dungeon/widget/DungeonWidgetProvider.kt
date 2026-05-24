package com.dungeon.widget

import android.app.PendingIntent
import android.appwidget.AppWidgetManager
import android.appwidget.AppWidgetProvider
import android.content.Context
import android.content.Intent
import android.widget.RemoteViews
import com.dungeon.R
import com.dungeon.database.GameDatabase
import com.dungeon.ui.IntroActivity
import com.dungeon.ui.WidgetDialogActivity
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class DungeonWidgetProvider : AppWidgetProvider() {

    override fun onUpdate(context: Context, appWidgetManager: AppWidgetManager, appWidgetIds: IntArray) {
        for (appWidgetId in appWidgetIds) {
            updateAppWidget(context, appWidgetManager, appWidgetId)
        }
    }

    companion object {
        fun updateAppWidget(context: Context, appWidgetManager: AppWidgetManager, appWidgetId: Int) {
            val views = RemoteViews(context.packageName, R.layout.widget_dungeon_info)

            val menuIntent = Intent(context, WidgetDialogActivity::class.java)
            val menuPendingIntent = PendingIntent.getActivity(
                context, 0, menuIntent,
                PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
            )
            views.setOnClickPendingIntent(R.id.btn_widget_menu, menuPendingIntent)

            val launchIntent = Intent(context, IntroActivity::class.java)
            val launchPendingIntent = PendingIntent.getActivity(
                context, 1, launchIntent,
                PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
            )
            views.setOnClickPendingIntent(R.id.btn_widget_launch, launchPendingIntent)

            CoroutineScope(Dispatchers.IO).launch {
                val db = GameDatabase.getDatabase(context)
                val state = db.gameStateDao().getGameState()
                val party = db.gameStateDao().getParty()

                if (state != null) {
                    val statusText = if (state.isSimulationActive) "[Engine: Active]" else "[Engine: Paused]"
                    val color = if (state.isSimulationActive) android.graphics.Color.parseColor("#4CAF50") else android.graphics.Color.parseColor("#FF5252")
                    
                    views.setTextViewText(R.id.tv_widget_engine_status, statusText)
                    views.setTextColor(R.id.tv_widget_engine_status, color)
                    
                    views.setTextViewText(R.id.tv_widget_biome, "Biome: ${state.currentBiome}")
                    views.setTextViewText(R.id.tv_widget_floor, "Floor: ${state.currentFloor}")
                    
                    if (party.isNotEmpty()) {
                        val hero = party[0]
                        views.setTextViewText(R.id.tv_widget_hero, "${hero.name} (${hero.characterClass})")
                        views.setTextViewText(R.id.tv_widget_hp, "HP: ${hero.currentHp}/${hero.maxHp}")
                    }
                }
                appWidgetManager.updateAppWidget(appWidgetId, views)
            }
        }
    }
}
