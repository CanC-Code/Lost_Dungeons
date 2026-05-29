package com.dungeon.ui

import android.annotation.SuppressLint
import android.content.Intent
import android.os.Bundle
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.cardview.widget.CardView
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.dungeon.R
import com.dungeon.database.GameDatabase
import com.dungeon.database.entities.PartyMemberEntity
import com.dungeon.engine.SimulationController
import kotlinx.coroutines.*
import kotlin.math.sqrt
import kotlin.math.min
import kotlin.random.Random

enum class GameState { OVERWORLD, BATTLE, MENU }

class BattleActivity : AppCompatActivity() {

    private lateinit var db: GameDatabase
    private lateinit var controller: SimulationController
    private var activeHero: PartyMemberEntity? = null

    private var currentState = GameState.OVERWORLD
    private var isBattleActive = false
    private var enemyHp = 100
    private var enemyMaxHp = 100
    private var currentEnemyName = ""

    private var evadeActiveUntil: Long = 0
    private val evadeWindowMillis: Long = 1200
    private var isEvadeOnCooldown = false

    // UI References
    private lateinit var touchOverlay: FrameLayout
    private lateinit var layoutBattle: LinearLayout
    private lateinit var layoutEnemyStatus: LinearLayout
    private lateinit var tvInfo: TextView
    private lateinit var btnIngameMenu: View

    // Joystick References
    private lateinit var joystickBase: CardView
    private lateinit var joystickThumb: CardView
    private var leftPointerId = -1
    private var rightPointerId = -1
    private var lastRightX = 0f
    private var lastRightY = 0f
    private var joyBaseX = 0f
    private var joyBaseY = 0f
    private var moveInputX = 0f
    private var moveInputY = 0f
    private var inputLoopJob: Job? = null

    // Battle HUD
    private lateinit var rvLog: RecyclerView
    private lateinit var pbPlayerHp: ProgressBar
    private lateinit var tvPlayerHpText: TextView
    private lateinit var pbEnemyHp: ProgressBar
    private lateinit var tvEnemyName: TextView
    private lateinit var btnEvade: Button
    private lateinit var pbEvadeTimer: ProgressBar

    // Menu Buttons
    private lateinit var btnInventory: Button
    private lateinit var btnTeam: Button

    private val combatLogs = mutableListOf<String>()
    private lateinit var logAdapter: CombatLogAdapter
    private var attackJob: Job? = null

    @SuppressLint("ClickableViewAccessibility")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        supportActionBar?.hide()
        setContentView(R.layout.activity_battle)

        db = GameDatabase.getDatabase(this)
        controller = SimulationController()

        // Initialize UI References
        touchOverlay = findViewById(R.id.touch_overlay)
        layoutBattle = findViewById(R.id.layout_battle_hud)
        layoutEnemyStatus = findViewById(R.id.layout_enemy_status)
        tvInfo = findViewById(R.id.tv_environment_info)
        btnIngameMenu = findViewById(R.id.btn_ingame_menu)

        joystickBase = findViewById(R.id.joystick_base)
        joystickThumb = findViewById(R.id.joystick_thumb)

        rvLog = findViewById(R.id.rv_combat_log)
        pbPlayerHp = findViewById(R.id.pb_player_hp)
        tvPlayerHpText = findViewById(R.id.tv_player_hp_text)
        pbEnemyHp = findViewById(R.id.pb_enemy_hp)
        tvEnemyName = findViewById(R.id.tv_enemy_name)
        btnEvade = findViewById(R.id.btn_evade)
        pbEvadeTimer = findViewById(R.id.pb_evade_timer)

        // Menu Buttons
        btnInventory = findViewById(R.id.btn_inventory)
        btnTeam = findViewById(R.id.btn_team)

        // Set up adapters
        logAdapter = CombatLogAdapter(combatLogs)
        rvLog.layoutManager = LinearLayoutManager(this).apply { stackFromEnd = true }
        rvLog.adapter = logAdapter

        // Battle Buttons
        findViewById<Button>(R.id.btn_fight).setOnClickListener { playerAttack(isMagic = false) }
        findViewById<Button>(R.id.btn_magic).setOnClickListener { playerAttack(isMagic = true) }
        findViewById<Button>(R.id.btn_item).setOnClickListener { logMsg("You have no items yet.") }
        findViewById<Button>(R.id.btn_run).setOnClickListener { attemptRun() }
        btnEvade.setOnClickListener { triggerEvade() }

        // Menu Buttons
        btnInventory.setOnClickListener {
            startActivity(Intent(this, InventoryActivity::class.java))
        }
        btnTeam.setOnClickListener {
            startActivity(Intent(this, TeamActivity::class.java))
        }

        // In-game menu toggle
        btnIngameMenu.setOnClickListener {
            if (currentState == GameState.MENU) {
                currentState = GameState.OVERWORLD
                controller.nativeSetGameState(0, "")
                tvInfo.visibility = View.GONE
            } else if (currentState == GameState.OVERWORLD) {
                currentState = GameState.MENU
                controller.nativeSetGameState(2, "")
                tvInfo.visibility = View.VISIBLE
            }
        }

        // Touch input handling
        touchOverlay.setOnTouchListener { v, event ->
            if (currentState == GameState.BATTLE) return@setOnTouchListener false

            val action = event.actionMasked
            val pointerIndex = event.actionIndex
            val pointerId = event.getPointerId(pointerIndex)
            val x = event.getX(pointerIndex)
            val y = event.getY(pointerIndex)

            val overlayHalfWidth = v.width / 2f
            val isLeftSide = x < overlayHalfWidth

            when (action) {
                MotionEvent.ACTION_DOWN, MotionEvent.ACTION_POINTER_DOWN -> {
                    if (isLeftSide && leftPointerId == -1) {
                        leftPointerId = pointerId
                        joyBaseX = x
                        joyBaseY = y

                        joystickBase.x = joyBaseX - (joystickBase.width / 2f)
                        joystickBase.y = joyBaseY - (joystickBase.height / 2f)
                        joystickThumb.x = x - (joystickThumb.width / 2f)
                        joystickThumb.y = y - (joystickThumb.height / 2f)

                        joystickBase.visibility = View.VISIBLE
                        joystickThumb.visibility = View.VISIBLE
                        startInputLoop()
                    } else if (!isLeftSide && rightPointerId == -1) {
                        rightPointerId = pointerId
                        lastRightX = x
                        lastRightY = y
                    }
                }

                MotionEvent.ACTION_MOVE -> {
                    for (i in 0 until event.pointerCount) {
                        val pId = event.getPointerId(i)
                        val curX = event.getX(i)
                        val curY = event.getY(i)

                        if (pId == leftPointerId) {
                            val dx = curX - joyBaseX
                            val dy = curY - joyBaseY
                            val distance = sqrt((dx * dx + dy * dy).toDouble()).toFloat()
                            val maxRadius = joystickBase.width / 2f

                            if (maxRadius <= 0f) return@setOnTouchListener true

                            val ratio = min(1f, maxRadius / distance.coerceAtLeast(0.001f))
                            val thumbX = if (distance > maxRadius) joyBaseX + dx * ratio else curX
                            val thumbY = if (distance > maxRadius) joyBaseY + dy * ratio else curY

                            joystickThumb.x = thumbX - (joystickThumb.width / 2f)
                            joystickThumb.y = thumbY - (joystickThumb.height / 2f)

                            moveInputX = (thumbX - joyBaseX) / maxRadius
                            moveInputY = -((thumbY - joyBaseY) / maxRadius)
                        } else if (pId == rightPointerId) {
                            val deltaX = curX - lastRightX
                            val deltaY = curY - lastRightY
                            lastRightX = curX
                            lastRightY = curY
                            controller.nativeUpdateInput(0f, 0f, deltaX, deltaY)
                        }
                    }
                }

                MotionEvent.ACTION_UP, MotionEvent.ACTION_POINTER_UP, MotionEvent.ACTION_CANCEL -> {
                    if (pointerId == leftPointerId) {
                        leftPointerId = -1
                        moveInputX = 0f
                        moveInputY = 0f
                        joystickBase.visibility = View.INVISIBLE
                        joystickThumb.visibility = View.INVISIBLE
                        stopInputLoop()
                    } else if (pointerId == rightPointerId) {
                        rightPointerId = -1
                    }
                }
            }
            true
        }

        // Initialize native renderer
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

        // Load game state
        lifecycleScope.launch(Dispatchers.IO) {
            val state = db.gameStateDao().getGameState()
            val party = db.gameStateDao().getParty()

            withContext(Dispatchers.Main) {
                if (state != null && party.isNotEmpty()) {
                    activeHero = party[0]
                    tvInfo.text = "Path: ${state.currentBiome} | Floor: ${state.currentFloor} | ${activeHero!!.name}"
                    tvInfo.visibility = View.GONE
                    updateHpUI()
                    switchToOverworld()
                } else {
                    tvInfo.text = "Error loading character state."
                    tvInfo.visibility = View.VISIBLE
                }
            }
        }
    }

    // --- Input Loop for Movement ---
    private fun startInputLoop() {
        if (inputLoopJob?.isActive == true) return
        inputLoopJob = lifecycleScope.launch(Dispatchers.Default) {
            while (isActive && leftPointerId != -1) {
                controller.nativeUpdateInput(moveInputX, moveInputY, 0f, 0f)
                if (moveInputX != 0f || moveInputY != 0f) {
                    if (Random.nextInt(1000) < 5) {
                        withContext(Dispatchers.Main) { switchToBattle() }
                    }
                }
                delay(16)
            }
        }
    }

    private fun stopInputLoop() {
        inputLoopJob?.cancel()
    }

    // --- Game State Management ---
    private fun switchToOverworld() {
        currentState = GameState.OVERWORLD
        isBattleActive = false
        attackJob?.cancel()

        layoutBattle.visibility = View.GONE
        layoutEnemyStatus.visibility = View.GONE
        touchOverlay.visibility = View.VISIBLE
        tvInfo.visibility = View.GONE
        controller.nativeSetGameState(0, "")
    }

    private fun switchToBattle() {
        currentState = GameState.BATTLE
        touchOverlay.visibility = View.GONE
        layoutBattle.visibility = View.VISIBLE
        layoutEnemyStatus.visibility = View.VISIBLE
        tvInfo.visibility = View.GONE

        leftPointerId = -1
        joystickBase.visibility = View.INVISIBLE
        joystickThumb.visibility = View.INVISIBLE
        stopInputLoop()

        combatLogs.clear()
        logAdapter.notifyDataSetChanged()
        spawnEnemy()
        controller.nativeSetGameState(1, currentEnemyName)
        startEnemyAttackLoop()
    }

    private fun spawnEnemy() {
        enemyMaxHp = Random.nextInt(40, 90)
        enemyHp = enemyMaxHp
        currentEnemyName = listOf("Goblin", "Slime", "Dire Wolf", "Orc").random()
        tvEnemyName.text = currentEnemyName
        logMsg("A wild $currentEnemyName ambushes you!")
        isBattleActive = true
        updateHpUI()
    }

    // --- Combat Logic ---
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
        val damage = if (isMagic) hero.magicStat + Random.nextInt(0, 5)
                   else hero.attackStat + Random.nextInt(0, 5)
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

    // --- Evade Mechanics ---
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

    // --- UI Updates ---
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
        activeHero?.let {
            lifecycleScope.launch(Dispatchers.IO) {
                db.gameStateDao().updatePartyMember(it)
            }
        }
    }

    // --- Combat Log Adapter ---
    inner class CombatLogAdapter(private val logs: List<String>) :
        RecyclerView.Adapter<CombatLogAdapter.ViewHolder>() {

        inner class ViewHolder(view: View) : RecyclerView.ViewHolder(view) {
            val tvText: TextView = view.findViewById(R.id.tv_log_text)
        }

        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
            val view = LayoutInflater.from(parent.context)
                .inflate(R.layout.item_combat_log, parent, false)
            return ViewHolder(view)
        }

        override fun onBindViewHolder(holder: ViewHolder, position: Int) {
            holder.tvText.text = "> ${logs[position]}"
        }

        override fun getItemCount() = logs.size
    }
}