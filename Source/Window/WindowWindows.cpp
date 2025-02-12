#include "Window.h"

global HCURSOR cursor_handles[2]; // 0 - arrow, 1 - hand
global InputState input;

static LRESULT CALLBACK WindowProc(HWND handle, UINT umsg, WPARAM wparam, LPARAM lparam) {
    LRESULT result = 0;

    Window *win = (Window *) GetWindowLongPtr(handle, GWLP_USERDATA);

    switch (umsg) {
        case WM_DESTROY: {
            win->running = 0;
            return 0;
        } break;
        case WM_SIZE: {
            if (win) {
                win->resized = 1;
            }
        } break;
        case WM_PAINT: {
            PAINTSTRUCT ps = {};
            BeginPaint(handle, &ps);
            EndPaint(handle, &ps);
        } break;
        case WM_SETCURSOR: {
        } break;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN: {
            if (win->key_callback) {
                if ((lparam & 0x80000000) == 0) {
                    win->key_callback(wparam);
                }
            }
        } break;
        case WM_CHAR: {
            WCHAR utf16char = (WCHAR)wparam;
            char asciichar;
            int asciicharlen = WideCharToMultiByte(CP_ACP, 0, &utf16char, 1, &asciichar, 1, 0, 0);
            if (asciicharlen == 1 && input.text_len + 1 < sizeof(input.text) - 1) {
                input.text[input.text_len] = asciichar;
                input.text[input.text_len + 1] = 0;
                input.text_len++;
            }

            if (win->char_callback) {
                if ((lparam & 0x80000000) == 0) {
                    win->char_callback(asciichar);
                }
            }
        } break;
        case WM_INPUT: {
            RAWINPUT raw;
            UINT size = sizeof(raw);
            if (GetRawInputData((HRAWINPUT)lparam, RID_INPUT, (LPVOID) &raw, &size, sizeof(RAWINPUTHEADER)) == size) {
                if (raw.header.dwType == RIM_TYPEMOUSE) {
                    if (!(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) {
                        input.mouse_delta_pos.x += raw.data.mouse.lLastX;
                        input.mouse_delta_pos.y += raw.data.mouse.lLastY;
                    }

                    if (raw.data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
                        SHORT WheelDelta = (SHORT)raw.data.mouse.usButtonData;
                        int ScrollSteps = WheelDelta / 120;
                        input.delta_scroll += ScrollSteps;
                        input.scroll += ScrollSteps;
                    }

                    if (raw.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) {
                        input.buttons[MOUSE_BUTTON_LEFT].pressed = 1;
                        input.buttons[MOUSE_BUTTON_LEFT].down = 1;
                    }
                    if (raw.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP) {
                        input.buttons[MOUSE_BUTTON_LEFT].released = 1;
                        input.buttons[MOUSE_BUTTON_LEFT].down = 0;
                    }

                    if (raw.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) {
                        input.buttons[MOUSE_BUTTON_RIGHT].pressed = 1;
                        input.buttons[MOUSE_BUTTON_RIGHT].down = 1;
                    }
                    if (raw.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP) {
                        input.buttons[MOUSE_BUTTON_RIGHT].released = 1;
                        input.buttons[MOUSE_BUTTON_RIGHT].down = 0;
                    }
                }
            }
            result = DefWindowProc(handle, umsg, wparam, lparam);
        } break;
        default: {
            result = DefWindowProc(handle, umsg, wparam, lparam);
        } break;
    }

    return result;
}

b8 InitWindow(Window *win) {
    if (!win->title) win->title = "Window";

    int posx = win->pos.x;
    if (!posx) posx = CW_USEDEFAULT;

    int posy = win->pos.y;
    if (!posy) posy = CW_USEDEFAULT;

    int width = win->size.x;
    if (!width) width = CW_USEDEFAULT;

    int height = win->size.y;
    if (!height) height = CW_USEDEFAULT;

    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    cursor_handles[0] = LoadCursor(0, IDC_ARROW);
    cursor_handles[1] = LoadCursor(0, IDC_HAND);

    WNDCLASSEXA window_class = {};
    window_class.cbSize = sizeof(window_class);
    window_class.lpfnWndProc = WindowProc;
    window_class.hInstance = GetModuleHandle(0);
    window_class.hCursor = cursor_handles[0];
    window_class.lpszClassName = "PlatformWindowClass";

    if (!RegisterClassExA(&window_class)) {
        Print("RegisterClassExA failed.\n");
        return 0;
    }

    if (width != CW_USEDEFAULT && height != CW_USEDEFAULT) {
        RECT rect = {0, 0, width, height};
        if (AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE)) {
            width = rect.right - rect.left;
            height = rect.bottom - rect.top;
        }
    }

    HWND handle = CreateWindowExA(0,
                                window_class.lpszClassName,
                                win->title,
                                WS_OVERLAPPEDWINDOW,
                                posx,
                                posy,
                                width,
                                height,
                                0,
                                0,
                                window_class.hInstance,
                                0);

    if (!handle) {
        Print("CreateWindowExA failed.\n");
        return 0;
    }

    SetWindowLongPtr(handle, GWLP_USERDATA, (LONG_PTR) win);

    // Setup Raw Mouse input
    RAWINPUTDEVICE raw_input_device = {};
    raw_input_device.usUsagePage = 0x01;
    raw_input_device.usUsage = 0x02;
    raw_input_device.hwndTarget = handle;
    
    if (!RegisterRawInputDevices(&raw_input_device, 1, sizeof(raw_input_device))) {
        Print("Failed to register mouse raw input\n");
        return 0;
    }

    win->device_ctx = GetDC(handle);

    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cAlphaBits = 8;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    int format = ChoosePixelFormat(win->device_ctx, &pfd);
    if (!format) {
        Print("Failed ChoosePixelFormat!\n");
        return 0;
    }
    SetPixelFormat(win->device_ctx, format, &pfd);

    // Show Window
    ShowWindow(handle, SW_SHOW);

    win->handle = handle;
    win->running = 1;
    win->style = WS_OVERLAPPEDWINDOW;

    return 1;
}

void DestroyWindow(Window *win) {
    ReleaseDC(win->handle, win->device_ctx);
    DestroyWindow(win->handle);
}

void UpdateWindow(Window *win) {
    win->resized = 0;

    input.text[0] = 0;
    input.text_len = 0;

    input.mouse_delta_pos.x = 0;
    input.mouse_delta_pos.y = 0;
    input.delta_scroll = 0;

    for (int i = 0; i < WIN_MAX_BUTTONS; ++i) {
        input.buttons[i].pressed = 0;
        input.buttons[i].released = 0;
    }

    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
        switch (message.message) {
            default: {
                TranslateMessage(&message);
                DispatchMessage(&message);
            } break;
        }
    }

    // Window Info
    RECT client_rect;
    GetClientRect(win->handle, &client_rect);

    win->size.x = client_rect.right - client_rect.left;
    win->size.y = client_rect.bottom - client_rect.top;

    POINT win_pos = {client_rect.left, client_rect.top};
    ClientToScreen(win->handle, &win_pos);

    win->pos.x = win_pos.x;
    win->pos.y = win_pos.y;

    // Keyboard input
    BYTE key_states[256];
    GetKeyboardState(key_states);
    for (int i = 5; i < Min(256, WIN_MAX_KEYS); ++i) {
        ButtonInput *button = input.keys + i;

        b8 down = key_states[i] >> 7;
        b8 wasdown = button->down;

        button->down = down;
        button->pressed = !wasdown && down;
        button->released = wasdown && !down;
    }

    // Mouse input
    POINT mouse_pos;
    GetCursorPos(&mouse_pos);
    mouse_pos.x -= win->pos.x;
    mouse_pos.y -= win->pos.y;

    input.mouse_pos.x = mouse_pos.x;
    input.mouse_pos.y = mouse_pos.y;
}

void ToggleFullscreen(Window *win) {
    win->fullscreen = !win->fullscreen;

    if (win->fullscreen) {
        GetWindowRect(win->handle, &win->rect);
        win->style = GetWindowLong(win->handle, GWL_STYLE);

        SetWindowLong(
            win->handle, GWL_STYLE, win->style & ~(WS_CAPTION | WS_THICKFRAME));

        MONITORINFO monitor_info = {};
        monitor_info.cbSize = sizeof(MONITORINFO);

        GetMonitorInfo(MonitorFromWindow(win->handle, MONITOR_DEFAULTTOPRIMARY), &monitor_info);

        SetWindowPos(win->handle,
                     HWND_TOP,
                     monitor_info.rcMonitor.left,
                     monitor_info.rcMonitor.top,
                     monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                     monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    } else {
        SetWindowLong(win->handle, GWL_STYLE, win->style);
        SetWindowPos(win->handle,
                     NULL,
                     win->rect.left,
                     win->rect.top,
                     win->rect.right - win->rect.left,
                     win->rect.bottom - win->rect.top,
                     SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

void MaximizeWindow(Window *win) {
    ShowWindow(win->handle, SW_MAXIMIZE);

    RECT client_rect;
    GetClientRect(win->handle, &client_rect);

    win->size.x = client_rect.right - client_rect.left;
    win->size.y = client_rect.bottom - client_rect.top;

    POINT win_pos = {client_rect.left, client_rect.top};
    ClientToScreen(win->handle, &win_pos);

    win->pos.x = win_pos.x;
    win->pos.y = win_pos.y;
}

b32 IsMaximized(Window *win) {
    b32 result = 0;

    WINDOWPLACEMENT placement = {};
    placement.length = sizeof(placement);
    if (GetWindowPlacement(win->handle, &placement)) {
        result = placement.showCmd == SW_SHOWMAXIMIZED; 
    }

    return result;
}

void SetWindowTitle(Window *win, const char *title) {
    SetWindowTextA(win->handle, title);
}

void SetCursorToArrow() {
    SetCursor(cursor_handles[0]);
    ClipCursor(0);
}

void SetCursorToPointer() {
    SetCursor(cursor_handles[1]);
    ClipCursor(0);
}

void SetCursorToNone(Window *win) {
    SetCursor(0);

    RECT rect;
    GetClientRect(win->handle, &rect);
    POINT ul = { rect.left, rect.top };
    POINT lr = { rect.right, rect.bottom };

    ClientToScreen(win->handle, &ul);
    ClientToScreen(win->handle, &lr);

    rect.left = ul.x;
    rect.top = ul.y;
    rect.right = lr.x;
    rect.bottom = lr.y;
    ClipCursor(&rect);
}

char *GetTextInput(int *len) {
    *len = input.text_len;
    return input.text;
}

b8 IsKeyDown(u8 key) {
    return input.keys[key].down;
}

b8 WasKeyPressed(u8 key) {
    return input.keys[key].pressed;
}

b8 WasKeyReleased(u8 Key) {
    return input.keys[Key].released;
}

b8 IsButtonDown(u8 button) {
    return input.buttons[button].down;
}

b8 WasButtonPressed(u8 button) {
    return input.buttons[button].pressed;
}

b8 WasButtonReleased(u8 button) {
    return input.buttons[button].released;
}

Int2 GetMousePosition() {
    return input.mouse_pos;
}

Int2 GetMouseDeltaPosition() {
    return input.mouse_delta_pos;
}

int GetMouseScroll() {
    return input.scroll;
}

int GetMouseScrollDelta() {
    return input.delta_scroll;
}
