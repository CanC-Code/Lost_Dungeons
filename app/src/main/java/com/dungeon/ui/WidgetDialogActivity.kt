package com.dungeon.ui

import android.appwidget.AppWidgetManager
import android.content.ComponentName
import android.content.Intent
import android.os.Bundle
import android.widget.Button
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import androidx.room.Room
import com.dungeon.R
import com.dungeon.database.GameDatabase
import com.dungeon.ui.adapters.PartySelectionAdapter
import com.dungeon.widget.DungeonWidgetProvider
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class WidgetDialogActivity : AppCompatActivity() {

    private lateinit var db: GameDatabase
    private lateinit var partyAdapter: PartySelectionAdapter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_widget_dialog)

        db = Room.databaseBuilder(
            applicationContext,
            GameDatabase::class.java, "dungeon-db"
        ).build()

        val btnToggleSim = findViewById<Button>(R.id.btn_toggle_sim)
        val btnClose = findViewById<Button>(R.id.btn_close)
        val rvParty = findViewById<RecyclerView>(R.id.rv_party_members)

        partyAdapter = PartySelectionAdapter()
        rvParty.layoutManager = LinearLayoutManager(this)
        rvParty.adapter = partyAdapter

        loadData(btnToggleSim)

        btnToggleSim.setOnClickListener {
            toggleSimulation(btnToggleSim)
        }

        btnClose.setOnClickListener {
            refreshWidgetAndClose()
        }
    }

    private fun loadData(toggleBtn: Button) {
        lifecycleScope.launch(Dispatchers.IO) {
            val state = db.gameStateDao().getGameState()
            val party = db.gameStateDao().getParty()

            withContext(Dispatchers.Main) {
                if (state != null) {
                    toggleBtn.text = if (state.isSimulationActive) "Pause Simulation" else "Resume Simulation"
                }
                partyAdapter.submitList(party)
            }
        }
    }

    private fun toggleSimulation(toggleBtn: Button) {
        lifecycleScope.launch(Dispatchers.IO) {
            val state = db.gameStateDao().getGameState()
            if (state != null) {
                val newState = state.copy(isSimulationActive = !state.isSimulationActive)
                db.gameStateDao().updateGameState(newState)
                withContext(Dispatchers.Main) {
                    toggleBtn.text = if (newState.isSimulationActive) "Pause Simulation" else "Resume Simulation"
                }
            }
        }
    }

    private fun refreshWidgetAndClose() {
        // Triggers the widget provider to redraw itself with the newest database state
        val intent = Intent(this, DungeonWidgetProvider::class.java)
        intent.action = AppWidgetManager.ACTION_APPWIDGET_UPDATE
        val ids = AppWidgetManager.getInstance(application).getAppWidgetIds(
            ComponentName(application, DungeonWidgetProvider::class.java)
        )
        intent.putExtra(AppWidgetManager.EXTRA_APPWIDGET_IDS, ids)
        sendBroadcast(intent)
        finish()
    }
}
