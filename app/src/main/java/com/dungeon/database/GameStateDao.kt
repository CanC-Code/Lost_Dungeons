package com.dungeon.database

import androidx.room.*
import com.dungeon.database.entities.*

@Dao
interface GameStateDao {
    @Query("SELECT * FROM game_state LIMIT 1")
    fun getGameState(): GameStateEntity?

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun updateGameState(state: GameStateEntity)

    @Query("SELECT * FROM party_members")
    fun getParty(): List<PartyMemberEntity>

    // NEW: Required to create a character in an empty database
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun insertPartyMember(member: PartyMemberEntity)

    @Update
    fun updatePartyMember(member: PartyMemberEntity)

    @Query("SELECT * FROM inventory")
    fun getInventory(): List<InventoryItemEntity>
}
