package com.dungeon.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.ProgressBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.dungeon.R
import com.dungeon.database.GameDatabase
import com.dungeon.database.entities.PartyMemberEntity
import kotlinx.coroutines.*
import kotlin.random.Random

class BattleActivity : AppCompatActivity() {

    private lateinit var db: GameDatabase
    private var activeHero: PartyMemberEntity? = null
    
    // Live Battle State
    private var isBattleActive = true
    private var enemyHp = 100
    private var enemyMaxHp = 100
    
    // Evasion Window Timing
    private var evadeActiveUntil: Long = 0
    private val evadeWindowMillis: Long = 1200 // 1.2 seconds to dodge
    private var isEvadeOnCooldown = false
    
    // UI References
    private lateinit var rvLog: RecyclerView
    private lateinit var pbPlayerHp: ProgressBar
    private lateinit var tvPlayerHpText: TextView
    private lateinit var pbEnemyHp: ProgressBar
    private lateinit var tvEnemyName: TextView
    
    // Evade Button References
    private lateinit var btnEvade: Button
    private lateinit var pbEvadeTimer: ProgressBar

    // Combat Log Data
    private val combatLogs = mutableListOf<String>()
    private lateinit var logAdapter: CombatLogAdapter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_battle)

        db = GameDatabase.getDatabase(this)
        
        rvLog = findViewById(R.id.rv_combat_log)
        pbPlayerHp = findViewById(R.id.pb_player_hp)
        tvPlayerHpText = findViewById(R.id.tv_player_hp_text)
        pbEnemyHp = findViewById(R.id.pb_enemy_hp)
        tvEnemyName = findViewById(R.id.tv_enemy_name)
        
        btnEvade = findViewById(R.id.btn_evade)
        pbEvadeTimer = findViewById(R.id.pb_evade_timer)

        val tvInfo = findViewById(R.id.tv_environment_info)

        // Setup RecyclerView for Logs
        logAdapter = CombatLogAdapter(combatLogs)
        rvLog.layoutManager = LinearLayoutManager(this).apply {
            stackFromEnd = true // Makes the list populate from the bottom up
        }
        rvLog.adapter = logAdapter

        // Setup Buttons
        findViewById<Button>(R.id.btn_fight).setOnClickListener { playerAttack(isMagic = false) }
        findViewById<Button>(R.id.btn_magic).setOnClickListener { playerAttack(isMagic = true) }
        findViewById<Button>(R.id.btn_item).setOnClickListener { logMsg("You have no items yet.") }
        findViewById<Button>(R.id.btn_run).setOnClickListener { attemptRun() }
        btnEvade.setOnClickListener { triggerEvade() }

        // --- Initialize Native Asset Manager ---
        // This links the Kotlin AssetManager to the C++ Engine for JSON parsing
        val controller = com.dungeon.engine.SimulationController()
        controller.nativeInitAssetManager(assets)
        // ---------------------------------------

        // Load Hero and Start Battle Loop
        lifecycleScope.launch(Dispatchers.IO) {
            val state = db.gameStateDao().getGameState()
            val party = db.gameStateDao().getParty()
            
            withContext(Dispatchers.Main) {
                if (state != null && party.isNotEmpty()) {
                    activeHero = party[0]
                    tvInfo.text = "Path: ${state.currentBiome} | Floor: ${state.currentFloor} | ${activeHero!!.name}"
                    updateHpUI()
                    spawnEnemy()
                    startEnemyAttackLoop()
                } else {
                    logMsg("Error loading character state.")
                }
            }
        }
    }

    private fun updateHpUI() {
        activeHero?.let { hero ->
            pbPlayerHp.max = hero.maxHp
            pbPlayerHp.progress = hero.currentHp
            tvPlayerHpText.text = "Hero HP: ${hero.currentHp}/${hero.maxHp}"
        }
        pbEnemyHp.max = enemyMaxHp
        pbEnemyHp.progress = enemyHp
    }

    private fun logMsg(msg: String) {
        combatLogs.add(msg)
        logAdapter.notifyItemInserted(combatLogs.size - 1)
        rvLog.scrollToPosition(combatLogs.size - 1)
    }

    private fun triggerEvade() {
        if (!isBattleActive || isEvadeOnCooldown) return
        
        isEvadeOnCooldown = true
        btnEvade.isEnabled = false
        evadeActiveUntil = System.currentTimeMillis() + evadeWindowMillis
        logMsg("You prepare to dodge! (Timing Window Active)")

        pbEvadeTimer.visibility = View.VISIBLE
        
        lifecycleScope.launch {
            val startTime = System.currentTimeMillis()
            var elapsed = 0L
            
            // 1. Tick down the visual window
            while (elapsed < evadeWindowMillis) {
                elapsed = System.currentTimeMillis() - startTime
                val progress = 100 - ((elapsed.toFloat() / evadeWindowMillis) * 100).toInt()
                pbEvadeTimer.progress = progress
                delay(16) // roughly 60fps
            }
            
            pbEvadeTimer.visibility = View.INVISIBLE
            logMsg("Evade stance ended. Recovering...")
            
            // 2. Cooldown phase
            delay(2500)
            btnEvade.isEnabled = true
            isEvadeOnCooldown = false
            logMsg("Evade is ready!")
        }
    }

    private fun playerAttack(isMagic: Boolean) {
        if (!isBattleActive || activeHero == null) return
        
        val hero = activeHero!!
        val damage = if (isMagic) hero.magicStat + Random.nextInt(0, 5) else hero.attackStat + Random.nextInt(0, 5)
        
        enemyHp -= damage
        if (enemyHp < 0) enemyHp = 0
        
        logMsg(if (isMagic) "You cast a spell for $damage DMG!" else "You strike for $damage DMG!")
        updateHpUI()

        if (enemyHp <= 0) winBattle()
    }

    private fun attemptRun() {
        if (!isBattleActive) return
        if (Random.nextBoolean()) {
            logMsg("Got away safely! Moving forward...")
            spawnEnemy()
        } else {
            logMsg("Failed to escape!")
        }
    }

    private fun startEnemyAttackLoop() {
        lifecycleScope.launch {
            while (isActive) {
                delay(3000) // Enemy attacks every 3 seconds
                if (isBattleActive && activeHero != null) {
                    
                    val now = System.currentTimeMillis()
                    if (now <= evadeActiveUntil) {
                        logMsg("PERFECT DODGE! Negated enemy attack!")
                        trainEvasion()
                    } else {
                        val hero = activeHero!!
                        val dodgeRoll = Random.nextInt(0, 100)
                        if (dodgeRoll < hero.evasionStat) {
                            logMsg("Your quick reflexes passively dodged the attack!")
                        } else {
                            val dmg = Random.nextInt(5, 15)
                            val newHp = (hero.currentHp - dmg).coerceAtLeast(0)
                            activeHero = hero.copy(currentHp = newHp)
                            logMsg("The enemy hits you for $dmg damage.")
                            updateHpUI()
                            saveHeroState()

                            if (newHp == 0) {
                                isBattleActive = false
                                logMsg("YOU DIED. Game Over.")
                            }
                        }
                    }
                }
            }
        }
    }

    private fun trainEvasion() {
        activeHero?.let { hero ->
            val updatedHero = hero.copy(evasionStat = hero.evasionStat + 1)
            activeHero = updatedHero
            logMsg("Evade leveled up! Evasion Stat is now ${updatedHero.evasionStat}!")
            saveHeroState()
        }
    }

    private fun saveHeroState() {
        activeHero?.let { hero ->
            lifecycleScope.launch(Dispatchers.IO) {
                db.gameStateDao().updatePartyMember(hero)
            }
        }
    }

    private fun winBattle() {
        isBattleActive = false
        logMsg("Victory! Enemy defeated.")
        
        lifecycleScope.launch {
            delay(1500)
            spawnEnemy()
        }
    }

    private fun spawnEnemy() {
        enemyMaxHp = Random.nextInt(40, 90)
        enemyHp = enemyMaxHp
        tvEnemyName.text = listOf("Goblin", "Slime", "Wolf", "Skeleton").random()
        logMsg("A wild ${tvEnemyName.text} appears!")
        isBattleActive = true
        updateHpUI()
    }

    // --- Inner Adapter Class for RecyclerView ---
    inner class CombatLogAdapter(private val logs: List<String>) : RecyclerView.Adapter<CombatLogAdapter.ViewHolder>() {
        inner class ViewHolder(view: View) : RecyclerView.ViewHolder(view) {
            val tvText: TextView = view.findViewById(R.id.tv_log_text)
        }
        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val view = LayoutInflater.from(parent.context).inflate(R.layout.item_combat_log, parent, false)
            return ViewHolder(view)
        }
        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            holder.tvText.text = "> ${logs[position]}"
        }
        override fun getItemCount() = logs.size
    }
}
