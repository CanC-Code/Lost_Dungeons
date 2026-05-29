package com.dungeon.ui.adapters

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.recyclerview.widget.RecyclerView
import com.dungeon.R
import com.dungeon.database.entities.PartyMemberEntity

class TeamAdapter(
    private var members: List<PartyMemberEntity>
) : RecyclerView.Adapter<TeamAdapter.ViewHolder>() {

    fun submitList(newMembers: List<PartyMemberEntity>) {
        members = newMembers
        notifyDataSetChanged()
    }

    class ViewHolder(view: View) : RecyclerView.ViewHolder(view) {
        val tvName: TextView = view.findViewById(R.id.tv_member_name)
        val tvClass: TextView = view.findViewById(R.id.tv_member_class)
        val tvHp: TextView = view.findViewById(R.id.tv_member_hp)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): ViewHolder {
        val view = LayoutInflater.from(parent.context)
            .inflate(R.layout.item_team_member, parent, false)
        return ViewHolder(view)
    }

    override fun onBindViewHolder(holder: ViewHolder, position: Int) {
        val member = members[position]
        holder.tvName.text = member.name
        holder.tvClass.text = member.characterClass
        holder.tvHp.text = "HP: ${member.currentHp}/${member.maxHp}"
    }

    override fun getItemCount() = members.size
}