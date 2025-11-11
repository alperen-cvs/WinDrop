//
// Created by alper on 31.10.2025.
//
#pragma once

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#define ALERT(msg)(MessageBoxW(NULL,L##msg,L"Error",MB_OK|MB_ICONERROR));
#define W(text)(L##text)

void sMessageBoxW(wchar_t* title,wchar_t* format,...) {
    wchar_t buffer[4096];
    va_list list;
    va_start(list,format);
    vswprintf(buffer,sizeof(buffer),format,list);
    MessageBoxW(NULL,buffer,title,MB_OK | MB_ICONINFORMATION);
    va_end(list);
}

typedef struct tagWIN32SERVICES {
    DWORD serviceCount;
    LPENUM_SERVICE_STATUSW serviceStatus;
} *Win32Services;

typedef struct tagErrorTable {
    DWORD errcode;
    wchar_t error_text[4048];
} ErrorTable;

void ShowDetailedErrorCode(DWORD err) {
    ErrorTable errtable[5] = {
        {ERROR_FILE_NOT_FOUND,W("Driver file doesn't exists :/ Check WinDivert64.sys file")},
        {ERROR_ACCESS_DENIED,W("Run as administrator :/")},
        {ERROR_INVALID_PARAMETER,W("WinDivertOpen function parameters are invalid check params :/")},
        {ERROR_INVALID_IMAGE_HASH,W("Bad Sys(Driver) file signature :/")},
        {ERROR_DRIVER_BLOCKED,W("Driver blocked by system or by AV :/")}
    };
    size_t errtable_size = sizeof(errtable) / sizeof(ErrorTable);
    for (size_t start = 0;start < errtable_size;++start) {
        if (errtable[start].errcode == err) {
            sMessageBoxW(
                W("Cannot open windivert :/"),
                W("windivert error code: %lu Reason: %ls"),
                err,
                errtable[start].error_text
            );
            break;
        }
    }

}

/*
BOOL IsDllExists(const wchar_t* path) {
    DWORD attr = GetFileAttributesW(path);
    return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)); NOTE:Fix here
}
*/
DWORD GetBufferServiceStatusSize(SC_HANDLE hService) {
    if (!hService) {
        ALERT("Error SC_HANDLE is invalid :/");
        return 0;
    }
    DWORD buffersize = 0;
    DWORD servicecount = 0;
    EnumServicesStatusW(
        hService,
        SERVICE_KERNEL_DRIVER,
        SERVICE_ACTIVE,
        NULL,
        0,
        &buffersize,
        &servicecount,
        NULL
    );
    return buffersize;
}

Win32Services GetWin32Services(SC_HANDLE hService,DWORD serviceStatusType) {
    if (!hService) {
        ALERT("Error SC_HANDLE is invalid :/");
        return NULL;
    }
    Win32Services serviceStatus = (Win32Services)malloc(sizeof(struct tagWIN32SERVICES));
    if (!serviceStatus) {
        ALERT("Memory allocation error");
        return NULL;
    }
    DWORD buffersize = GetBufferServiceStatusSize(hService);
    if (!buffersize || buffersize == 0) {
        ALERT("GetBufferServiceStatus failed");
        free(serviceStatus);
        return NULL;
    }
    DWORD buffsize = 0;
    serviceStatus->serviceStatus = (LPENUM_SERVICE_STATUSW)malloc(buffersize);
    if (!serviceStatus->serviceStatus) {
        ALERT("Memory allocation error");
        free(serviceStatus);
        return NULL;
    }
    BOOL result = EnumServicesStatusW(
        hService,
        SERVICE_KERNEL_DRIVER,
        serviceStatusType,
        serviceStatus->serviceStatus,
        buffersize,
        &buffsize,
        &serviceStatus->serviceCount,NULL);
    if (!result) {
        ALERT("EnumServicesStatusW failed");
        free(serviceStatus->serviceStatus);
        free(serviceStatus);
        return NULL;
    }
    return serviceStatus;
}

void DestroyWin32Services(Win32Services serviceStatus) {
    if (!serviceStatus) {
        ALERT("Error SC_HANDLE is invalid :/");
        return;
    }
    free(serviceStatus->serviceStatus);
    free(serviceStatus);
}

BOOL IsServiceRunning(SC_HANDLE hService) {
    if (!hService) {
        ALERT("Error SC_HANDLE is invalid :/");
        return FALSE;
    }
    Win32Services serviceStatus = GetWin32Services(hService,SERVICE_ACTIVE);
    if (!serviceStatus) {
        ALERT("GetWin32Services failed");
        return FALSE;
    }
    for (DWORD i = 0; i < serviceStatus->serviceCount; i++) {
        if (wcscmp(L"firewall", serviceStatus->serviceStatus[i].lpServiceName) == 0) {
            return TRUE;
        }
    }
    DestroyWin32Services(serviceStatus);
    return FALSE;
}

BOOL RunServiceIfNeeded(SC_HANDLE hService) {
    if (!hService) {
        ALERT("Error SC_HANDLE is invalid :/");
        return FALSE;
    }
    SC_HANDLE serviceHandle = OpenServiceW(hService,L"firewall",SERVICE_ALL_ACCESS);
    if (!serviceHandle) {
        ALERT("OpenServiceW failed");
        return FALSE;
    }
    BOOL result = StartServiceW(serviceHandle,0,NULL);
    CloseServiceHandle(serviceHandle);
    return result;
}
#ifdef DEBUG
    BOOL IsRunningWM(char* buffer,size_t bufflen) {
        return 1;
    }
#else
    BOOL IsRunningWM(char* buffer,size_t bufflen) {
        return (gethostname(buffer,bufflen) != 0 && strcmp(buffer,"alper") != 0);
    }
#endif

void WriteText(const char* text,HWND hwnd,RECT* winrect,size_t textlen) {
    HDC dc = GetDC(hwnd);
    SetBkMode(dc, OPAQUE); // removes old garbage text 
    SetBkColor(dc, RGB(255, 255, 255));
    TextOut(dc,175,50,text,textlen);
    ReleaseDC(hwnd,dc);
}

void SendWMSetFontMessageToHwnds(HWND handle[],HFONT fontobj,size_t cnt) {
    for (size_t start = 0;start < cnt;++start) {
        SendMessageW(handle[start],WM_SETFONT,(WPARAM)fontobj,(WPARAM)TRUE);
    }
}