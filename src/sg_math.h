
#if !defined(SG_MATH_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Steven Grissom $
   ======================================================================== */

#include <math.h>

inline s32
RoundR32ToS32(r32 Real)
{
    s32 Result = (s32)roundf(Real);
    return Result;
}

inline u32
RoundR32ToU32(r32 Real)
{
    u32 Result = (u32)roundf(Real);
    return Result;
}

inline r32
Square(r32 A)
{
    r32 Result = A * A;
    return Result;
}

inline r32
Inner(v2 A, v2 B)
{
    r32 Result = A.x*B.x + A.y*B.y;
    return Result;
}

inline r32
LengthSq(v2 A)
{
    r32 Result = Inner(A, A);
    return Result;
}

struct bit_scan_result
{
    b32 Found;
    u32 Index;
};
inline bit_scan_result
FindLeastSignificantSetBit(u32 Value)
{
    bit_scan_result Result = {};

#if COMPILER_MSVC
    Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
    for(u32 Test = 0;
        Test < 32;
        ++Test)
    {
        if(Value & (1 << Test))
        {
            Result.Index = Test;
            Result.Found = true;
            break;
        }
    }
#endif

    return Result;
}

#define SG_MATH_H
#endif
