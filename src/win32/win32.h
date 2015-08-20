#if !defined(WIN32_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Steven Grissom $
   ======================================================================== */

struct screen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    s32 Width;
    s32 Height;
    u32 Pitch;
    u32 BytesPerPixel;
};

struct window_dimension
{
    u32 Width;
    u32 Height;
};

struct game_code
{
    HMODULE GameCodeDLL;
    FILETIME DLLLastWriteTime;

    //NOTE(steven): include any functions the platform layer must call from the DLL
    game_update_and_render *UpdateAndRender;

    b32 IsValid;
};

#define WIN32_FILENAME_COUNT MAX_PATH
struct replay_buffer
{
    HANDLE FileHandle;
    HANDLE MemoryMap;
    char Filename[WIN32_FILENAME_COUNT];
    void *MemoryBlock;
};

struct state
{
    u64 TotalSize;
    void *GameMemoryBlock;
    replay_buffer ReplayBuffers[1];

    HANDLE RecordingHandle;
    u32 InputRecordingIndex;
    HANDLE PlaybackHandle;
    u32 InputPlayingIndex;

    char EXEPath[WIN32_FILENAME_COUNT];
    char *EXEFilename;
};

#define WIN32_H
#endif
