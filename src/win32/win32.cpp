/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Steven Grissom $
   ======================================================================== */

#include <windows.h>
#include <sgg.h>

global b32 GlobalRunning;
global u64 GlobalPerfCountFrequency;
global screen_buffer *GlobalScreenBuffer;

internal window_dimension
GetWindowDimension(HWND Window)
{
    window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.width = ClientRect.right - ClientRect.left;
    Result.height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void
DisplayBufferInWindow(screen_buffer *Buffer,
                      HDC DeviceContext, u32 WindowWidth, u32 WindowHeight)
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

internal LRESULT CALLBACK
mainWindowCallback(HWND window,
                   UINT message,
                   WPARAM wParam,
                   LPARAM lParam)
{
    LRESULT result = 0;

    switch(message)
    {
        case WM_ACTIVATEAPP:
        {
        } break;
        case WM_SETCURSOR:
        {
            SetCursor(0);
        } break;
        case WM_CLOSE:
        case WM_DESTROY:
        {
            globalRunning = false;
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
            DisplayBufferInWindow(&globalScreenBuffer, DeviceContext,
                                  Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            result = DefWindowProcA(window, message, wParam, lParam);
        } break;
    }

    return result;
}

internal void
processPendingMessages(HWND window)
{
    MSG message;
    while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
    {
        switch(message.message)
        {
            case WM_QUIT:
            {
                globalRunning = false;
            } break;

            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                uint32 VKCode = (uint32)message.wParam;
                bool32 wasDown = (message.lParam & (1 << 30)) != 0;
                bool32 isDown = (message.lParam & (1 << 31)) == 0;
                if(wasDown != isDown)
                {
                    if(VKCode == VK_ESCAPE)
                    {
                    }
                    else if(VKCode == VK_SPACE)
                    {
                    }
                }

                //NOTE(steven): alt-f4
                if ((VKCode == VK_F4) && (message.lParam & (1 << 29)))
                {
                    globalRunning = false;
                }
            } break;
            default:
            {
                TranslateMessage(&message);
                DispatchMessageA(&message);
            } break;
        };
    }
}

s32 CALLBACK
WinMain(HINSTANCE instance,
        HINSTANCE, LPSTR, s32)
{
    state State = {};
    
    LARGE_INTEGER perfFrequencyResult;
    QueryPerformanceFrequency(&perfFrequencyResult);
    GlobalPerfCountFrequency = perfFrequencyResult.QuadPart;

    UINT DesiredSchedulerMS = 1;
    b32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    LoadXInput();

    WNDCLASS windowClass = {};
  
    windowClass.style = CS_HREDRAW|CS_VREDRAW;
    windowClass.lpfnWndProc = mainWindowCallback;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = "Win32WindowClass";
    windowClass.hCursor = LoadCursor(0, IDC_ARROW);
    windowClass.hIcon = LoadIcon(0, IDI_APPLICATION);

    executeFunction = &execute;

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
            globalRunning = true;

            while(globalRunning)
            {
                processPendingMessages(Window);
            }
        } else {
            InvalidCodePath;
        }
    } else {
        InvalidCodePath;
    }
    
    return 0;
}
