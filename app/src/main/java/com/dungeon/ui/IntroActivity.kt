package com.dungeon.ui

import android.content.Intent
import android.os.Bundle
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.EditText
import android.widget.Spinner
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.dungeon.R
import com.dungeon.database.GameDatabase
import com.dungeon.database.entities.GameStateEntity
import com.dungeon.database.entities.PartyMemberEntity
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class IntroActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val db = GameDatabase.getDatabase(this)
        
        lifecycleScope.launch(Dispatchers.IO) {
            val state = db.gameStateDao().getGameState()
            val party = db.gameStateDao().getParty()
            
            // Ensure both state and party exist before skipping intro
            if (state != null && party.isNotEmpty()) {
                withContext(Dispatchers.Main) {
                    startActivity(Intent(this@IntroActivity, BattleActivity::class.java))
                    finish()
                }
            } else {
                withContext(Dispatchers.Main) {
                    setupIntroUI(db)
                }
            }
        }
    }

    private fun setupIntroUI(db: GameDatabase) {
        setContentView(R.layout.activity_intro)

        val etName = findViewById<EditText>(R.id.et_character_name)
        val spinClass = findViewById<Spinner>(R.id.spin_class)
        val spinPath = findViewById<Spinner>(R.id.spin_path)
        val btnStart = findViewById<Button>(R.id.btn_start_journey)

        val classes = arrayOf("Warrior (High HP/DEF)", "Mage (High MAG)", "Rogue (High ATK/SPD)", "Cleric (Healer)", "Ranger (Balanced)")
        spinClass.adapter = ArrayAdapter(this, android.R.layout.simple_spinner_dropdown_item, classes)

        val paths = arrayOf("Forest", "Mountains", "Caves", "Field")
        spinPath.adapter = ArrayAdapter(this, android.R.layout.simple_spinner_dropdown_item, paths)

        btnStart.setOnClickListener {
            val name = etName.text.toString()
            if (name.isBlank()) {
                Toast.makeText(this, "Please enter a name", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            val selectedClass = classes[spinClass.selectedItemPosition].split(" ")[0]
            val selectedPath = paths[spinPath.selectedItemPosition]
            seedDatabaseAndStart(db, name, selectedClass, selectedPath)
        }
    }

    private fun seedDatabaseAndStart(db: GameDatabase, name: String, charClass: String, path: String) {
        lifecycleScope.launch(Dispatchers.IO) {
            val dao = db.gameStateDao()
            
            var hp = 100; var atk = 10; var mag = 10; var def = 10; var eva = 5
            when (charClass) {
                "Warrior" -> { hp = 150; atk = 15; def = 20; eva = 2 }
                "Mage" -> { hp = 80; mag = 25; def = 5; eva = 5 }
                "Rogue" -> { hp = 90; atk = 25; def = 10; eva = 15 }
                "Cleric" -> { hp = 120; mag = 20; def = 15; eva = 5 }
                "Ranger" -> { hp = 110; atk = 18; mag = 12; def = 12; eva = 10 }
            }

            dao.updateGameState(GameStateEntity(
                lastSyncTimestamp = System.currentTimeMillis(),
                currentFloor = 1, currentBiome = path, isSimulationActive = true
            ))

            // FIX: Use insertPartyMember instead of updatePartyMember
            dao.insertPartyMember(PartyMemberEntity(
                name = name, characterClass = charClass, currentHp = hp, maxHp = hp,
                attackStat = atk, magicStat = mag, defenseStat = def, evasionStat = eva, isSelected = true
            ))

            withContext(Dispatchers.Main) {
                startActivity(Intent(this@IntroActivity, BattleActivity::class.java))
                finish()
            }
        }
    }
}
