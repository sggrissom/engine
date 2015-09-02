#if !defined(SG_TILE_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Steven Grissom $
   ======================================================================== */

// TODO(casey): Think about what the real safe margin is!
#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define TILE_CHUNK_UNINITIALIZED INT32_MAX

typedef tile_map_difference v3;

struct tile_map_position
{
    v3_s32 Tile;
    v2 Offset;
};

struct tile_chunk_position
{
    v3_s32 ChunkCoords;

    v2_s32 Tile;
};

struct tile_chunk
{
    v3_s32 WorldCoords;

    tile *Tiles;
};

struct tile_map
{
    s32 ChunkShift;
    s32 ChunkMask;
    s32 ChunkDim;

    r32 TileSideInMeters;

    tile_chunk TileChunkHash[4096];
};

inline tile_chunk *
GetTileChunk(tile_map *TileMap, v3_s32 ChunkCoords, memory_arena *Arena = 0)
{
    Assert(ChunkCoords.x > -TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkCoords.y > -TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkCoords.z > -TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkCoords.x < TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkCoords.y < TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkCoords.z < TILE_CHUNK_SAFE_MARGIN);
    
    // TODO(steven): BETTER HASH FUNCTION!
    u32 HashValue = 19*ChunkCoords.x + 7*ChunkCoords.y + 3*ChunkCoords.z;
    u32 HashSlot = HashValue & (ArrayCount(TileMap->TileChunkHash) - 1);
    Assert(HashSlot < ArrayCount(TileMap->TileChunkHash));
    
    tile_chunk *Chunk = TileMap->TileChunkHash + HashSlot;
    do
    {
        if(ChunkCoords == Chunk->ChunkCoords)
        {            
            break;
        }

        if(Arena && (Chunk->TileChunkX != TILE_CHUNK_UNINITIALIZED) && (!Chunk->NextInHash))
        {
            Chunk->NextInHash = PushStruct(Arena, tile_chunk);
            Chunk = Chunk->NextInHash;
            Chunk->TileChunkX = TILE_CHUNK_UNINITIALIZED;
        }
        
        if(Arena && (Chunk->TileChunkX == TILE_CHUNK_UNINITIALIZED))
        {
            uint32 TileCount = TileMap->ChunkDim*TileMap->ChunkDim;

            Chunk->Chunk = Chunk;
            
            Chunk->Tiles = PushArray(Arena, TileCount, uint32);
            // TODO(casey): Do we want to always initialize?
            for(uint32 TileIndex = 0;
                TileIndex < TileCount;
                ++TileIndex)
            {
                Chunk->Tiles[TileIndex] = 1;
            }

            Chunk->NextInHash = 0;

            break;
        }

        ++Chunk;
    } while(Chunk);
    
    return(Chunk);
}

inline uint32
GetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, int32 TileX, int32 TileY)
{
    Assert(TileChunk);
    Assert(TileX < TileMap->ChunkDim);
    Assert(TileY < TileMap->ChunkDim);
    
    uint32 TileChunkValue = TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX];
    return(TileChunkValue);
}

inline void
SetTileValueUnchecked(tile_map *TileMap, tile_chunk *TileChunk, int32 TileX, int32 TileY,
                      uint32 TileValue)
{
    Assert(TileChunk);
    Assert(TileX < TileMap->ChunkDim);
    Assert(TileY < TileMap->ChunkDim);
    
    TileChunk->Tiles[TileY*TileMap->ChunkDim + TileX] = TileValue;
}

inline void
SetTileValue(tile_map *TileMap, tile_chunk *TileChunk,
             uint32 TestTileX, uint32 TestTileY, uint32 TileValue)
{
    if(TileChunk && TileChunk->Tiles)
    {
        SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);
    }
}

inline tile_chunk_position
GetChunkPositionFor(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    tile_chunk_position Result;

    Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
    Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
    Result.TileChunkZ = AbsTileZ;
    Result.RelTileX = AbsTileX & TileMap->ChunkMask;
    Result.RelTileY = AbsTileY & TileMap->ChunkMask;

    return(Result);
}

inline uint32
GetTileValue(tile_map *TileMap, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY)
{
    uint32 TileChunkValue = 0;
    
    if(TileChunk && TileChunk->Tiles)
    {
        TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
    }
    
    return(TileChunkValue);
}

inline uint32
GetTileValue(tile_map *TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ);
    uint32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);

    return(TileChunkValue);
}

inline uint32
GetTileValue(tile_map *TileMap, tile_map_position Pos)
{
    uint32 TileChunkValue = GetTileValue(TileMap, Pos.AbsTileX, Pos.AbsTileY, Pos.AbsTileZ);

    return(TileChunkValue);
}

internal bool32
IsTileValueEmpty(uint32 TileValue)
{
    bool32 Empty = ((TileValue == 1) ||
                    (TileValue == 3) ||
                    (TileValue == 4));

    return(Empty);
}

internal bool32
IsTileMapPointEmpty(tile_map *TileMap, tile_map_position Pos)
{
    uint32 TileChunkValue = GetTileValue(TileMap, Pos);
    bool32 Empty = IsTileValueEmpty(TileChunkValue);

    return(Empty);
}

internal void
SetTileValue(memory_arena *Arena, tile_map *TileMap,
             uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ,
             uint32 TileValue)
{
    tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY, AbsTileZ);
    tile_chunk *TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY, ChunkPos.TileChunkZ, Arena);
    SetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, TileValue);
}

internal void
InitializeTileMap(tile_map *TileMap, real32 TileSideInMeters)
{
    TileMap->ChunkShift = 4;
    TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
    TileMap->ChunkDim = (1 << TileMap->ChunkShift);        
    TileMap->TileSideInMeters = TileSideInMeters;

    for(uint32 TileChunkIndex = 0;
        TileChunkIndex < ArrayCount(TileMap->TileChunkHash);
        ++TileChunkIndex)
    {
        TileMap->TileChunkHash[TileChunkIndex].TileChunkX = TILE_CHUNK_UNINITIALIZED;
    }
}

//
// TODO(casey): Do these really belong in more of a "positioning" or "geometry" file?
//

inline void
RecanonicalizeCoord(tile_map *TileMap, int32 *Tile, real32 *TileRel)
{
    // TODO(casey): Need to do something that doesn't use the divide/multiply method
    // for recanonicalizing because this can end up rounding back on to the tile
    // you just came from.

    // NOTE(casey): TileMap is assumed to be toroidal topology, if you
    // step off one end you come back on the other!
    int32 Offset = RoundReal32ToInt32(*TileRel / TileMap->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset*TileMap->TileSideInMeters;

    // TODO(casey): Fix floating point math so this can be exact?
    Assert(*TileRel > -0.5f*TileMap->TileSideInMeters);
    Assert(*TileRel < 0.5f*TileMap->TileSideInMeters);
}

inline tile_map_position
MapIntoTileSpace(tile_map *TileMap, tile_map_position BasePos, v2 Offset)
{
    tile_map_position Result = BasePos;

    Result.Offset_ += Offset;
    RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.Offset_.X);
    RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.Offset_.Y);
    
    return(Result);
}

#define SG_TILE_H
#endif
