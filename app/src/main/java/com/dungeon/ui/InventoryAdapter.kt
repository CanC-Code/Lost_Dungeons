package com.dungeon.ui.adapters

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.dungeon.R
import com.dungeon.database.entities.InventoryItemEntity

class InventoryAdapter(
    private var items: List<InventoryItemEntity>
) : RecyclerView.Adapter<InventoryAdapter.ViewHolder>() {

    fun submitList(newItems: List<InventoryItemEntity>) {
        items = newItems
        notifyDataSetChanged()
    }

    class ViewHolder(view: View) : RecyclerView.ViewHolder(view) {
        val tvItemName: TextView = view.findViewById(R.id.tv_item_name)
        val tvQuantity: TextView = view.findViewById(R.id.tv_item_quantity)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_inventory, parent, false)
        return ViewHolder(view)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val item = items[position]
        holder.tvItemName.text = item.itemName
        holder.tvQuantity.text = "Qty: ${item.quantity}"
    }

    override fun getItemCount() = items.size
}