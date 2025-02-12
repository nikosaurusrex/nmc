#pragma once

#include "../Platform/Platform.h"

#include "Keys.h"

enum {
    WIN_MAX_KEYS = 256,
    WIN_MAX_TEXT = 256,
    WIN_MAX_BUTTONS = 12,
};

struct Int2 {
    int x;
    int y;
};

typedef void (*KeyPressCallbackFunction)(u32 codepoint);
typedef void (*CharCallbackFunction)(char ch);

struct Window {
    const char *title;
    Int2 pos;
    Int2 size;
    b8 resized;

    b8 running;
    b8 fullscreen;

    KeyPressCallbackFunction key_callback;
    CharCallbackFunction char_callback;

#if OS_WINDOWS
    HWND handle;
    HDC device_ctx;
    RECT rect;
    DWORD style;
#endif
};

struct ButtonInput {
    b8 down;
    b8 pressed;
    b8 released;
};

struct InputState {
    ButtonInput keys[WIN_MAX_KEYS];
    ButtonInput buttons[WIN_MAX_BUTTONS];
    
    int scroll;
    int delta_scroll;
    Int2 mouse_pos;
    Int2 mouse_delta_pos;

    char text[WIN_MAX_TEXT];
    u32 text_len;
};

b8 InitWindow(Window *win);
void DestroyWindow(Window *win);

void UpdateWindow(Window *win);

void ToggleFullscreen(Window *win);
void MaximizeWindow(Window *win);
b32 IsMaximized(Window *win);

void SetWindowTitle(Window *win, const char *title);

void SetCursorToArrow();
void SetCursorToPointer();
void SetCursorToNone(Window *win);

char *GetTextInput(int *len);
b8 IsKeyDown(u8 key);
b8 WasKeyPressed(u8 key);
b8 WasKeyReleased(u8 key);

b8 IsButtonDown(u8 button);
b8 WasButtonPressed(u8 button);
b8 WasButtonReleased(u8 button);

Int2 GetMousePosition();
Int2 GetMouseDeltaPosition();
int GetMouseScroll();
int GetMouseScrollDelta();
