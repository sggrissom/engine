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
};

struct game_state
{
};

#define GAME_H
#endif
