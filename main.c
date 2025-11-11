#include <string.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "../include/utils.h"
#include "../include/windivert.h"
#include "../include/consts.h"
#include "../include/windrop.h"


engine_t engine;
RECT winrect;
DWORD errcode;
HFONT hFont;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lparam);

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR cmdline,int nCmdShow) {
    if (!IsUserAnAdmin()) {
        ALERT("Run as admin");
        return 1;
    }
    hFont = CreateFontW(-20, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, TURKISH_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_STROKE_PRECIS, PROOF_QUALITY, DEFAULT_PITCH, W("Arial"));
    if (!hFont) {
        sMessageBoxW(W("Error"),W("Cannot create font"));
    }

    LPCWSTR windowName = L"WinDrop";
    memset(&engine,0,sizeof(engine));
    LPCWSTR className = L"WinDrop";
    WNDCLASSEXW wex = { sizeof(WNDCLASSEXW) };
    wex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wex.hInstance = hInstance;
    wex.lpfnWndProc = WndProc;
    wex.lpszClassName = className;
    wex.hCursor = LoadCursorW(hInstance,MAKEINTRESOURCEW(IDC_ARROW));
    wex.hIconSm = LoadIconW(hInstance,MAKEINTRESOURCEW(IDI_MAIN_ICON));
    RegisterClassExW(&wex);
    if (!GetClassInfoExW(hInstance,className,&wex)) {
        ALERT("Cannot Register window class :/");
        return 1;
    }
    HWND window = CreateWindowW(className,windowName,WS_OVERLAPPEDWINDOW | WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,500,500,NULL,NULL,hInstance,NULL);
    if (!window) {
        ALERT("Cannot create window");
        return 1;
    }
    engine.param_t.hwnd = window;
    ShowWindow(window,nCmdShow);
    UpdateWindow(window);
    GetWindowRect(window,&winrect);
    GetWindowRect(window,&winrect);
    MSG message;
    while (GetMessageW(&message,NULL,0,0)) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return message.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lparam) {

    switch (msg) {
        case WM_DESTROY:
            CloseEngine(&engine);
            DeleteObject(hFont);
            PostQuitMessage(0);
            return 0;
        case WM_CREATE:
            HWND start_button_hwnd = CreateWindowW(L"BUTTON",L"Start",BS_DEFPUSHBUTTON | WS_CHILD | WS_VISIBLE,10,150,100,50,hwnd,(HMENU)TAG_START_BUTTON,(HINSTANCE)GetModuleHandle(NULL),NULL);
            HWND stop_button_hwnd = CreateWindowW(W("BUTTON"),L"Stop",BS_DEFPUSHBUTTON | WS_CHILD | WS_VISIBLE,490 - 115,150,100,50,hwnd,(HMENU)TAG_STOP_BUTTON,(HINSTANCE)GetModuleHandle(NULL),NULL);
            if (!start_button_hwnd || !stop_button_hwnd) {
                ALERT("Cannot create button(s)"); // deprecated
                SendMessage(hwnd,WM_DESTROY,0,0);
            }
            SendMessageW(start_button_hwnd,WM_SETFONT,(WPARAM)hFont,TRUE);
            SendMessageW(stop_button_hwnd,WM_SETFONT,(WPARAM)hFont,TRUE);
            SendMessageW(hwnd,WM_SETFONT,(WPARAM)hFont,TRUE);
            break;
        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    switch (LOWORD(wParam)) {
                        case TAG_START_BUTTON: {
                            if (!engine.param_t.alive) {
                                engine.param_t.wDivert = WinDivertOpen("(outbound and ip) and (tcp or udp) and (tcp.PayloadLength > 0 or udp.PayloadLength > 0)",WINDIVERT_LAYER_NETWORK,0,0);
                                if (engine.param_t.wDivert == INVALID_HANDLE_VALUE ) {
                                    errcode = GetLastError();
                                    SendMessageW(hwnd,WM_SHOW_ERROR,0,0);
                                } else {
                                    SendMessageW(hwnd,WM_START_WINDROP,0,0);
                                }
                            }
                            break;
                        }
                        case TAG_STOP_BUTTON: {
                            if (engine.param_t.alive) {
                                WriteText("Windrop stopped ",hwnd,&winrect,strlen("Windrop stopped"));
                                CloseEngine(&engine);
                            }
                            
                        }
                        break;
                    }
            }
            break;
        case WM_SHOW_ERROR:
            ShowDetailedErrorCode(errcode);
            break;
        case WM_START_WINDROP: {
            if (engine.param_t.alive) {
                WriteText("Windrop already started",hwnd,&winrect,strlen("Windrop already started"));
            } else {
                engine.param_t.alive = TRUE;
                engine.main_thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)StartEngine,(LPVOID)&engine.param_t,0,NULL);
                WriteText("Windrop started",hwnd,&winrect,strlen("Windrop started"));
            }
            break;      
        }
        case WM_CLOSE:
            SendMessageW(hwnd,WM_DESTROY,0,0);
            break;
        case WM_GETMINMAXINFO: {
            MINMAXINFO* minmaxinfo = (MINMAXINFO*)lparam;
            minmaxinfo->ptMinTrackSize.x = 500;
            minmaxinfo->ptMinTrackSize.y = 500; // We restricted the minimum  and maximum length to 500x500 and must be pointer
            minmaxinfo->ptMaxTrackSize.x = 500;
            minmaxinfo->ptMaxTrackSize.y = 500;
            break; 
        }
        case WM_DRAWTEXT: {
            WriteText((char*)wParam,hwnd,&winrect,(size_t)lparam);
            break;
        }
        default:
            return DefWindowProcW(hwnd,msg,wParam,lparam);
    }
    return 0;
}