package com.dungeon.database.entities

import androidx.room.Entity
import androidx.room.PrimaryKey

@Entity(tableName = "party_members")
data class PartyMemberEntity(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    val name: String,
    val currentHp: Int,
    val maxHp: Int,
    val attackStat: Int,
    val isSelected: Boolean
)
