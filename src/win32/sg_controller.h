#if !defined(SG_CONTROLLER_H)
/* ========================================================================
   $File: $
   $Date: $
   $Revision: $
   $Creator: Steven Grissom $
   ======================================================================== */

#include <xinput.h>

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
LoadXInput()
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }
    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if(XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if(!XInputGetState) {XInputGetState = XInputGetStateStub;}
        
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if(!XInputSetState) {XInputSetState = XInputSetStateStub;}
    } else {
        //loading xinput failed, leave the empty stubs
    }
}

internal void
ProcessKeyboardMessage(button_state *NewState, b32 IsDown)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

internal void
ProcessXInputButton(DWORD XInputButtonState,
                    button_state *OldState, DWORD ButtonBit,
                    button_state *NewState)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount += (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal r32
ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
    r32 Result = 0;

    if(Value < -DeadZoneThreshold)
    {
        Result = (r32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
    }
    else if(Value > DeadZoneThreshold)
    {
        Result = (r32)((Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
    }

    return Result;
}

internal void
UpdateControllers(HWND Window,
                  game_input *NewInput,
                  game_input *OldInput)
{
    POINT MouseP;
    GetCursorPos(&MouseP);
    ScreenToClient(Window, &MouseP);
    NewInput->MouseX = MouseP.x;
    NewInput->MouseY = MouseP.y;
    NewInput->MouseZ = 0;
    ProcessKeyboardMessage(&NewInput->MouseButtons[0],
                           GetKeyState(VK_LBUTTON) & (1 << 15));
    ProcessKeyboardMessage(&NewInput->MouseButtons[1],
                           GetKeyState(VK_MBUTTON) & (1 << 15));
    ProcessKeyboardMessage(&NewInput->MouseButtons[2],
                           GetKeyState(VK_RBUTTON) & (1 << 15));
    ProcessKeyboardMessage(&NewInput->MouseButtons[3],
                           GetKeyState(VK_XBUTTON1) & (1 << 15));
    ProcessKeyboardMessage(&NewInput->MouseButtons[4],
                           GetKeyState(VK_XBUTTON2) & (1 << 15));

    DWORD MaxControllerCount = XUSER_MAX_COUNT;
    if(MaxControllerCount > ArrayCount(NewInput->Controllers) - 1)
    {
        MaxControllerCount = ArrayCount(NewInput->Controllers) - 1;
    }
    
    for (DWORD ControllerIndex = 0;
         ControllerIndex < MaxControllerCount;
         ++ControllerIndex)
    {
        DWORD OurControllerIndex = ControllerIndex + 1;
        controller_input *OldController = GetController(OldInput, OurControllerIndex);
        controller_input *NewController = GetController(NewInput, OurControllerIndex);
                    
        XINPUT_STATE ControllerState;
        if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
        {
            NewController->IsConnected = true;
            NewController->IsAnalog = OldController->IsAnalog;
                           
            // NOTE(casey): This controller is plugged in
            // TODO(casey): See if ControllerState.dwPacketNumber increments too rapidly
            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

            // TODO(casey): This is a square deadzone, check XInput to
            // verify that the deadzone is "round" and show how to do
            // round deadzone processing.
            NewController->StickAverageX = ProcessXInputStickValue(
                Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            NewController->StickAverageY = ProcessXInputStickValue(
                Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            if((NewController->StickAverageX != 0.0f) ||
               (NewController->StickAverageY != 0.0f))
            {
                NewController->IsAnalog = true;
            }

            if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
            {
                NewController->StickAverageY = 1.0f;
                NewController->IsAnalog = false;
            }
                            
            if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
            {
                NewController->StickAverageY = -1.0f;
                NewController->IsAnalog = false;
            }
                            
            if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
            {
                NewController->StickAverageX = -1.0f;
                NewController->IsAnalog = false;
            }
                            
            if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
            {
                NewController->StickAverageX = 1.0f;
                NewController->IsAnalog = false;
            }

            r32 Threshold = 0.5f;
            ProcessXInputButton(
                (NewController->StickAverageX < -Threshold) ? 1 : 0,
                &OldController->MoveLeft, 1,
                &NewController->MoveLeft);
            ProcessXInputButton(
                (NewController->StickAverageX > Threshold) ? 1 : 0,
                &OldController->MoveRight, 1,
                &NewController->MoveRight);
            ProcessXInputButton(
                (NewController->StickAverageY < -Threshold) ? 1 : 0,
                &OldController->MoveDown, 1,
                &NewController->MoveDown);
            ProcessXInputButton(
                (NewController->StickAverageY > Threshold) ? 1 : 0,
                &OldController->MoveUp, 1,
                &NewController->MoveUp);

            ProcessXInputButton(Pad->wButtons,
                                       &OldController->ActionDown, XINPUT_GAMEPAD_A,
                                       &NewController->ActionDown);
            ProcessXInputButton(Pad->wButtons,
                                       &OldController->ActionRight, XINPUT_GAMEPAD_B,
                                       &NewController->ActionRight);
            ProcessXInputButton(Pad->wButtons,
                                       &OldController->ActionLeft, XINPUT_GAMEPAD_X,
                                       &NewController->ActionLeft);
            ProcessXInputButton(Pad->wButtons,
                                       &OldController->ActionUp, XINPUT_GAMEPAD_Y,
                                       &NewController->ActionUp);
            ProcessXInputButton(Pad->wButtons,
                                       &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                                       &NewController->LeftShoulder);
            ProcessXInputButton(Pad->wButtons,
                                       &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                       &NewController->RightShoulder);

            ProcessXInputButton(Pad->wButtons,
                                       &OldController->Start, XINPUT_GAMEPAD_START,
                                       &NewController->Start);
            ProcessXInputButton(Pad->wButtons,
                                       &OldController->Back, XINPUT_GAMEPAD_BACK,
                                       &NewController->Back);
        }
        else
        {
            // NOTE(steven): The controller is not available
            NewController->IsConnected = false;
        }
    }
}

#define SG_CONTROLLER_H
#endif
