#include <iostream>
#include "engine.h"
#include "tetris.h"
void* BitmapMemory;

int BitmapWidth;
int BitmapHeight;

int ClientWidth;
int ClientHeight;

void DrawPixel(int X, int Y, u32 Color) {
    u32* Pixel = (u32*)BitmapMemory;
    Pixel += Y * BitmapWidth + X;
    *Pixel = Color;
}

void ClearScreen(u32 Color) {

    u32* Pixel = (u32*)BitmapMemory;

    for (int Index = 0; Index < BitmapWidth * BitmapHeight; ++Index) {
        *Pixel++ = Color;
    }
}

void DrawRectangle(int RectangleX, int RectangleY, int RectangleWidth, int RectangleHeight, u32 Color) {
    u32* Pixel = (u32*)BitmapMemory;
    Pixel += RectangleY * BitmapWidth + RectangleX;
    if (RectangleY < 0)
        return;
    for (int Y = 0; Y < RectangleHeight; ++Y) {
        for (int X = 0; X < RectangleWidth; ++X) {     
            

            *Pixel++ = Color;
        }
        Pixel += BitmapWidth - RectangleWidth;
    }
    
}
int xpos = 0;

fnc* onKeySubs[20] = {};
int onKeyIndex = 0;
void onKeyDown(fnc* func) {
    onKeySubs[onKeyIndex] = func;
    onKeyIndex += 1;
}

void invokeOnKey(char k) {
    for (int i = 0; i < 20; i++) {
        if (onKeySubs[i] != NULL)
            onKeySubs[i](k);
    }
}

int random(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}

// change number to up the amm of keys in the buffer
char keysDown[10] = {}; 
void handleInput(char k, bool d) {

    if (d) {
        invokeOnKey(k);
        for (int i = 0; i < sizeof(keysDown); i++) {
            if (k != NULL) {
                keysDown[i] = k;
            }
        }
    }
    else {
        for (int i = 0; i < sizeof(keysDown); i++) {
            if (keysDown[i] == k) {
                keysDown[i] = NULL;
            }
        }
    }
}

auto start = std::chrono::system_clock::now();
float getTime() {
    auto now = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = now - start;

    return elapsed_seconds.count();
}


bool isKeyDown(char k) {
    for (int i = 0; i < sizeof(keysDown); i++) {
        if (keysDown[i] == k) {
            return true;
        }
    }
    return false;
}

LRESULT CALLBACK WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
    switch (Message) {
    case WM_KEYDOWN: {
        handleInput(WParam, true);
        switch (WParam) {
        case 'O': {
            // exits program
            DestroyWindow(Window);
        };
        }
    } break;
    case WM_DESTROY: {
        PostQuitMessage(0);
    } break;
    case WM_KEYUP: {
        handleInput(WParam, false);
    } break;
    default: {
        return DefWindowProc(Window, Message, WParam, LParam);
    }
    }

    return 0;
}

// hacky way of storibg function pointers
fn* drawSubscribers[20] = {};
int drawSubIndex = 0;
void onDraw(fn *func) {
    drawSubscribers[drawSubIndex] = func;
    drawSubIndex += 1;
}


fn* updateSubscribers[20] = {};
int updateSubIndex = 0;
void onUpdate(fn* func) {
    updateSubscribers[updateSubIndex] = func;
    updateSubIndex += 1;
}

void invokeUpdate() {
    for (int i = 0; i < 20; i++) {
        if (updateSubscribers[i] != NULL) {
            updateSubscribers[i]();
        }
    }
}

void invokeDraw() {
    for (int i = 0; i < 20; i++) {
        if (drawSubscribers[i] != NULL) {
            drawSubscribers[i]();
        }
    }
}

int WINAPI wWinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PWSTR CmdLine, int CmdShow) {

    WNDCLASS WindowClass = {};
    const wchar_t ClassName[] = L"MyWindowClass";
    WindowClass.lpfnWndProc = WindowProc;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = ClassName;
    WindowClass.hCursor = LoadCursor(0, IDC_CROSS);
    if (!RegisterClass(&WindowClass)) {
        MessageBox(0, L"RegisterClass failed", 0, 0);
        return GetLastError();
    }
    // creates a console then redirects cout to it bc cpp is goofy like that
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

    tetris::setup();

    int winSize = 1000;
    HWND Window = CreateWindowEx(0, ClassName, L"Tetris",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        winSize / 2, winSize,
        0, 0, Instance, 0);

    if (!Window) {
        MessageBox(0, L"CreateWindowEx failed", 0, 0);
        return GetLastError();
    }



    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    ClientWidth = ClientRect.right - ClientRect.left;
    ClientHeight = ClientRect.bottom - ClientRect.top;

    BitmapWidth = ClientWidth;
    BitmapHeight = ClientHeight;


    int BytesPerPixel = 4;

    BitmapMemory = VirtualAlloc(0,
        BitmapWidth * BitmapHeight * BytesPerPixel,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE
    );


    BITMAPINFO BitmapInfo;
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    HDC DeviceContext = GetDC(Window);

    bool Running = true;

    while (Running) {
        MSG Message;
        while (PeekMessage(&Message, nullptr, 0, 0, PM_REMOVE)) {
            if (Message.message == WM_QUIT) Running = false;
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }

        ClearScreen(0x000000);

        invokeUpdate();

        StretchDIBits(DeviceContext,
            0, 0,
            BitmapWidth, BitmapHeight,
            0, 0,
            ClientWidth, ClientHeight,
            BitmapMemory, &BitmapInfo,
            DIB_RGB_COLORS, SRCCOPY);
    }

    return 0;
}
