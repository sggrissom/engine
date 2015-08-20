#if !defined(SG_PLATFORM_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Steven Grissom $
   ======================================================================== */

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct thread_context
    {
        u32 Placeholder;
    } thread_context;

#if INTERNAL

    typedef struct debug_read_file_result
    {
        u32 ContentsSize;
        void *Contents;
    } debug_read_file_result;
    
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, char *Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(thread_context *Thread, char *Filename, u32 MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif

#define BITMAP_BYTES_PER_PIXEL 4
    typedef struct game_screen_buffer
    {
        void *Memory;
        u32 Width;
        u32 Height;
        u32 Pitch;
    } game_screen_buffer;

    typedef struct game_sound_buffer
    {
        u32 SamplePerSecond;
        u32 SampleCount;
        s16 *Samples;
    } game_sound_buffer;

    typedef struct button_state
    {
        u32 HalfTransitionCount;
        b32 EndedDown;
    } button_state;

    typedef struct controller_input
    {
        b32 IsConnected;
        b32 IsAnalog;    
        r32 StickAverageX;
        r32 StickAverageY;
    
        union
        {
            button_state Buttons[12];
            struct
            {
                button_state MoveUp;
                button_state MoveDown;
                button_state MoveLeft;
                button_state MoveRight;
            
                button_state ActionUp;
                button_state ActionDown;
                button_state ActionLeft;
                button_state ActionRight;
            
                button_state LeftShoulder;
                button_state RightShoulder;

                button_state Back;
                button_state Start;
            };
        };
    } controller_input;

    typedef struct game_input
    {
        button_state MouseButtons[5];
        s32 MouseX, MouseY, MouseZ;

        b32 ExecutableReloaded;
        r32 dtForFrame;

        controller_input Controllers[5];
    } game_input;

    typedef struct game_memory
    {
        b32 IsInitialized;

        u64 PermanentStorageSize;
        void *PermanentStorage;

        u64 TransientStorageSize;
        void *TransientStorage;

        debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
        debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
        debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
    } game_memory;

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory *Memory, game_input *Input, game_screen_buffer *Buffer)
    typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
    
    inline controller_input *GetController(game_input *Input, u32 ControllerIndex)
    {
        Assert(ControllerIndex < ArrayCount(Input->Controllers));

        controller_input *Result = &Input->Controllers[ControllerIndex];
        return Result;
    }

#ifdef __cplusplus
}
#endif


#define SG_PLATFORM_H
#endif
