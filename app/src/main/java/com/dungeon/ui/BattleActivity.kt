package com.dungeon.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.RelativeLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.dungeon.R
import com.dungeon.database.GameDatabase
import com.dungeon.database.entities.PartyMemberEntity
import com.dungeon.engine.SimulationController
import kotlinx.coroutines.*
import kotlin.random.Random

enum class GameState { OVERWORLD, BATTLE }

class BattleActivity : AppCompatActivity() {

    private lateinit var db: GameDatabase
    private var activeHero: PartyMemberEntity? = null
    
    // Core Game State
    private var currentState = GameState.OVERWORLD
    private var isBattleActive = false
    private var enemyHp = 100
    private var enemyMaxHp = 100
    
    // Evasion Window Timing
    private var evadeActiveUntil: Long = 0
    private val evadeWindowMillis: Long = 1200
    private var isEvadeOnCooldown = false
    
    // UI Layout References
    private lateinit var layoutOverworld: RelativeLayout
    private lateinit var layoutBattle: LinearLayout
    private lateinit var layoutEnemyStatus: LinearLayout
    private lateinit var tvInfo: TextView
    
    // Battle HUD References
    private lateinit var rvLog: RecyclerView
    private lateinit var pbPlayerHp: ProgressBar
    private lateinit var tvPlayerHpText: TextView
    private lateinit var pbEnemyHp: ProgressBar
    private lateinit var tvEnemyName: TextView
    private lateinit var btnEvade: Button
    private lateinit var pbEvadeTimer: ProgressBar

    // Combat Log Data
    private val combatLogs = mutableListOf<String>()
    private lateinit var logAdapter: CombatLogAdapter
    
    private var attackJob: Job? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_battle)

        db = GameDatabase.getDatabase(this)
        
        // Map HUD Elements
        layoutOverworld = findViewById(R.id.layout_overworld_hud)
        layoutBattle = findViewById(R.id.layout_battle_hud)
        layoutEnemyStatus = findViewById(R.id.layout_enemy_status)
        tvInfo = findViewById(R.id.tv_environment_info)
        
        // Map Battle Elements
        rvLog = findViewById(R.id.rv_combat_log)
        pbPlayerHp = findViewById(R.id.pb_player_hp)
        tvPlayerHpText = findViewById(R.id.tv_player_hp_text)
        pbEnemyHp = findViewById(R.id.pb_enemy_hp)
        tvEnemyName = findViewById(R.id.tv_enemy_name)
        btnEvade = findViewById(R.id.btn_evade)
        pbEvadeTimer = findViewById(R.id.pb_evade_timer)

        // Setup RecyclerView
        logAdapter = CombatLogAdapter(combatLogs)
        rvLog.layoutManager = LinearLayoutManager(this).apply { stackFromEnd = true }
        rvLog.adapter = logAdapter

        // Setup Overworld Movement Buttons
        findViewById<Button>(R.id.btn_move_up).setOnClickListener { move("North") }
        findViewById<Button>(R.id.btn_move_down).setOnClickListener { move("South") }
        findViewById<Button>(R.id.btn_move_left).setOnClickListener { move("West") }
        findViewById<Button>(R.id.btn_move_right).setOnClickListener { move("East") }

        // Setup Battle Action Buttons
        findViewById<Button>(R.id.btn_fight).setOnClickListener { playerAttack(isMagic = false) }
        findViewById<Button>(R.id.btn_magic).setOnClickListener { playerAttack(isMagic = true) }
        findViewById<Button>(R.id.btn_item).setOnClickListener { logMsg("You have no items yet.") }
        findViewById<Button>(R.id.btn_run).setOnClickListener { attemptRun() }
        btnEvade.setOnClickListener { triggerEvade() }

        // --- NATIVE ENGINE INITIALIZATION ---
        val controller = SimulationController()
        controller.nativeInitAssetManager(assets)
        
        val renderSurface = findViewById<SurfaceView>(R.id.render_surface)
        renderSurface.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                controller.nativeSurfaceCreated(holder.surface)
            }
            override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
                controller.nativeSurfaceChanged(width, height)
            }
            override fun surfaceDestroyed(holder: SurfaceHolder) {
                controller.nativeSurfaceDestroyed()
            }
        })

        // Load Hero and initialize Overworld
        lifecycleScope.launch(Dispatchers.IO) {
            val state = db.gameStateDao().getGameState()
            val party = db.gameStateDao().getParty()
            
            withContext(Dispatchers.Main) {
                if (state != null && party.isNotEmpty()) {
                    activeHero = party[0]
                    tvInfo.text = "Path: ${state.currentBiome} | Floor: ${state.currentFloor} | ${activeHero!!.name}"
                    updateHpUI()
                    switchToOverworld()
                } else {
                    tvInfo.text = "Error loading character state."
                }
            }
        }
    }

    // --- State Machine ---
    private fun switchToOverworld() {
        currentState = GameState.OVERWORLD
        isBattleActive = false
        attackJob?.cancel() // Stop the enemy attack loop
        
        layoutBattle.visibility = View.GONE
        layoutEnemyStatus.visibility = View.GONE
        layoutOverworld.visibility = View.VISIBLE
    }

    private fun switchToBattle() {
        currentState = GameState.BATTLE
        
        layoutOverworld.visibility = View.GONE
        layoutBattle.visibility = View.VISIBLE
        layoutEnemyStatus.visibility = View.VISIBLE
        
        combatLogs.clear()
        logAdapter.notifyDataSetChanged()
        
        spawnEnemy()
        startEnemyAttackLoop()
    }

    // --- Overworld Logic ---
    private fun move(direction: String) {
        if (currentState != GameState.OVERWORLD) return
        
        // TODO: In Phase 4, Step 3, we will send this command to C++ GLM Matrix
        // controller.nativeMoveCamera(dx, dy)
        
        // 25% chance of random encounter per step
        if (Random.nextInt(100) < 25) {
            switchToBattle()
        }
    }

    // --- Battle Logic ---
    private fun spawnEnemy() {
        enemyMaxHp = Random.nextInt(40, 90)
        enemyHp = enemyMaxHp
        tvEnemyName.text = listOf("Goblin", "Slime", "Wolf", "Skeleton").random()
        logMsg("A wild ${tvEnemyName.text} ambushes you!")
        isBattleActive = true
        updateHpUI()
    }

    private fun startEnemyAttackLoop() {
        attackJob?.cancel()
        attackJob = lifecycleScope.launch {
            while (isActive && isBattleActive) {
                delay(3000)
                if (isBattleActive && activeHero != null) {
                    val now = System.currentTimeMillis()
                    if (now <= evadeActiveUntil) {
                        logMsg("PERFECT DODGE! Negated enemy attack!")
                        trainEvasion()
                    } else {
                        val hero = activeHero!!
                        if (Random.nextInt(0, 100) < hero.evasionStat) {
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

    private fun playerAttack(isMagic: Boolean) {
        if (!isBattleActive || activeHero == null) return
        val hero = activeHero!!
        val damage = if (isMagic) hero.magicStat + Random.nextInt(0, 5) else hero.attackStat + Random.nextInt(0, 5)
        enemyHp = (enemyHp - damage).coerceAtLeast(0)
        
        logMsg(if (isMagic) "You cast a spell for $damage DMG!" else "You strike for $damage DMG!")
        updateHpUI()
        
        if (enemyHp <= 0) winBattle()
    }

    private fun attemptRun() {
        if (!isBattleActive) return
        if (Random.nextBoolean()) {
            logMsg("Got away safely! Returning to exploration...")
            lifecycleScope.launch { delay(1000); switchToOverworld() }
        } else {
            logMsg("Failed to escape!")
        }
    }

    private fun winBattle() {
        isBattleActive = false
        logMsg("Victory! Enemy defeated.")
        lifecycleScope.launch { delay(1500); switchToOverworld() }
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
            while (elapsed < evadeWindowMillis) {
                elapsed = System.currentTimeMillis() - startTime
                val progress = 100 - ((elapsed.toFloat() / evadeWindowMillis) * 100).toInt()
                pbEvadeTimer.progress = progress
                delay(16)
            }
            pbEvadeTimer.visibility = View.INVISIBLE
            logMsg("Evade stance ended. Recovering...")
            delay(2500)
            btnEvade.isEnabled = true
            isEvadeOnCooldown = false
            logMsg("Evade is ready!")
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

    private fun trainEvasion() {
        activeHero?.let { hero ->
            val updatedHero = hero.copy(evasionStat = hero.evasionStat + 1)
            activeHero = updatedHero
            logMsg("Evade leveled up! Evasion Stat is now ${updatedHero.evasionStat}!")
            saveHeroState()
        }
    }

    private fun saveHeroState() {
        activeHero?.let { lifecycleScope.launch(Dispatchers.IO) { db.gameStateDao().updatePartyMember(it) } }
    }

    // --- RecyclerView Adapter ---
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
