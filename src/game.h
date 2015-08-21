#if !defined(GAME_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Steven Grissom $
   ======================================================================== */

#include <sgg.h>
#include "sg_platform.h"
#include "sg_math.h"

struct controlled_player
{
    v3 P;
    v3 dP;
    v3 ddP;
    r32 Height;
    r32 Width;
    u32 PlayerIndex;
};

struct game_state
{
    u32 PlayerCount;
    controlled_player Players[5];
};

#define GAME_H
#endif
