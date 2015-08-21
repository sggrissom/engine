/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Steven Grissom $
   ======================================================================== */

#include "game.h"

internal void
DrawBitmap(game_screen_buffer *Buffer, loaded_bitmap *Bitmap,
           v2 P, v2 Align = {0,0})
{
    P -= Align;

    s32 MinX = RoundR32ToS32(P.x);
    s32 MinY = RoundR32ToS32(P.y);
    s32 MaxX = RoundR32ToS32(P.x + (r32)Bitmap->Width);
    s32 MaxY = RoundR32ToS32(P.y + (r32)Bitmap->Height);

    s32 SourceOffsetX = 0;
    if(MinX < 0)
    {
        SourceOffsetX = -MinX;
        MinX = 0;
    }
    s32 SourceOffsetY = 0;
    if(MinY < 0)
    {
        SourceOffsetY = -MinY;
        MinY = 0;
    }
    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    u32 *SourceRow = Bitmap->Pixels + Bitmap->Width*(Bitmap->Height - 1);
    SourceRow += -SourceOffsetY*Bitmap->Width + SourceOffsetX;
    u8 *DestRow = ((u8 *)Buffer->Memory +
                   MinX*Buffer->BytesPerPixel +
                   MinY*Buffer->Pitch);

    for(s32 Y = MinY;
        Y < MaxY;
        ++Y)
    {
        u32 *Dest = (u32 *)DestRow;
        u32 *Source = SourceRow;
        for(s32 X = MinX;
            X < MaxX;
            ++X)
        {
            r32 A = (r32)((*Source >> 24) & 0xFF) / 255.0f;
            r32 SR = (r32)((*Source >>16) & 0xFF);
            r32 SG = (r32)((*Source >> 8) & 0xFF);
            r32 SB = (r32)((*Source >> 0) & 0xFF);

            r32 DR = (r32)((*Dest >> 16) & 0xFF);
            r32 DG = (r32)((*Dest >> 8) & 0xFF);
            r32 DB = (r32)((*Dest >> 0) & 0xFF);

            r32 R = (1.0f-A)*DR + A*SR;
            r32 G = (1.0f-A)*DG + A*SG;
            r32 B = (1.0f-A)*DB + A*SB;

            *Dest = (((u32)(R + 0.5f) << 16) |
                     ((u32)(G + 0.5f) << 8) |
                     ((u32)(B + 0.5f) << 0));
            
            ++Dest;
            ++Source;
        }

        DestRow += Buffer->Pitch;
        SourceRow -= Bitmap->Width;
    }
}

#pragma pack(push, 1)
struct bitmap_header
{
    u16 FileType;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 BitmapOffset;
    u32 Size;
    s32 Width;
    s32 Height;
    u16 Planes;
    u16 BitsPerPixel;
    u32 Compression;
    u32 SizeOfBitmap;
    s32 HorzResolution;
    s32 VertResolution;
    u32 ColorsUsed;
    u32 ColorsImportant;

    u32 RedMask;
    u32 GreenMask;
    u32 BlueMask;
};
#pragma pack(pop)

internal loaded_bitmap
DEBUGLoadBitmap(thread_context *Thread, debug_platform_read_entire_file *ReadEntireFile, char *Filename)
{
    loaded_bitmap Result = {};
    
    debug_read_file_result ReadResult = ReadEntireFile(Thread, Filename);    
    if(ReadResult.ContentsSize != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Contents;
        u32 *Pixels = (u32 *)((u8 *)ReadResult.Contents + Header->BitmapOffset);
        Result.Pixels = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;

        Assert(Header->Compression == 3);

        u32 RedMask = Header->RedMask;
        u32 GreenMask = Header->GreenMask;
        u32 BlueMask = Header->BlueMask;
        u32 AlphaMask = ~(RedMask | GreenMask | BlueMask);        
        
        bit_scan_result RedShift = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenShift = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueShift = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaShift = FindLeastSignificantSetBit(AlphaMask);

        Assert(RedShift.Found);
        Assert(GreenShift.Found);
        Assert(BlueShift.Found);
        Assert(AlphaShift.Found);
        
        u32 *SourceDest = Pixels;
        for(s32 Y = 0;
            Y < Header->Height;
            ++Y)
        {
            for(s32 X = 0;
                X < Header->Width;
                ++X)
            {
                u32 C = *SourceDest;
                *SourceDest++ = ((((C >> AlphaShift.Index) & 0xFF) << 24) |
                                 (((C >> RedShift.Index) & 0xFF) << 16) |
                                 (((C >> GreenShift.Index) & 0xFF) << 8) |
                                 (((C >> BlueShift.Index) & 0xFF) << 0));
            }
        }
    }

    return Result;
}

internal void
DrawRectangle(game_screen_buffer *Buffer, v2 vMin, v2 vMax, v3 ColorVector)
{
    s32 MinX = RoundR32ToS32(vMin.x);
    s32 MinY = RoundR32ToS32(vMin.y);
    s32 MaxX = RoundR32ToS32(vMax.x);
    s32 MaxY = RoundR32ToS32(vMax.y);

    if(MinX < 0)
    {
        MinX = 0;
    }
    if(MinY < 0)
    {
        MinY = 0;
    }
    if(MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    if(MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }

    u32 Color32 = ((RoundR32ToU32(ColorVector.r * 255.0f) << 16) |
                   (RoundR32ToU32(ColorVector.g * 255.0f) << 8) |
                   (RoundR32ToU32(ColorVector.b * 255.0f) << 0));

    u8 *Row = ((u8 *)Buffer->Memory +
               MinX*Buffer->BytesPerPixel +
               MinY*Buffer->Pitch);
    for(s32 Y = MinY;
        Y < MaxY;
        ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        for(s32 X = MinX;
            X < MaxX;
            ++X)
        {
            *Pixel++ = Color32;
        }

        Row += Buffer->Pitch;
    }
}

internal void
MovePlayer(game_state *GameState, entity *Player, r32 dt, r32 Speed)
{
    v2 twoDimentionDDP = v2{Player->SimEntity.ddP.x, Player->SimEntity.ddP.y};
    r32 ddPLength = LengthSq(twoDimentionDDP);
    if(ddPLength > 1.0f)
    {
        twoDimentionDDP *= (1.0f / (r32)sqrt(ddPLength));
    }

    twoDimentionDDP *= Speed;
    twoDimentionDDP += -8.0f*v2{Player->SimEntity.dP.x, Player->SimEntity.dP.y};
    Player->SimEntity.ddP = v3{twoDimentionDDP.x, twoDimentionDDP.y, Player->SimEntity.ddP.z};
    
    v3 OldPlayerP = Player->P;
    v3 PlayerDelta = (0.5f*Player->SimEntity.ddP*Square(dt) +
                      Player->SimEntity.dP*dt);
    Player->SimEntity.dP = Player->SimEntity.ddP*dt + Player->SimEntity.dP;
    v3 NewPlayerP = OldPlayerP + PlayerDelta;

    if(NewPlayerP.z < 0)
    {
        NewPlayerP.z = 0;
        Player->SimEntity.dP.z = Player->SimEntity.ddP.z = 0;
    }

    Player->P = NewPlayerP;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    r32 MetersToPixels = 40.0f;
    r32 PlayerSpeed = 50.0f;

    game_state *GameState = (game_state *)Memory->PermanentStorage;

    if(!Memory->IsInitialized)
    {
        AddEntity(GameState, EntityType_Null);
        
        /*
        GameState->Backdrop =
            DEBUGLoadBitmap(Thread, Memory->DEBUGPlatformReadEntireFile, "w:/engine/data/test24.bmp");
        */


        AddWall(GameState, v3{2.0f, 2.0f});
        
        for(u32 PlayerIndex = 0;
            PlayerIndex < ArrayCount(GameState->Players);
            ++PlayerIndex)
        {
            GameState->Players[PlayerIndex] = {};
        }

        Memory->IsInitialized = true;
    }

    for(u32 ControllerIndex = 0;
        ControllerIndex < ArrayCount(Input->Controllers);
        ++ControllerIndex)
    {
        controller_input *Controller = GetController(Input, ControllerIndex);
        controlled_player *Player = GameState->Players + ControllerIndex;
        if(Player->EntityIndex == 0)
        {
            if(Controller->Start.EndedDown)
            {
                AddPlayer(GameState, ControllerIndex);
            }
        }
        else
        {
            entity *EntPlayer = GetEntity(GameState, Player->EntityIndex);
            sim_entity *SimPlayer = &EntPlayer->SimEntity;
            
            SimPlayer->ddP = {0,0,-9.8f};
            
            if(Controller->IsAnalog)
            {
                SimPlayer->ddP = v3{Controller->StickAverageX, Controller->StickAverageY, -9.8f};
            }
            else
            {
                if(Controller->MoveUp.EndedDown)
                {
                    SimPlayer->ddP.y = 1.0f;
                }
                if(Controller->MoveDown.EndedDown)
                {
                    SimPlayer->ddP.y = -1.0f;
                }
                if(Controller->MoveLeft.EndedDown)
                {
                    SimPlayer->ddP.x = -1.0f;
                }
                if(Controller->MoveRight.EndedDown)
                {
                    SimPlayer->ddP.x = 1.0f;
                }
            }

            if(Controller->Start.EndedDown && EntPlayer->P.z == 0)
            {
                SimPlayer->dP.z = 3.0f;
            }

            MovePlayer(GameState, EntPlayer, Input->dtForFrame, PlayerSpeed);
        }
    }

    r32 ScreenCenterX = 0.5f*(r32)Buffer->Width;
    r32 ScreenCenterY = 0.5f*(r32)Buffer->Height;

    v2 vMin = {0, 0};
    v2 vMax = {(r32)Buffer->Width,(r32)Buffer->Height};
    v3 Color = {0.0f, 1.0f, 0.0f};
    
    DrawRectangle(Buffer, vMin, vMax, Color);

    DrawBitmap(Buffer, &GameState->Backdrop, v2{0,0});
    
    for(u32 EntityIndex = 1;
        EntityIndex < GameState->EntityCount;
        ++EntityIndex)
    {
        entity *Entity = GetEntity(GameState, EntityIndex);
        if(Entity)
        {
            r32 GroundPointX = ScreenCenterX + MetersToPixels*Entity->P.x;
            r32 GroundPointY = ScreenCenterY - MetersToPixels*Entity->P.y;

            v2 LeftTop = {GroundPointX - 0.5f*MetersToPixels*Entity->SimEntity.Width,
                                GroundPointY - 0.5f*MetersToPixels*Entity->SimEntity.Height};
            
            v2 WidthHeight = {Entity->SimEntity.Width, Entity->SimEntity.Height};

            Color = Entity->SimEntity.Color;

            DrawRectangle(Buffer,
                          LeftTop,
                          LeftTop + MetersToPixels*WidthHeight,
                          Color);
        }
    }
}
