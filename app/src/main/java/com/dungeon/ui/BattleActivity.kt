package com.dungeon.ui

import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.dungeon.R
import com.dungeon.database.GameDatabase
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class BattleActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_battle)

        val tvInfo = findViewById<TextView>(R.id.tv_environment_info)
        val tvLog = findViewById<TextView>(R.id.tv_combat_log)

        // Controller Buttons
        findViewById<Button>(R.id.btn_fight).setOnClickListener { tvLog.text = "> You swing your weapon!" }
        findViewById<Button>(R.id.btn_magic).setOnClickListener { tvLog.text = "> You cast a spell!" }
        findViewById<Button>(R.id.btn_item).setOnClickListener { tvLog.text = "> Checking inventory..." }
        findViewById<Button>(R.id.btn_run).setOnClickListener { tvLog.text = "> You attempt to flee..." }

        // Load active state
        val db = GameDatabase.getDatabase(this)
        lifecycleScope.launch(Dispatchers.IO) {
            val state = db.gameStateDao().getGameState()
            val party = db.gameStateDao().getParty()
            
            withContext(Dispatchers.Main) {
                if (state != null && party.isNotEmpty()) {
                    val hero = party[0]
                    tvInfo.text = "Path: ${state.currentBiome} | Floor: ${state.currentFloor} | ${hero.name} (${hero.characterClass})"
                }
            }
        }
    }
}
