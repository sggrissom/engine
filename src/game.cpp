/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Steven Grissom $
   ======================================================================== */

#include "game.h"

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
MovePlayer(game_state *GameState, controlled_player *Player, r32 dt, r32 Speed)
{
    v2 twoDimentionDDP = v2{Player->ddP.x, Player->ddP.y};
    r32 ddPLength = LengthSq(twoDimentionDDP);
    if(ddPLength > 1.0f)
    {
        twoDimentionDDP *= (1.0f / (r32)sqrt(ddPLength));
    }

    twoDimentionDDP *= Speed;

    twoDimentionDDP += -8.0f*Player->dP;
    v2 oldPlayerP = Player->P;
    v2 PlayerDelta = (0.5f*twoDimentionDDP*Square(dt) +
                      Player->dP*dt);
    Player->dP = ddP*dt + Player->dP;
    v2 NewPlayerP = OldPlayerP + PlayerDelta;

    Player->P = NewPlayerP;
}

internal u32
AddPlayer(game_state *GameState)
{
    Assert(GameState->PlayerCount < ArrayCount(GameState->Players));
    u32 PlayerIndex = GameState->PlayerCount++;

    gameState->Players + PlayerIndex = {};

    controlled_player Player = GameState->Players + PlayerIndex;
    Player->P = {20, 20, 0};
    Player->Height = 0.5f;
    Player->Width = 1.0f;

    return PlayerIndex;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    r32 MetersToPixels = 1.4f / 60.0f;

    game_state *GameState = (game_state *)Memory->PermanentStorage;

    if(!Memory->IsInitialized)
    {

        Memory->IsInitialized = true;
    }

    for(u32 ControllerIndex = 0;
        ControllerIndex < ArrayCount(Input->Controllers);
        ++ControllerIndex)
    {
        controller_input *Controller = GetController(Input, ControllerIndex);
        controlled_player *Player = GameState->Players + ControllerIndex;
        if(Player->EntityIndex = 0)
        {
            if(Controller->Start.EndedDown)
            {
                *Player = {};
                Player->EntityIndex = AddPlayer(GameState).LowIndex;
            }
        }
        else
        {
            player->ddP = {0,0,-9.8f};
            
            if(Controller->IsAnalog)
            {
                Player->ddP = V3{Controller->StickAverageX, Controller->StickAverageY, -9.8f};
            }
            else
            {
                if(Controller->MoveUp.EndedDown)
                {
                    Player->ddP.y = 1.0f;
                }
                if(Controller->MoveDown.EndedDown)
                {
                    Player->ddP.y = -1.0f;
                }
                if(Controller->MoveLeft.EndedDown)
                {
                    Player->ddP.x = -1.0f;
                }
                if(Controller->MoveRight.EndedDown)
                {
                    Player->ddP.x = 1.0f;
                }
            }

            if(Controller->Start.EndedDown && Player->P.z == 0)
            {
                Player->dP.z = 3.0f;
            }

            MovePlayer(GameState, Player, Input->dtForFrame);
        }
    }

    r32 ScreenCenterX = 0.5f*(r32)Buffer->Width;
    r32 ScreenCenterY = 0.5f*(r32)Buffer->Height;

    v2 vMin = {0, 0};
    v2 vMax = {(r32)Buffer->Width,(r32)Buffer->Height};
    v3 Color = {0.0f, 1.0f, 0.0f};
    
    DrawRectangle(Buffer, vMin, vMax, Color);
    
    for(u32 PlayerIndex = 1;
        PlayerIndex < GameState->PlayerCount;
        ++PlayerIndex)
    {
        controlled_player *Player = GameState->Players + PlayerIndex;

        real32 PlayerGroundPointX = ScreenCenterX + MetersToPixels*Player->P.x;
        real32 PlayerGroundPointY = ScreenCenterY - MetersToPixels*Player->P.y;
        real32 Z = -MetersToPixels*Player->P.z;
        v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f*MetersToPixels*Player->Width,
                            PlayerGroundPointY - 0.5f*MetersToPixels*Player->Height};
        v2 PlayerWidthHeight = {Player->Width, Player->Height};

        Color = {0, 0, 1.0f};

        DrawRectangle(Buffer,
                      PlayerLeftTop,
                      PlayerLeftTop + MetersToPixels*PlayerWidthHeight,
                      Color);
    }
}
