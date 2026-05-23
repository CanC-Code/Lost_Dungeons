package com.dungeon.ui

import android.os.Bundle
import android.widget.Button
import android.widget.ProgressBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
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
    private val evadeWindowMillis: Long = 1200 // 1.2 seconds to dodge successfully
    
    // UI References
    private lateinit var tvLog: TextView
    private lateinit var pbPlayerHp: ProgressBar
    private lateinit var tvPlayerHpText: TextView
    private lateinit var pbEnemyHp: ProgressBar
    private lateinit var tvEnemyName: TextView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_battle)

        db = GameDatabase.getDatabase(this)
        
        tvLog = findViewById(R.id.tv_combat_log)
        pbPlayerHp = findViewById(R.id.pb_player_hp)
        tvPlayerHpText = findViewById(R.id.tv_player_hp_text)
        pbEnemyHp = findViewById(R.id.pb_enemy_hp)
        tvEnemyName = findViewById(R.id.tv_enemy_name)

        val tvInfo = findViewById<TextView>(R.id.tv_environment_info)

        // Setup Buttons
        findViewById<Button>(R.id.btn_fight).setOnClickListener { playerAttack(isMagic = false) }
        findViewById<Button>(R.id.btn_magic).setOnClickListener { playerAttack(isMagic = true) }
        findViewById<Button>(R.id.btn_item).setOnClickListener { logMsg("You have no items yet.") }
        findViewById<Button>(R.id.btn_run).setOnClickListener { attemptRun() }
        findViewById<Button>(R.id.btn_evade).setOnClickListener { triggerEvade() }

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
        tvLog.text = "> $msg"
    }

    private fun triggerEvade() {
        if (!isBattleActive) return
        evadeActiveUntil = System.currentTimeMillis() + evadeWindowMillis
        logMsg("You prepare to dodge! (Timing Window Active)")
    }

    private fun playerAttack(isMagic: Boolean) {
        if (!isBattleActive || activeHero == null) return
        
        val hero = activeHero!!
        val damage = if (isMagic) hero.magicStat + Random.nextInt(0, 5) else hero.attackStat + Random.nextInt(0, 5)
        
        enemyHp -= damage
        if (enemyHp < 0) enemyHp = 0
        
        logMsg(if (isMagic) "You cast a spell for $damage DMG!" else "You strike for $damage DMG!")
        updateHpUI()

        if (enemyHp <= 0) {
            winBattle()
        }
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
                    // Check if player perfectly timed the Evade button
                    if (now <= evadeActiveUntil) {
                        logMsg("PERFECT DODGE! Negated enemy attack!")
                        trainEvasion()
                    } else {
                        // Resolve hit
                        val hero = activeHero!!
                        // Base chance to passively dodge based on trained evasion stat
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
            logMsg("PERFECT DODGE! Evasion Stat increased to ${updatedHero.evasionStat}!")
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
}
