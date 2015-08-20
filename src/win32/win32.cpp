/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Steven Grissom $
   ======================================================================== */

#include <windows.h>
#include <sgg.h>
#include "sg_platform.h"

#include "win32.h"
#include "sg_controller.h"

global b32 GlobalRunning;
global b32 GlobalPause;
global b32 DEBUGGlobalShowCursor;
global u64 GlobalPerfCountFrequency;
global screen_buffer GlobalScreenBuffer;
global WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};

internal void
CatStrings(size_t SourceACount, char *SourceA,
           size_t SourceBCount, char *SourceB,
           size_t DestCount, char *Dest)
{
    for(u32 Index = 0;
        Index < SourceACount;
        ++Index)
    {
        *Dest++ = *SourceA++;
    }

    for(u32 Index = 0;
        Index < SourceBCount;
        ++Index)
    {
        *Dest++ = *SourceB++;
    }

    *Dest++ = 0;
}

internal u32
StringLength(char *String)
{
    u32 Count = 0;

    while(*String++)
    {
        ++Count;
    }

    return Count;
}

internal void
GetEXEFilename(state *State)
{
    DWORD SizeOfFilename = GetModuleFileNameA(0, State->EXEPath, sizeof(State->EXEPath));
    State->EXEFilename = State->EXEPath;
    for(char *Scan = State->EXEPath;
        *Scan;
        ++Scan)
    {
        if(*Scan == '\\')
        {
            State->EXEFilename = Scan + 1;
        }
    }
}

internal void
BuildEXEPathFilename(state *State, char *Filename,
                     u32 DestCount, char *Dest)
{
    CatStrings(State->EXEFilename - State->EXEPath, State->EXEPath,
               StringLength(Filename), Filename,
               DestCount, Dest);
}

inline FILETIME
GetLastWriteTime(char *Filename)
{
    FILETIME LastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }

    return LastWriteTime;
}

internal game_code
LoadGameCode(char *SourceDLLName, char *TempDLLName, char *LockFilename)
{
    game_code Result = {};

    WIN32_FILE_ATTRIBUTE_DATA Ignored;

    if(!GetFileAttributesEx(LockFilename, GetFileExInfoStandard, &Ignored))
    {
        Result.DLLLastWriteTime = GetLastWriteTime(SourceDLLName);

        CopyFile(SourceDLLName, TempDLLName, FALSE);

        Result.GameCodeDLL = LoadLibraryA(TempDLLName);
        if(Result.GameCodeDLL)
        {
            Result.UpdateAndRender = (game_update_and_render *)
                GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");

            Result.IsValid = (Result.UpdateAndRender) ? true : false;
        }
    }

    if(!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
    }

    return Result;
}

internal void
UnloadGameCode(game_code *GameCode)
{
    if(GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }

    GameCode->IsValid = false;
    GameCode->UpdateAndRender = 0;
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    if(Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result Result = {};
    
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            Assert(FileSize.QuadPart <= 0xFFFFFFFF);
            u32 FileSize32 = (u32)FileSize.QuadPart;
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(Result.Contents)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
                   (FileSize32 == BytesRead))
                {
                    Result.ContentsSize = FileSize32;
                }
                else
                {                    
                    DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
                    Result.Contents = 0;
                }
            }
        }
        else
        CloseHandle(FileHandle);
    }

    return(Result);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    b32 Result = false;
    
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
        {
            // NOTE(casey): File read successfully
            Result = (BytesWritten == MemorySize);
        }
        CloseHandle(FileHandle);
    }

    return(Result);
}

internal window_dimension
GetWindowDimension(HWND Window)
{
    window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void
ResizeDIBSection(screen_buffer *Buffer, u32 Width, u32 Height)
{
    if(Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    u32 BytesPerPixel = 4;
    Buffer->BytesPerPixel = BytesPerPixel;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    u32 BitmapMemorySize = (Buffer->Width*Buffer->Height) * BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width*BytesPerPixel;
}

internal void
DisplayBufferInWindow(screen_buffer *Buffer,
                      HDC DeviceContext, s32 WindowWidth, s32 WindowHeight)
{
    if((WindowWidth >= Buffer->Width*2) &&
       (WindowHeight >= Buffer->Height*2))
    {
        StretchDIBits(DeviceContext,
                      0, 0, 2*Buffer->Width, 2*Buffer->Height,
                      0, 0, Buffer->Width, Buffer->Height,
                      Buffer->Memory,
                      &Buffer->Info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
    else
    {
        u32 OffsetX = 10;
        u32 OffsetY = 10;

        PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
        PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
        PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
        PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);
    
        StretchDIBits(DeviceContext,
                      OffsetX, OffsetY, Buffer->Width, Buffer->Height,
                      0, 0, Buffer->Width, Buffer->Height,
                      Buffer->Memory,
                      &Buffer->Info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
}

internal void
ToggleFullscreen(HWND Window)
{
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if(Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if(GetWindowPlacement(Window, &GlobalWindowPosition) &&
           GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal LRESULT CALLBACK
MainWindowCallback(HWND Window,
                   UINT Message,
                   WPARAM wParam,
                   LPARAM lParam)
{
    LRESULT result = 0;

    switch(Message)
    {
        case WM_ACTIVATEAPP:
        {
        } break;
        case WM_SETCURSOR:
        {
            if(DEBUGGlobalShowCursor)
            {
                result = DefWindowProcA(Window, Message, wParam, lParam);
            }
            else
            {
                SetCursor(0);
            }
        } break;
        case WM_CLOSE:
        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;
        
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            InvalidCodePath;
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            window_dimension Dimension = GetWindowDimension(Window);
            DisplayBufferInWindow(&GlobalScreenBuffer, DeviceContext,
                                  Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            result = DefWindowProcA(Window, Message, wParam, lParam);
        } break;
    }

    return result;
}

internal void
ProcessPendingMessages(state *State, controller_input *Keyboard)
{
    MSG Message;
    while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 VKCode = (u32)Message.wParam;
                b32 WasDown = (Message.lParam & (1 << 30)) != 0;
                b32 IsDown = (Message.lParam & (1 << 31)) == 0;
                if(WasDown != IsDown)
                {
                    if(VKCode == 'W')
                    {
                        ProcessKeyboardMessage(&Keyboard->MoveUp, IsDown);
                    }
                    else if(VKCode == 'A')
                    {
                        ProcessKeyboardMessage(&Keyboard->MoveLeft, IsDown);
                    }
                    else if(VKCode == 'S')
                    {
                        ProcessKeyboardMessage(&Keyboard->MoveDown, IsDown);
                    }
                    else if(VKCode == 'D')
                    {
                        ProcessKeyboardMessage(&Keyboard->MoveRight, IsDown);
                    }
                    else if(VKCode == 'Q')
                    {
                        ProcessKeyboardMessage(&Keyboard->LeftShoulder, IsDown);
                    }
                    else if(VKCode == 'E')
                    {
                        ProcessKeyboardMessage(&Keyboard->RightShoulder, IsDown);
                    }
                    else if(VKCode == VK_UP)
                    {
                        ProcessKeyboardMessage(&Keyboard->ActionUp, IsDown);
                    }
                    else if(VKCode == VK_LEFT)
                    {
                        ProcessKeyboardMessage(&Keyboard->ActionLeft, IsDown);
                    }
                    else if(VKCode == VK_DOWN)
                    {
                        ProcessKeyboardMessage(&Keyboard->ActionDown, IsDown);
                    }
                    else if(VKCode == VK_RIGHT)
                    {
                        ProcessKeyboardMessage(&Keyboard->ActionRight, IsDown);
                    }
                    else if(VKCode == VK_ESCAPE)
                    {
                        ProcessKeyboardMessage(&Keyboard->Back, IsDown);
                    }
                    else if(VKCode == VK_SPACE)
                    {
                        ProcessKeyboardMessage(&Keyboard->Start, IsDown);
                    }
#if INTERNAL
                    else if(VKCode == 'P')
                    {
                        if(IsDown)
                        {
                            GlobalPause = !GlobalPause;
                        }
                    }
#endif
                }

                b32 AltKeyIsDown = (Message.lParam & (1 << 29));
                if ((VKCode == VK_F4) && AltKeyIsDown)
                {
                    GlobalRunning = false;
                }
                if((VKCode == VK_RETURN) && AltKeyIsDown)
                {
                    if(Message.hwnd)
                    {
                        ToggleFullscreen(Message.hwnd);
                    }
                }
            } break;
            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            } break;
        };
    }
}

inline LARGE_INTEGER
GetWallClock(void)
{    
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline r32
GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    r32 Result = ((r32)(End.QuadPart - Start.QuadPart) /
                     (r32)GlobalPerfCountFrequency);
    return Result;
}

s32 CALLBACK
WinMain(HINSTANCE instance,
        HINSTANCE, LPSTR, s32)
{
    state State = {};
    
    LARGE_INTEGER perfFrequencyResult;
    QueryPerformanceFrequency(&perfFrequencyResult);
    GlobalPerfCountFrequency = perfFrequencyResult.QuadPart;

    GetEXEFilename(&State);

    char SourceGameCodeDLLFullPath[WIN32_FILENAME_COUNT];
    BuildEXEPathFilename(&State, "game.dll",
                         sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);

    char TempGameCodeDLLFullPath[WIN32_FILENAME_COUNT];
    BuildEXEPathFilename(&State, "game_temp.dll",
                         sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);

    char GameCodeLockFullPath[WIN32_FILENAME_COUNT];
    BuildEXEPathFilename(&State, "lock.tmp",
                         sizeof(GameCodeLockFullPath), GameCodeLockFullPath);

    UINT DesiredSchedulerMS = 1;
    b32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

#if INTERNAL
    DEBUGGlobalShowCursor = true;
#endif

    ResizeDIBSection(&GlobalScreenBuffer, 960, 540);

    LoadXInput();

    WNDCLASS windowClass = {};
    windowClass.style = CS_HREDRAW|CS_VREDRAW;
    windowClass.lpfnWndProc = MainWindowCallback;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "Win32WindowClass";
    windowClass.hCursor = LoadCursor(0, IDC_ARROW);
    windowClass.hIcon = LoadIcon(0, IDI_APPLICATION);

    if(RegisterClassA(&windowClass))
    {
        HWND Window =
            CreateWindowExA(
                0,
                windowClass.lpszClassName,
                "Steven's Awesome App",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,0,instance,0);
        if(Window)
        {
            u32 MonitorRefreshHz = 60;
            HDC RefreshDC = GetDC(Window);
            u32 RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            ReleaseDC(Window, RefreshDC);
            if(RefreshRate > 1)
            {                
                MonitorRefreshHz = RefreshRate;
            }

            r32 GameUpdateHz = MonitorRefreshHz / 1.0f; //full speed for now
            r32 TargetSecondsPerFrame = 1.0f / (r32)GameUpdateHz;
            
            GlobalRunning = true;

#if INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
            LPVOID BaseAddress = 0;
#endif

            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(256);
            GameMemory.TransientStorageSize = Megabytes(256);
            GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
            GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

            State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
            State.GameMemoryBlock = VirtualAlloc(BaseAddress, (size_t)State.TotalSize,
                                                 MEM_RESERVE|MEM_COMMIT,
                                                 PAGE_READWRITE);
            GameMemory.PermanentStorage = State.GameMemoryBlock;
            GameMemory.TransientStorage = ((u8 *)GameMemory.PermanentStorage +
                                           GameMemory.PermanentStorageSize);

            if(GameMemory.PermanentStorage && GameMemory.TransientStorage)
            {
                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

                LARGE_INTEGER LastCounter = GetWallClock();
                LARGE_INTEGER FlipWallClock = GetWallClock();

                game_code Game = LoadGameCode(SourceGameCodeDLLFullPath,
                                              TempGameCodeDLLFullPath,
                                              GameCodeLockFullPath);

                u64 LastCycleCount = __rdtsc();
                
                while(GlobalRunning)
                {
                    NewInput->dtForFrame = TargetSecondsPerFrame;

                    NewInput->ExecutableReloaded = false;
                    FILETIME NewDLLWriteTime = GetLastWriteTime(SourceGameCodeDLLFullPath);
                    if(CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
                    {
                        UnloadGameCode(&Game);
                        Game = LoadGameCode(SourceGameCodeDLLFullPath,
                                            TempGameCodeDLLFullPath,
                                            GameCodeLockFullPath);
                        NewInput->ExecutableReloaded = true;
                    }

                    controller_input *OldKeyboardController = GetController(OldInput, 0);
                    controller_input *NewKeyboardController = GetController(NewInput, 0);
					*NewKeyboardController = {};
                    NewKeyboardController->IsConnected = true;

                    for(u32 ButtonIndex = 0;
                        ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
                        ++ButtonIndex)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }
                    
                    ProcessPendingMessages(&State, NewKeyboardController);
                    
                    if(!GlobalPause)
                    {
                        UpdateControllers(Window, NewInput, OldInput);

                        thread_context Thread = {};

                        game_screen_buffer Buffer = {};
                        Buffer.Memory = GlobalScreenBuffer.Memory;
                        Buffer.Width = GlobalScreenBuffer.Width;
                        Buffer.Height = GlobalScreenBuffer.Height;
                        Buffer.Pitch = GlobalScreenBuffer.Pitch;

                        if(Game.UpdateAndRender)
                        {
                            Game.UpdateAndRender(&Thread, &GameMemory, NewInput, &Buffer);
                        }

                        LARGE_INTEGER WorkCounter = GetWallClock();
                        r32 WorkSecondsElapsed = GetSecondsElapsed(LastCounter, WorkCounter);
                        r32 SecondsElapsedForFrame = WorkSecondsElapsed;
                        
                        if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            if(SleepIsGranular)
                            {
                                DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame -
                                                                   SecondsElapsedForFrame));
                                if(SleepMS > 0)
                                {
                                    Sleep(SleepMS);
                                }
                            }

                            while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                SecondsElapsedForFrame = GetSecondsElapsed(LastCounter, GetWallClock());
                            }
                        }
                        else
                        {
                            //TODO(steven): Missed frame ratet
                        }

                        LARGE_INTEGER EndCounter = GetWallClock();
                        r32 MSPerFrame = 1000.0f*GetSecondsElapsed(LastCounter, EndCounter);
                        LastCounter = EndCounter;

                        window_dimension Dimension = GetWindowDimension(Window);
                        HDC DeviceContext = GetDC(Window);
                        DisplayBufferInWindow(&GlobalScreenBuffer, DeviceContext,
                                              Dimension.Width, Dimension.Height);
                        ReleaseDC(Window, DeviceContext);

                        FlipWallClock = GetWallClock();

                        game_input *temp = NewInput;
                        NewInput = OldInput;
                        OldInput = temp;

#if 0
                        u64 EndCycleCount = __rdtsc();
                        u64 CyclesElapsed = EndCycleCount - LastCycleCount;
                        LastCycleCount = EndCycleCount;

                        r64 FPS = 0.0f;
                        r64 MCPF = (r64)CyclesElapsed / (1000.0f * 1000.0f);

                        char FPSBuffer[256];
                        _snprintf_s(FPSBuffer, sizeof(FPSBuffer),
                                    "%.02fms/f,   %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
                        OutputDebugStringA(FPSBuffer);
#endif
                    }
                }
            }
        } else {
            InvalidCodePath;
        }
    } else {
        InvalidCodePath;
    }
    
    return 0;
}
