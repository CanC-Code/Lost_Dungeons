package com.dungeon.ui

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.dungeon.R
import com.dungeon.database.GameDatabase
import com.dungeon.ui.adapters.TeamAdapter
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class TeamActivity : AppCompatActivity() {
    private lateinit var db: GameDatabase
    private lateinit var teamAdapter: TeamAdapter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_team)

        db = GameDatabase.getDatabase(this)
        val rvTeam = findViewById<RecyclerView>(R.id.rv_team)
        teamAdapter = TeamAdapter(mutableListOf())
        rvTeam.layoutManager = LinearLayoutManager(this)
        rvTeam.adapter = teamAdapter

        loadTeam()
    }

    private fun loadTeam() {
        lifecycleScope.launch(Dispatchers.IO) {
            val team = db.gameStateDao().getParty()
            withContext(Dispatchers.Main) {
                teamAdapter.submitList(team)
            }
        }
    }
}