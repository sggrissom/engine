/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Steven Grissom $
   ======================================================================== */

#include "game.h"

internal void
DrawBitmap()
{
    
}

internal void
DEBUGLoadBitmap()
{
    
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
MovePlayer(game_state *GameState, controlled_player *Player, r32 dt, r32 Speed)
{
    v2 twoDimentionDDP = v2{Player->ddP.x, Player->ddP.y};
    r32 ddPLength = LengthSq(twoDimentionDDP);
    if(ddPLength > 1.0f)
    {
        twoDimentionDDP *= (1.0f / (r32)sqrt(ddPLength));
    }

    twoDimentionDDP *= Speed;
    twoDimentionDDP += -8.0f*v2{Player->dP.x, Player->dP.y};
    Player->ddP = v3{twoDimentionDDP.x, twoDimentionDDP.y, Player->ddP.z};
    
    v3 OldPlayerP = Player->P;
    v3 PlayerDelta = (0.5f*Player->ddP*Square(dt) +
                      Player->dP*dt);
    Player->dP = Player->ddP*dt + Player->dP;
    v3 NewPlayerP = OldPlayerP + PlayerDelta;

    if(NewPlayerP.z < 0) NewPlayerP.z = 0;

    Player->P = NewPlayerP;
}

internal void
AddPlayer(game_state *GameState, u32 ControllerIndex)
{
    controlled_player *Player = GameState->Players + ControllerIndex;
    Player->PlayerIndex = ControllerIndex + 1;
    Player->P = {0, 0, 0};
    Player->Height = 1.0f;
    Player->Width = 1.0f;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    r32 MetersToPixels = 40.0f;
    r32 PlayerSpeed = 50.0f;

    game_state *GameState = (game_state *)Memory->PermanentStorage;

    if(!Memory->IsInitialized)
    {
        GameState->PlayerCount = ArrayCount(GameState->Players);
        
        for(u32 PlayerIndex = 0;
            PlayerIndex < GameState->PlayerCount;
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
        if(Player->PlayerIndex == 0)
        {
            if(Controller->Start.EndedDown)
            {
                AddPlayer(GameState, ControllerIndex);
            }
        }
        else
        {
            Player->ddP = {0,0,-9.8f};
            
            if(Controller->IsAnalog)
            {
                Player->ddP = v3{Controller->StickAverageX, Controller->StickAverageY, -9.8f};
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

            MovePlayer(GameState, Player, Input->dtForFrame, PlayerSpeed);
        }
    }

    r32 ScreenCenterX = 0.5f*(r32)Buffer->Width;
    r32 ScreenCenterY = 0.5f*(r32)Buffer->Height;

    v2 vMin = {0, 0};
    v2 vMax = {(r32)Buffer->Width,(r32)Buffer->Height};
    v3 Color = {0.0f, 1.0f, 0.0f};
    
    DrawRectangle(Buffer, vMin, vMax, Color);
    
    for(u32 PlayerIndex = 1;
        PlayerIndex <= GameState->PlayerCount;
        ++PlayerIndex)
    {
        controlled_player *Player = GameState->Players + PlayerIndex - 1;
        if(Player->PlayerIndex)
        {
            r32 HalfScreenWidth = 0.5f * Buffer->Width;
            r32 HalfScreenHeight = 0.5f * Buffer->Height;

            r32 PlayerGroundPointX = ScreenCenterX + MetersToPixels*Player->P.x;
            r32 PlayerGroundPointY = ScreenCenterY - MetersToPixels*Player->P.y;

            r32 Z = -MetersToPixels*Player->P.z;
            v2 PlayerLeftTop = {PlayerGroundPointX - 0.5f*MetersToPixels*Player->Width,
                                PlayerGroundPointY - 0.5f*MetersToPixels*Player->Height};
            
            if(PlayerLeftTop.x < 0) PlayerLeftTop.x = 0;
            if(PlayerLeftTop.x > Buffer->Width) PlayerLeftTop.x = (r32)Buffer->Width;
            if(PlayerLeftTop.y < 0) PlayerLeftTop.y = 0;
            if(PlayerLeftTop.y > Buffer->Height) PlayerLeftTop.y = (r32)Buffer->Height;

            v2 PlayerWidthHeight = {Player->Width, Player->Height};
            // PlayerWidthHeight = {1,1};

            Color = {0, 0, 1.0f};

            DrawRectangle(Buffer,
                          PlayerLeftTop,
                          PlayerLeftTop + MetersToPixels*PlayerWidthHeight,
                          Color);
        }
    }
}
