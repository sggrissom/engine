#if !defined(GAME_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Steven Grissom $
   ======================================================================== */

#include <sgg.h>
#include "sg_platform.h"
#include "sg_vector.h"
#include "sg_math.h"

struct loaded_bitmap
{
    s32 Width;
    s32 Height;
    u32 *Pixels;
};

struct sim_entity
{
    v2 dP;
    v2 ddP;
    r32 Height;
    r32 Width;
    v3 Color;
};

enum entity_type
{
    EntityType_Null,
    EntityType_Hero,
    EntityType_Wall,
};

struct entity
{
    v2 P;
    sim_entity SimEntity;
};

struct controlled_player
{
    u32 EntityIndex;
};

struct game_state
{
    loaded_bitmap Backdrop;

    controlled_player Players[ArrayCount(((game_input*)0)->Controllers)];

    u32 EntityCount;
    entity Entities[10000];
};

inline entity *
GetEntity(game_state *GameState, u32 EntityIndex)
{
    entity *Result = 0;

    if((EntityIndex > 0) && (EntityIndex < GameState->EntityCount))
    {
        Result = GameState->Entities + EntityIndex;
    }

    return Result;
}

inline controlled_player *
GetPlayer(game_state *GameState, u32 ControllerIndex)
{
    controlled_player *Result = 0;

    if((ControllerIndex >= 0) && (ControllerIndex < ArrayCount(GameState->Players)))
    {
        Result = GameState->Players + ControllerIndex;
        //Result = GetEntity(GameState, Player->EntityIndex);
    }

    return Result;
}

struct add_entity_result
{
    entity *Entity;
    u32 EntityIndex;
};

internal add_entity_result
AddEntity(game_state *GameState, entity_type Type, v3 Color = {}, v2 P = {})
{
    add_entity_result Result;
    
    Assert(GameState->EntityCount < ArrayCount(GameState->Entities));
    u32 EntityIndex = GameState->EntityCount++;

    entity *Entity = GameState->Entities + EntityIndex;
    *Entity = {};
   
    Entity->P = P;
    Entity->SimEntity.Color = Color;

    Result.Entity = Entity;
    Result.EntityIndex = EntityIndex;

    return Result;
}

internal add_entity_result
AddPlayer(game_state *GameState, u32 ControllerIndex, v2 P = v2{1.0f,1.0f})
{
    controlled_player *Player = GameState->Players + ControllerIndex;
    *Player = {};

    add_entity_result EntityResult = AddEntity(GameState, EntityType_Hero, {0,0,1}, P);

    Player->EntityIndex = EntityResult.EntityIndex;

    EntityResult.Entity->SimEntity.Width = 1.0f;
    EntityResult.Entity->SimEntity.Height = 1.0f;

    return EntityResult;
}

internal add_entity_result
AddWall(game_state *GameState, v2 P)
{
    add_entity_result Result = AddEntity(GameState, EntityType_Wall, v3{1.0f,0.0f,0.0f}, P);

    Result.Entity->SimEntity.Width = 0.5f;
    Result.Entity->SimEntity.Height = 0.5f;

    return Result;
}

#define GAME_H
#endif
