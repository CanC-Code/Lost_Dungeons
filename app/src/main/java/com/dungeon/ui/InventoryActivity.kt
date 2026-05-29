package com.dungeon.ui

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.dungeon.R
import com.dungeon.database.GameDatabase
import com.dungeon.database.entities.InventoryItemEntity
import com.dungeon.ui.adapters.InventoryAdapter
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class InventoryActivity : AppCompatActivity() {
    private lateinit var db: GameDatabase
    private lateinit var inventoryAdapter: InventoryAdapter

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_inventory)

        db = GameDatabase.getDatabase(this)
        val rvInventory = findViewById<RecyclerView>(R.id.rv_inventory)
        inventoryAdapter = InventoryAdapter(mutableListOf())
        rvInventory.layoutManager = LinearLayoutManager(this)
        rvInventory.adapter = inventoryAdapter

        loadInventory()
    }

    private fun loadInventory() {
        lifecycleScope.launch(Dispatchers.IO) {
            val inventory = db.gameStateDao().getInventory()
            withContext(Dispatchers.Main) {
                inventoryAdapter.submitList(inventory)
            }
        }
    }
}